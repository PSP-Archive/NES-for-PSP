/*===================================================================*/
/*                                                                   */
/*  InfoNES_System_PSP.c : PSP specific File                         */
/*                                                                   */
/*  2001/05/18  InfoNES Project                                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/
#include "../InfoNES.h"
#include "../InfoNES_System.h"
#include "../InfoNES_pAPU.h"

#include "pspsdk.h"
#include "filer.h"
#include "menu.h"

/*-------------------------------------------------------------------*/
/*  ROM image file information                                       */
/*-------------------------------------------------------------------*/
char szRomName[ 256 ];
char szSaveName[ 256 ];
int nSRAM_SaveFlag;

/*-------------------------------------------------------------------*/
/*  Global Variables                                                 */
/*-------------------------------------------------------------------*/

extern unsigned long buf_pos;

int bThread;

/* Pad state */
DWORD dwKeyPad1;
DWORD dwKeyPad2;
DWORD dwKeySystem;
int rapid_state = 0;

DWORD fps=0;
DWORD fpsc=0;
DWORD nowtime=0;
DWORD lasttime=0;

/*-------------------------------------------------------------------*/
/*  Function prototypes                                              */
/*-------------------------------------------------------------------*/

int LoadSRAM();
int SaveSRAM();

/* Palette data */
WORD NesPalette[ 64 ] =
{
  0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
  0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
  0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
  0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
  0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
  0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
};

void convNesPalette()
{
	int i;
	unsigned short wPl, wR, wG, wB;

	for (i=0; i <64; i++)
	{
		wPl =NesPalette[i];
		wR =(wPl >>10) & 0x1F;
		wG =(wPl >>5 ) & 0x1F;
		wB =(wPl     ) & 0x1F;
		NesPalette[i] =(wB <<10) +(wG <<5) +(wR);
	}
}

/*===================================================================*/
/*                                                                   */
/*                main() : Application main                          */
/*                                                                   */
/*===================================================================*/

/* Application main */
int pspMain()
{                                  
	char *p;

	strcpy(LastPath,psp_szCurrentPath);
	FilerMsg[0]=0;
	
	pspScreenFrame(2,0);
	
	convNesPalette();
	
	load_menu_bg();
	
	load_config();
	
#ifndef PSPE
	if(setting.cpuclock==0)
		scePowerSetClockFrequency(222,222,111);
	else if(setting.cpuclock==1)
		scePowerSetClockFrequency(266,266,133);
	else if(setting.cpuclock==2)
		scePowerSetClockFrequency(300,300,150);
	else if(setting.cpuclock==3)
		scePowerSetClockFrequency(333,333,166);
#endif

	for(;;)
	{
		while(!getFilePath(szRomName)){}
				
		strcpy(szSaveName, szRomName);
		p = strrchr(szSaveName, '.');
		strcpy(p, ".srm");
	
		/* Load cassette */
		if ( InfoNES_Load ( szRomName ) != 0 ) continue;

		if(setting.frameskip)
			FrameSkip = setting.frameskip-1;
		
		APU_Mute = !setting.sound;
		
		if(setting.sound) wavout_enable=1;
			
		/* Load SRAM */
		LoadSRAM();
		
		break;
	}
	
	pspFillVRAM(0);
	pspScreenFlipV();
	pspFillVRAM(0);
	pspScreenFlipV();

	/*-------------------------------------------------------------------*/
	/*  Pad Control                                                      */
	/*-------------------------------------------------------------------*/

	/* Initialize a pad state */
	dwKeyPad1   = 0;
	dwKeyPad2   = 0;
	dwKeySystem = 0;

	/*-------------------------------------------------------------------*/
	/*  Load Cassette & Create Thread                                    */
	/*-------------------------------------------------------------------*/

	/* Initialize thread state */
	bThread = -1;
	
	InfoNES_Main();
	
#ifndef PSPE	
	scePowerSetClockFrequency(222,222,111);
#endif
	
	return(0);
}

