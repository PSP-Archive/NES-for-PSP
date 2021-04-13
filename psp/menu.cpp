#include "pspsdk.h"
#include "filer.h"
#include "menu.h"
#include "colbl.c"
#include "zlibInterface.h"

#include "../InfoNES.h"

BYTE rom_image[0x200000];

SETTING setting;

int bBitmap;
unsigned short bgBitmap[480*272];

int LoadSRAM();
int SaveSRAM();

void save_config(void)
{
	char CfgPath[MAX_PATH];
	char *p;
	
	getDir(psp_szCurrentPath);
	strcpy(CfgPath, psp_szCurrentPath);
	p = strrchr(CfgPath, '/')+1;
	strcpy(p, "NES.CFG");

	int fd;
	fd = sceIoOpen(CfgPath,O_CREAT|O_RDWR|O_TRUNC, 0777);
	if(fd>=0){
		sceIoWrite(fd, &setting, sizeof(setting));
		sceIoClose(fd);
	}
}

void load_config(void)
{
	int i;
	char CfgPath[MAX_PATH];
	char *p;
	
	getDir(psp_szCurrentPath);
	strcpy(CfgPath, psp_szCurrentPath);
	p = strrchr(CfgPath, '/')+1;
	strcpy(p, "NES.CFG");

	int fd;
	fd = sceIoOpen(CfgPath,O_RDONLY, 0777);
	if(fd>=0){
		sceIoRead(fd, &setting, sizeof(setting));
		sceIoClose(fd);
		if(setting.key_config[6]==0)
			setting.key_config[6] = CTRL_TRIANGLE;
		if(setting.bgbright<0 || setting.bgbright>100)
			setting.bgbright=100;
		if(!strcmp(setting.vercnf, "NES0.5"))
			return;
	}

	strcpy(setting.vercnf, "NES0.5");
	setting.screensize = 0;
	setting.cpuclock = 0;
	setting.frameskip  = 0;
	setting.vsync = 0;
	setting.sound = 1;
	setting.key_config[0] = CTRL_CIRCLE;
	setting.key_config[1] = CTRL_CROSS;
	setting.key_config[2] = CTRL_TRIANGLE;
	setting.key_config[3] = CTRL_SQUARE;
	setting.key_config[4] = CTRL_SELECT;
	setting.key_config[5] = CTRL_START;
	setting.key_config[6] = CTRL_L;
	for(i=7; i<16; i++)
		setting.key_config[i] = 0;
	setting.color[0] = DEF_COLOR0;
	setting.color[1] = DEF_COLOR1;
	setting.color[2] = DEF_COLOR2;
	setting.color[3] = DEF_COLOR3;
	setting.bgbright=100;
}

// Unzip 対応 by ruka

// コールバック受け渡し用
typedef struct {
	BYTE *p_rom_image;			// pointer to rom image
	long rom_size;				// rom size
	char szFileName[MAX_PATH];	// extracted file name
}ROM_INFO, *LPROM_INFO;

// せっかくなのでプログレスでも出してみます
void draw_load_rom_progress(unsigned long ulExtractSize, unsigned long ulCurrentPosition)
{
	int nPer = 100 * ulExtractSize / ulCurrentPosition;
	static int nOldPer = 0;
	if (nOldPer == nPer & 0xFFFFFFFE) {
		return ;
	}
	nOldPer = nPer;
	if(bBitmap)
		pspBitBlt(0,0,480,272,1,bgBitmap);
	else
		pspFillVRAM(setting.color[0]);
	// プログレス
	pspDrawFrame(89,121,391,141,setting.color[1]);
	pspFillBox(90,123, 90+nPer*3, 139,setting.color[1]);
	// ％
	char szPer[16];
	itoa(nPer, szPer);
	strcat(szPer, "%");
	pspPrint(28,16,setting.color[3],szPer);
	// pgScreenFlipV()を使うとpgWaitVが呼ばれてしまうのでこちらで。
	// プログレスだからちらついても良いよね～
	pspScreenFlip();
}

// Unzip コールバック
int funcUnzipCallback(int nCallbackId, unsigned long ulExtractSize, unsigned long ulCurrentPosition,
                      const void *pData, unsigned long ulDataSize, unsigned long ulUserData)
{
    const char *pszFileName;
    int nExtId;
    const unsigned char *pbData;
    LPROM_INFO pRomInfo = (LPROM_INFO)ulUserData;

    switch(nCallbackId) {
    case UZCB_FIND_FILE:
		pszFileName = (const char *)pData;
		nExtId = getExtId(pszFileName);
		// 拡張子がnesなら展開
		if (nExtId == EXT_NES) {
			// 展開する名前、rom sizeを覚えておく
			strcpy(pRomInfo->szFileName, pszFileName);
			pRomInfo->rom_size = ulExtractSize;
			return UZCBR_OK;
		}
        break;
    case UZCB_EXTRACT_PROGRESS:
		pbData = (const unsigned char *)pData;
		// 展開されたデータを格納しよう
		memcpy(pRomInfo->p_rom_image + ulCurrentPosition, pbData, ulDataSize);
		draw_load_rom_progress(ulCurrentPosition + ulDataSize, ulExtractSize);
		return UZCBR_OK;
        break;
    default: // unknown...
		pspFillVRAM(RGB(255,0,0));
		pspPrint(0,0,0xFFFF,"Unzip fatal error.");
		pspScreenFlipV();
        break;
    }
    return UZCBR_PASS;
}



// load rom image by ruka
long load_rom(const char *szRomPath)
{
	int fd;
	long lReadSize;
	ROM_INFO stRomInfo;
	int nRet;
	int nExtId = getExtId(szRomPath);
	switch(nExtId) {
	case EXT_NES:	// "nes"
		fd=sceIoOpen(szRomPath, O_RDONLY, 0777);
		lReadSize = sceIoRead(fd, rom_image, 0x4000*256+0x2000*256);
		sceIoClose(fd);
		break;
	case EXT_ZIP:	// "zip"
		stRomInfo.p_rom_image = rom_image;
		stRomInfo.rom_size = 0;
		memset(stRomInfo.szFileName, 0x00, sizeof(stRomInfo.szFileName));
		// Unzipコールバックセット
		Unzip_setCallback(funcUnzipCallback);
		// Unzip展開する
	    nRet = Unzip_execExtract(szRomPath, (unsigned long)&stRomInfo);
		if (nRet != UZEXR_OK) {
			// 読み込み失敗！ - このコードでは、UZEXR_CANCELもここに来て
			// しまうがコールバックでキャンセルしてないので無視
			lReadSize = 0;
			pspFillVRAM(RGB(255,0,0));
			pspPrint(0,0,0xFFFF,"Unzip fatal error.");
			pspScreenFlipV();
		}
		lReadSize = stRomInfo.rom_size;
		break;
	default:
		lReadSize = 0;
		break;
	}
	return lReadSize;
}

int save_state(void)
{
	return 0;
}

int load_state(void)
{
	return 0;
}

// by kwn
void load_menu_bg()
{
	BYTE *menu_bg;
	unsigned char *vptr;
	static BYTE menu_bg_buf[480*272*3+0x36];
	char BgPath[MAX_PATH];
	char *p;
 	unsigned short x,y,yy,r,g,b,data;

	getDir(psp_szCurrentPath);
	strcpy(BgPath, psp_szCurrentPath);
	p = strrchr(BgPath, '/')+1;
	strcpy(p, "MENU.BMP");

	int fd;
	fd = sceIoOpen(BgPath,O_RDONLY,0777);
	if(fd>=0){
		sceIoRead(fd, menu_bg_buf, 480*272*3+0x36);
		sceIoClose(fd);

		menu_bg = menu_bg_buf + 0x36;
		vptr=(unsigned char*)bgBitmap;
		for(y=0; y<272; y++){
			for(x=0; x<480; x++){
				yy = 271 - y;
				r = *(menu_bg + (yy*480 + x)*3 + 2);
				g = *(menu_bg + (yy*480 + x)*3 + 1);
				b = *(menu_bg + (yy*480 + x)*3);
				data = (((b & 0xf8) << 7) | ((g & 0xf8) << 2) | (r >> 3));
				*(unsigned short *)vptr=data;
				vptr+=2;
			}
		}
		bBitmap = 1;
	}else{
		bBitmap = 0;
	}
}