int pspExit()
{
#ifndef PSPE	
	scePowerSetClockFrequency(222,222,111);
#endif
	
	/* Terminate emulation thread */
	bThread = 0;
	
	wavout_enable=0;
	
	dwKeySystem |= PAD_SYS_QUIT; 
	
	/* Save SRAM*/
	SaveSRAM();

	return 0;
}

/*===================================================================*/
/*                                                                   */
/*           LoadSRAM() : Load a SRAM                                */
/*                                                                   */
/*===================================================================*/
int LoadSRAM()
{
	int fd;
	unsigned char pSrcBuf[ SRAM_SIZE ];
	unsigned char chData;
	unsigned char chTag;
	int nRunLen;
	int nDecoded;
	int nDecLen;
	int nIdx;

	// It doesn't need to save it
	nSRAM_SaveFlag = 0;

	// It is finished if the ROM doesn't have SRAM
	if ( !ROM_SRAM ) return 0;
		
	// There is necessity to save it
	nSRAM_SaveFlag = 1;

	/*-------------------------------------------------------------------*/
	/*  Read a SRAM data                                                 */
	/*-------------------------------------------------------------------*/
	// Open SRAM file
	if((fd=sceIoOpen(szSaveName, O_RDONLY,0777))<0) return -1;

	// Read SRAM data
	sceIoRead(fd, pSrcBuf, SRAM_SIZE);
	// Close SRAM file
	sceIoClose(fd);
	
	/*-------------------------------------------------------------------*/
	/*  Extract a SRAM data                                              */
	/*-------------------------------------------------------------------*/
	nDecoded = 0;
	nDecLen = 0;

	chTag = pSrcBuf[ nDecoded++ ];

	while ( nDecLen < 8192 )
	{
		chData = pSrcBuf[ nDecoded++ ];

		if ( chData == chTag )
		{
			chData = pSrcBuf[ nDecoded++ ];
			nRunLen = pSrcBuf[ nDecoded++ ];
			for ( nIdx = 0; nIdx < nRunLen + 1; ++nIdx )
			{
				SRAM[ nDecLen++ ] = chData;
			}
		}
		else
		{
			SRAM[ nDecLen++ ] = chData;
		}
	}

	return 0;
}

/*===================================================================*/
/*                                                                   */
/*           SaveSRAM() : Save a SRAM                                */
/*                                                                   */
/*===================================================================*/
int SaveSRAM()
{
	int fd;
	int nUsedTable[ 256 ];
	unsigned char chData;
	unsigned char chPrevData;
	unsigned char chTag;
	int nIdx;
	int nEncoded;
	int nEncLen;
	int nRunLen;
	unsigned char pDstBuf[ SRAM_SIZE ];

	if ( !nSRAM_SaveFlag ) return 0;  // It doesn't need to save it

	/*-------------------------------------------------------------------*/
	/*  Compress a SRAM data                                             */
	/*-------------------------------------------------------------------*/

	memset( nUsedTable, 0, sizeof nUsedTable );

	for ( nIdx = 0; nIdx < SRAM_SIZE; ++nIdx )
	{
		++nUsedTable[ SRAM[ nIdx++ ] ];
	}
	for ( nIdx = 1, chTag = 0; nIdx < 256; ++nIdx )
	{
		if ( nUsedTable[ nIdx ] < nUsedTable[ chTag ] )
		chTag = nIdx;
	}

	nEncoded = 0;
	nEncLen = 0;
	nRunLen = 1;

	pDstBuf[ nEncLen++ ] = chTag;

	chPrevData = SRAM[ nEncoded++ ];

	while ( nEncoded < SRAM_SIZE && nEncLen < SRAM_SIZE - 133 )
	{
		chData = SRAM[ nEncoded++ ];

		if ( chPrevData == chData && nRunLen < 256 )
			++nRunLen;
		else
		{
			if ( nRunLen >= 4 || chPrevData == chTag )
			{
				pDstBuf[ nEncLen++ ] = chTag;
				pDstBuf[ nEncLen++ ] = chPrevData;
				pDstBuf[ nEncLen++ ] = nRunLen - 1;
			}
			else
			{
				for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
					pDstBuf[ nEncLen++ ] = chPrevData;
			}

			chPrevData = chData;
			nRunLen = 1;
		}

	}
	if ( nRunLen >= 4 || chPrevData == chTag )
	{
		pDstBuf[ nEncLen++ ] = chTag;
		pDstBuf[ nEncLen++ ] = chPrevData;
		pDstBuf[ nEncLen++ ] = nRunLen - 1;
	}
	else
	{
		for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
			pDstBuf[ nEncLen++ ] = chPrevData;
	}

	/*-------------------------------------------------------------------*/
	/*  Write a SRAM data                                                */
	/*-------------------------------------------------------------------*/

	// Open SRAM file
	if((fd=sceIoOpen(szSaveName, O_CREAT|O_RDWR|O_TRUNC, 0777))<0) return -1;

	// Write SRAM data
	sceIoWrite(fd, pDstBuf, nEncLen);

	// Close SRAM file
	sceIoClose(fd);
	
	// Successful
	return 0;
}