// 半透明処理
unsigned short rgbTransp(unsigned short fgRGB, unsigned short bgRGB, int alpha) {

    unsigned short fgR, fgG, fgB;
    unsigned short bgR, bgG, bgB;
	unsigned short R, G, B;
 	unsigned short rgb;

    fgB = (fgRGB >> 10) & 0x1F;
    fgG = (fgRGB >> 5) & 0x1F;
    fgR = fgRGB & 0x1F;

    bgB = (bgRGB >> 10) & 0x1F;
    bgG = (bgRGB >> 5) & 0x1F;
    bgR = bgRGB & 0x1F;

	R = coltbl[fgR][bgR][alpha/10];
	G = coltbl[fgG][bgG][alpha/10];
	B = coltbl[fgB][bgB][alpha/10];

	rgb = (((B & 0x1F)<<10)+((G & 0x1F)<<5)+((R & 0x1F)<<0)+0x8000);
    return rgb;
}

void bgbright_change()
{
	unsigned short *vptr,rgb;
 	int i;

	vptr=bgBitmap;
	for(i=0; i<272*480; i++){
			rgb = *vptr;
			*vptr = rgbTransp(rgb, 0x0000, setting.bgbright);
			vptr++;
	}
}

void menu_frame(const char *msg0, const char *msg1)
{
	if(bBitmap)
		pspBitBlt(0,0,480,272,1,bgBitmap);
	else
		pspFillVRAM(setting.color[0]);
	pspTextOut(365, 0, setting.color[1],(const unsigned char *)"■ NES for PSP 0.50 ■");
	// メッセージなど
	if(msg0!=0) pspTextOut(17, 14, setting.color[2], (const unsigned char *)msg0);
	pspDrawFrame(17,25,463,248,setting.color[1]);
	pspDrawFrame(18,26,462,247,setting.color[1]);
	// 操作説明
	if(msg1!=0) pspTextOut(17, 252, setting.color[2], (const unsigned char *)msg1);
}