/*===================================================================*/
/*                                                                   */
/*                  InfoNES_Menu() : Menu screen                     */
/*                                                                   */
/*===================================================================*/
int InfoNES_Menu()
{
/*
 *  Menu screen
 *
 *  Return values
 *     0 : Normally
 *    -1 : Exit InfoNES
 */

  /* If terminated */
  if ( bThread == 0 )
  {
	dwKeySystem |= PAD_SYS_QUIT; 

    return -1;
  }

  /* Nothing to do here */
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*               InfoNES_ReadRom() : Read ROM image file             */
/*                                                                   */
/*===================================================================*/
int InfoNES_ReadRom( const char *pszFileName )
{
	int fd, romsize, ramsize;
	int ReadTr = 0;
	
	romsize = load_rom(pszFileName);
	
	memcpy(&NesHeader,&rom_image[0],sizeof(NesHeader));
	if ( memcmp( NesHeader.byID, "NES\x1a",  4) != 0 )
	{
		/* not .nes file */
		return -1;
	}
	
	/* Clear SRAM */
	memset( SRAM, 0, SRAM_SIZE );
	
	/* If trainer presents Read Triner at 0x7000-0x71ff */
	if ( NesHeader.byInfo1 & 4 )
	{
		memcpy(&SRAM[0x1000],&rom_image[sizeof(NesHeader)],512);
		ReadTr = 512;
	}	

	ROM = &rom_image[sizeof(NesHeader)+ReadTr];
	
	if ( NesHeader.byVRomSize > 0 )
	{
		/* Allocate Memory for VROM Image */
		VROM = &rom_image[sizeof(NesHeader)+ReadTr+0x4000*NesHeader.byRomSize];
	}
	
	return 0;
}

/*===================================================================*/
/*                                                                   */
/*           InfoNES_ReleaseRom() : Release a memory for ROM         */
/*                                                                   */
/*===================================================================*/
void InfoNES_ReleaseRom()
{
  if ( ROM )
  {
    ROM = NULL;
  }

  if ( VROM )
  {
    VROM = NULL;
  }
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_MemoryCopy() : memcpy                         */
/*                                                                   */
/*===================================================================*/
void *InfoNES_MemoryCopy( void *dest, const void *src, int count )
{
	memcpy(dest,src,count);
	return dest;
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_MemorySet() : memset                          */
/*                                                                   */
/*===================================================================*/
void *InfoNES_MemorySet( void *dest, int c, int count )
{
	memset(dest,c,count);
	return dest;
}

/*===================================================================*/
/*                                                                   */
/*      InfoNES_LoadFrame() :                                        */
/*           Transfer the contents of work frame on the screen       */
/*                                                                   */
/*===================================================================*/
void InfoNES_LoadFrame()
{
		
	nowtime = sceKernelLibcClock();
	fps++;
	if ((lasttime+1000000) < nowtime)
	{
		if(setting.frameskip==0)
			if (fps<29)
				FrameSkip= 30/fps;
		fpsc=fps;
		fps=0;
		lasttime=nowtime;
	}

	if (setting.screensize)
		pspBitBltFullScreen((unsigned short *)WorkFrame);
	else
		pspBitBltN1(108,16,256,240,(signed long *)WorkFrame);
	
	if(setting.vsync)
		pspScreenFlipV();
	else
		pspScreenFlip();
}

/*===================================================================*/
/*                                                                   */
/*             InfoNES_PadState() : Get a joypad state               */
/*                                                                   */
/*===================================================================*/
void InfoNES_PadState( DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem )
{
	dwKeyPad1 =0;
	
//	sceCtrlReadBufferPositive(&psp_PadData, 1);
	sceCtrlPeekBufferPositive(&psp_PadData, 1);
	
	if(psp_PadData.Buttons & setting.key_config[0])	dwKeyPad1|=1;
	if(psp_PadData.Buttons & setting.key_config[1])	dwKeyPad1|=2;
	if(psp_PadData.Buttons & setting.key_config[2]) dwKeyPad1|=1 * (rapid_state/2);
	if(psp_PadData.Buttons & setting.key_config[3]) dwKeyPad1|=2 * (rapid_state/2);
	if(psp_PadData.Buttons & setting.key_config[4])	dwKeyPad1|=4;
	if(psp_PadData.Buttons & setting.key_config[5])	dwKeyPad1|=8;
	if(psp_PadData.Buttons & CTRL_UP)				dwKeyPad1|=16;
	if(psp_PadData.Buttons & CTRL_DOWN)				dwKeyPad1|=32;
	if(psp_PadData.Buttons & CTRL_LEFT)				dwKeyPad1|=64;
	if(psp_PadData.Buttons & CTRL_RIGHT)			dwKeyPad1|=128;

	*pdwPad1   = dwKeyPad1;
	*pdwPad2   = dwKeyPad2;
	*pdwSystem = dwKeySystem;

	rapid_state = (rapid_state + 1) % 4;
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundInit() : Sound Emulation Initialize           */
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundInit( void ) 
{
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundOpen() : Sound Open                           */
/*                                                                   */
/*===================================================================*/
int InfoNES_SoundOpen( int samples_per_sync, int sample_rate ) 
{
	return 1;
}

/*===================================================================*/
/*                                                                   */
/*        InfoNES_SoundClose() : Sound Close                         */
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundClose( void ) 
{
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_SoundOutput() : Sound Output 5 Waves           */
/*                                                                   */
/*===================================================================*/
void InfoNES_SoundOutput( int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5 )
{
	//sound_bufにがんがんつんでく
	int i;
	int output;
	
	for (i=0;i<735;i++)
	{
		//マージ
		output =( wave1[i] +wave2[i] +wave3[i] +wave4[i] +wave5[i] ) /5;
		//8bitPCM→16bitPCMへ変換
		output = (output-128)<<8;
		
		//bufferにセット
		sound_buf[buf_pos]   = output;
		sound_buf[buf_pos+1] = output;
		buf_pos +=2;
		//最後までいったら最初にもどる
		if (buf_pos >= SOUND_BUFLEN) buf_pos=0;
	}
	
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_Wait() : Wait Emulation if required            */
/*                                                                   */
/*===================================================================*/
void InfoNES_Wait() 
{
	if(psp_PadData.Buttons & setting.key_config[6])
	{
		wavout_enable=0;
#ifndef PSPE
		scePowerSetClockFrequency(222,222,111);
#endif
		menu();
		
#ifndef PSPE
		// CPU Frequency変更
		if(setting.cpuclock==0)
			scePowerSetClockFrequency(222,222,111);
		else if(setting.cpuclock==1)
			scePowerSetClockFrequency(266,266,133);
		else if(setting.cpuclock==2)
			scePowerSetClockFrequency(300,300,150);
		else if(setting.cpuclock==3)
			scePowerSetClockFrequency(333,333,166);
#endif
		if(setting.sound) wavout_enable=1;
		
		lasttime = sceKernelLibcClock();
	}
	
	if ( bThread == 0 )
	{
		dwKeySystem |= PAD_SYS_QUIT; 
	}
	
}

/*===================================================================*/
/*                                                                   */
/*            InfoNES_MessageBox() : Print System Message            */
/*                                                                   */
/*===================================================================*/
void InfoNES_MessageBox( char *pszMsg, ... )
{
}