void menu_colorconfig(void)
{
	enum
	{
		COLOR0_R=0,
		COLOR0_G,
		COLOR0_B,
		COLOR1_R,
		COLOR1_G,
		COLOR1_B,
		COLOR2_R,
		COLOR2_G,
		COLOR2_B,
		COLOR3_R,
		COLOR3_G,
		COLOR3_B,
		BG_BRIGHT,
		INIT,
		EXIT,
	};
	char tmp[4], msg[256];
	int color[4][3];
	int sel=0, x, y, i;

	memset(color, 0, sizeof(int)*4*3);
	for(i=0; i<4; i++){
		color[i][2] = setting.color[i]>>10 & 0x1F;
		color[i][1] = setting.color[i]>>5 & 0x1F;
		color[i][0] = setting.color[i] & 0x1F;
	}

	for(;;){
		pspReadPad();
		if(new_pad & CTRL_CIRCLE){
			if(sel==EXIT){
				break;
			}else if(sel==INIT){
				color[0][2] = DEF_COLOR0>>10 & 0x1F;
				color[0][1] = DEF_COLOR0>>5 & 0x1F;
				color[0][0] = DEF_COLOR0 & 0x1F;
				color[1][2] = DEF_COLOR1>>10 & 0x1F;
				color[1][1] = DEF_COLOR1>>5 & 0x1F;
				color[1][0] = DEF_COLOR1 & 0x1F;
				color[2][2] = DEF_COLOR2>>10 & 0x1F;
				color[2][1] = DEF_COLOR2>>5 & 0x1F;
				color[2][0] = DEF_COLOR2 & 0x1F;
				color[3][2] = DEF_COLOR3>>10 & 0x1F;
				color[3][1] = DEF_COLOR3>>5 & 0x1F;
				color[3][0] = DEF_COLOR3 & 0x1F;
				setting.bgbright = 100;
				if(bBitmap){
					load_menu_bg();
					bgbright_change();
				}
			}else if(sel == BG_BRIGHT) {
				//輝度変更
				setting.bgbright += 10;
				if(setting.bgbright > 100) setting.bgbright=0;
				if(bBitmap){
					load_menu_bg();
					bgbright_change();
				}
			}else{
				if(color[sel/3][sel%3]<31)
					color[sel/3][sel%3]++;
			}
		}else if(new_pad & CTRL_CROSS){
			if(sel == BG_BRIGHT) {
				//輝度変更
				setting.bgbright -= 10;
				if(setting.bgbright < 0) setting.bgbright=100;
				if(bBitmap){
					load_menu_bg();
					bgbright_change();
				}
			}else if(sel>=COLOR0_R && sel<=COLOR3_B){
				if(color[sel/3][sel%3]>0)
					color[sel/3][sel%3]--;
			}
		}else if(new_pad & CTRL_UP){
			if(sel!=0)	sel--;
			else		sel=EXIT;
		}else if(new_pad & CTRL_DOWN){
			if(sel!=EXIT)	sel++;
			else			sel=0;
		}else if(new_pad & CTRL_RIGHT){
			if(sel<COLOR1_R) 		sel=COLOR1_R;
			else if(sel<COLOR2_R)	sel=COLOR2_R;
			else if(sel<COLOR3_R)	sel=COLOR3_R;
			else if(sel<BG_BRIGHT)	sel=BG_BRIGHT;
			else if(sel<INIT)		sel=INIT;
		}else if(new_pad & CTRL_LEFT){
			if(sel>BG_BRIGHT)		sel=BG_BRIGHT;
			else if(sel>COLOR3_B)	sel=COLOR3_R;
			else if(sel>COLOR2_B)	sel=COLOR2_R;
			else if(sel>COLOR1_B)	sel=COLOR1_R;
			else					sel=COLOR0_R;
		}

		for(i=0; i<4; i++)
			setting.color[i]=color[i][2]<<10|color[i][1]<<5|color[i][0]|0x8000;

		x = 2;
		y = 5;

		if(sel>=COLOR0_R && sel<=BG_BRIGHT)
			strcpy(msg, "○：Add　×：Away");
		else
			strcpy(msg, "○：OK");

		menu_frame(0, msg);

		pspPrint(x,y++,setting.color[3],"  COLOR0 R:");
		pspPrint(x,y++,setting.color[3],"  COLOR0 G:");
		pspPrint(x,y++,setting.color[3],"  COLOR0 B:");
		y++;
		pspPrint(x,y++,setting.color[3],"  COLOR1 R:");
		pspPrint(x,y++,setting.color[3],"  COLOR1 G:");
		pspPrint(x,y++,setting.color[3],"  COLOR1 B:");
		y++;
		pspPrint(x,y++,setting.color[3],"  COLOR2 R:");
		pspPrint(x,y++,setting.color[3],"  COLOR2 G:");
		pspPrint(x,y++,setting.color[3],"  COLOR2 B:");
		y++;
		pspPrint(x,y++,setting.color[3],"  COLOR3 R:");
		pspPrint(x,y++,setting.color[3],"  COLOR3 G:");
		pspPrint(x,y++,setting.color[3],"  COLOR3 B:");
		y++;
		if(setting.bgbright / 100 == 1)
			pspPrint(x,y++,setting.color[3],"  BG BRIGHT:100%");
		else
			pspPrint(x,y++,setting.color[3],"  BG BRIGHT:  0%");
		if(setting.bgbright % 100 != 0)			// 10%～90%
			pspPutChar((x+13)*8,(y-1)*8,setting.color[3],0,'0'+setting.bgbright/10,1,0,1);
		y++;
		pspPrint(x,y++,setting.color[3],"  INIT");
		pspPrint(x,y++,setting.color[3],"  Return to Main Menu");

		x=14; y=5;
		for(i=0; i<12; i++){
			if(i!=0 && i%3==0) y++;
			itoa(color[i/3][i%3], tmp);
			pspPrint(x,y++,setting.color[3],tmp);
		}

		x = 2;
		y = sel + 5;
		if(sel>=COLOR1_R) y++;
		if(sel>=COLOR2_R) y++;
		if(sel>=COLOR3_R) y++;
		if(sel>=BG_BRIGHT) y++;
		if(sel>=INIT) y++;
		pspPutChar((x+1)*8,y*8,setting.color[3],0,127,1,0,1);

		pspScreenFlipV();
	}
}

void menu_keyconfig(void)
{
	enum
	{
		CONFIG_A = 0,
		CONFIG_B,
		CONFIG_RAPIDA,
		CONFIG_RAPIDB,
		CONFIG_SELECT,
		CONFIG_START,
		CONFIG_MENU,
		CONFIG_EXIT,
	};
	char msg[256];
	int sel=0, x, y, i;

	pspWaitVn(15);

	for(;;){
		pspReadPad();
		if(new_pad & CTRL_CIRCLE){
			if(sel == CONFIG_EXIT){
				break;
			} else {
				setting.key_config[sel] = CTRL_CIRCLE;
			}
		}else if(new_pad & CTRL_SQUARE){
			if(sel != CONFIG_EXIT)
				setting.key_config[sel] = CTRL_SQUARE;
		}else if(new_pad & CTRL_TRIANGLE){
			if(sel != CONFIG_EXIT)
				setting.key_config[sel] = CTRL_TRIANGLE;
		}else if(new_pad & CTRL_CROSS){
			if(sel != CONFIG_EXIT)
				setting.key_config[sel] = CTRL_CROSS;
		}else if(new_pad & CTRL_START){
			if(sel != CONFIG_EXIT)
				setting.key_config[sel] = CTRL_START;
		}else if(new_pad & CTRL_SELECT){
			if(sel != CONFIG_EXIT)
				setting.key_config[sel] = CTRL_SELECT;
		}else if(new_pad & CTRL_L){
			if(sel != CONFIG_EXIT)
				setting.key_config[sel] = CTRL_L;
		}else if(new_pad & CTRL_R){
			if(sel != CONFIG_EXIT)
				setting.key_config[sel] = CTRL_R;
		}else if(new_pad & CTRL_LEFT){
			if(sel != CONFIG_EXIT && sel!=CONFIG_MENU)
				setting.key_config[sel] = 0;
		}else if(new_pad & CTRL_RIGHT){
			if(sel != CONFIG_EXIT && sel!=CONFIG_MENU)
				setting.key_config[sel] = 0;
		}else if(new_pad & CTRL_UP){
			if(sel!=0)	sel--;
			else		sel=CONFIG_EXIT;
		}else if(new_pad & CTRL_DOWN){
			if(sel!=CONFIG_EXIT)	sel++;
			else				sel=0;
		}

		if(sel==CONFIG_EXIT)
			strcpy(msg,"○：OK");
		else
			strcpy(msg,"");

		menu_frame(0, msg);

		x = 2;
		y = 5;

		pspPrint(x,y++,setting.color[3],"  A BUTTON       :");
		pspPrint(x,y++,setting.color[3],"  B BUTTON       :");
		pspPrint(x,y++,setting.color[3],"  A BUTTON(RAPID):");
		pspPrint(x,y++,setting.color[3],"  B BUTTON(RAPID):");
		pspPrint(x,y++,setting.color[3],"  SELECT BUTTON  :");
		pspPrint(x,y++,setting.color[3],"  START BUTTON   :");
		pspPrint(x,y++,setting.color[3],"  MENU BUTTON    :");
		y++;                              
		pspPrint(x,y++,setting.color[3],"  Return to Main Menu");

		for (i=0; i<7; i++){
			y = i + 5;
			if (setting.key_config[i] == CTRL_SQUARE)
				pspPrint(21,y,setting.color[3],"SQUARE");
			else if (setting.key_config[i] == CTRL_TRIANGLE)
				pspPrint(21,y,setting.color[3],"TRIANGLE");
			else if (setting.key_config[i] == CTRL_CIRCLE)
				pspPrint(21,y,setting.color[3],"CIRCLE");
			else if (setting.key_config[i] == CTRL_CROSS)
				pspPrint(21,y,setting.color[3],"CROSS");
			else if (setting.key_config[i] == CTRL_START)
				pspPrint(21,y,setting.color[3],"START");
			else if (setting.key_config[i] == CTRL_SELECT)
				pspPrint(21,y,setting.color[3],"SELECT");
			else if (setting.key_config[i] == CTRL_L)
				pspPrint(21,y,setting.color[3],"L");
			else if (setting.key_config[i] == CTRL_R)
				pspPrint(21,y,setting.color[3],"R");
			else
				pspPrint(21,y,setting.color[3],"UNDEFINED");
		}

		y = sel + 5;
		if(sel >= CONFIG_EXIT)		y++;
		pspPutChar((x+1)*8,y*8,setting.color[3],0,127,1,0,1);

		pspScreenFlipV();
	}

}

void menu(void)
{
	enum
	{
		SRAM_SAVE=0,
		CPU_CLOCK,
		SCREEN_SIZE,
		FRAMESKIP,
		VSYNC,
		SOUND,
		COLOR_CONFIG,
		KEY_CONFIG,
		RESET,
		LOAD_ROM,
		CONTINUE,
	};

	char msg[256], path[MAX_PATH];
	int sel=0, x, y;
	char *p;

	msg[0]=0;

	if(!ROM_SRAM)
		sel = CPU_CLOCK;
		
	for(;;)
	{
		pspReadPad();
		if(new_pad & CTRL_CIRCLE)
		{
			if(sel == SRAM_SAVE)
			{
				if(SaveSRAM())
					strcpy(msg, "SRAM Save Failed");
				else
					strcpy(msg, "SRAM Saved Successfully");
			}else if(sel == CPU_CLOCK){
				setting.cpuclock ++;
				if(setting.cpuclock>3) setting.cpuclock=0;
			}else if(sel == SCREEN_SIZE){
				setting.screensize = !setting.screensize;
			}else if(sel == FRAMESKIP){
				setting.frameskip++;
				if(setting.frameskip>6) setting.frameskip=0;
				if(setting.frameskip!=0)
					FrameSkip = setting.frameskip-1;
			}else if(sel == VSYNC){
				setting.vsync = !setting.vsync;
			}else if(sel == SOUND){
				setting.sound = !setting.sound;
				APU_Mute = !setting.sound;
			}else if(sel == COLOR_CONFIG){
				menu_colorconfig();
				msg[0]=0;
			}else if(sel == KEY_CONFIG){
				menu_keyconfig();
				msg[0]=0;
			}else if(sel == RESET){
				InfoNES_Reset();
				if (setting.frameskip)
					FrameSkip = setting.frameskip-1;
				APU_Mute = !setting.sound;
				break;
			}else if(sel == LOAD_ROM){
				if(ROM_SRAM){
					if(!SaveSRAM())
						strcpy(FilerMsg, "Auto-SRAM Saved Successfully");
					else
						strcpy(FilerMsg, "Auto-SRAM Save Failed");
				}else
					FilerMsg[0]=0;
				if(getFilePath(szRomName)==true){
					for(;;){
						strcpy(szSaveName, szRomName);
						p = strrchr(szSaveName, '.');
						strcpy(p, ".srm");
	
						/* Load cassette */
						InfoNES_Load ( szRomName );
						
						if (setting.frameskip!=0)
							FrameSkip = setting.frameskip-1;
						APU_Mute = !setting.sound;
						
						/* Load SRAM */
						LoadSRAM();
						
						// Initialize InfoNES
						InfoNES_Init();
						break;
					}
					break;
				}
			}else if(sel == CONTINUE){
				break;
			}
		}else if(new_pad & CTRL_CROSS){
				break;
		}else if(new_pad & CTRL_UP){
			if(sel==CPU_CLOCK && !ROM_SRAM)
				sel=CONTINUE;
			else if(sel!=0)
				sel--;
			else
				sel=CONTINUE;
		}else if(new_pad & CTRL_DOWN){
			if(sel!=CONTINUE)	
				sel++;
			else if (ROM_SRAM)
					sel=0;
				else
					sel=CPU_CLOCK;
		}

		menu_frame(msg, "○：OK　×：CANCEL");

		x = 2;
		y = 5;

		if(ROM_SRAM)
			pspPrint(x,y++,setting.color[3],"  SRAM SAVE");
		else
			pspPrint(x,y++,setting.color[0],"  SRAM SAVE");
		y++;

		if(setting.cpuclock==0)
			pspPrint(x,y++,0xffff,"  CPU CLOCK: 222");
		else if(setting.cpuclock==1)
			pspPrint(x,y++,0xffff,"  CPU CLOCK: 266");
		else if(setting.cpuclock==2)
			pspPrint(x,y++,0xffff,"  CPU CLOCK: 300");
		else if(setting.cpuclock==3)
			pspPrint(x,y++,0xffff,"  CPU CLOCK: 333");
		y++;
			
		if(setting.screensize)
			pspPrint(x,y++,setting.color[3],"  SCREEN SIZE: FULL");
		else
			pspPrint(x,y++,setting.color[3],"  SCREEN SIZE: x1");
			
		if(setting.frameskip==0)
			pspPrint(x,y++,0xffff,"  FRAMESKIP: AUTO");
		else if(setting.frameskip==1)
			pspPrint(x,y++,0xffff,"  FRAMESKIP: 0");
		else if(setting.frameskip==2)
			pspPrint(x,y++,0xffff,"  FRAMESKIP: 1");
		else if(setting.frameskip==3)
			pspPrint(x,y++,0xffff,"  FRAMESKIP: 2");
		else if(setting.frameskip==4)
			pspPrint(x,y++,0xffff,"  FRAMESKIP: 3");
		else if(setting.frameskip==5)
			pspPrint(x,y++,0xffff,"  FRAMESKIP: 4");
		else if(setting.frameskip==6)
			pspPrint(x,y++,0xffff,"  FRAMESKIP: 5");

		if(setting.vsync)
			pspPrint(x,y++,setting.color[3],"  VSYNC:ON");
		else
			pspPrint(x,y++,setting.color[3],"  VSYNC:OFF");
		y++;
		
		if(setting.sound)
			pspPrint(x,y++,setting.color[3],"  SOUND:ON");
		else
			pspPrint(x,y++,setting.color[3],"  SOUND:OFF");
		y++;
		
		pspPrint(x,y++,setting.color[3],"  COLOR CONFIG");
		pspPrint(x,y++,setting.color[3],"  KEY CONFIG");
		y++;
		
		pspPrint(x,y++,setting.color[3],"  Reset");
		y++;
		
		pspPrint(x,y++,setting.color[3],"  Back to ROM list");
		y++;
		
		pspPrint(x,y++,setting.color[3],"  Continue");

		y = sel + 5;
		if(sel >= CPU_CLOCK)	y++;
		if(sel >= SCREEN_SIZE)	y++;
		if(sel >= SOUND)		y++;
		if(sel >= COLOR_CONFIG)	y++;
		if(sel >= RESET)		y++;
		if(sel >= LOAD_ROM)		y++;
		if(sel >= CONTINUE)		y++;

		pspPutChar((x+1)*8,y*8,setting.color[3],0,127,1,0,1);

		pspScreenFlipV();
	}

	save_config();

	pspFillVRAM(0);
	pspScreenFlipV();
	pspFillVRAM(0);
	pspScreenFlipV();
	pspWaitVn(10);
	memset(&psp_PadData, 0x00, sizeof(psp_PadData));
}
