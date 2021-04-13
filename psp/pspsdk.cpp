#include "pspsdk.h"


////////////////////////////////////////////////////////////////////////////////
//// Graphics
#define		PIXELSIZE	1				//in short
#define		LINESIZE	512				//in short
#define		FRAMESIZE	0x44000			//in byte

//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 34

long psp_screenmode;
long psp_showframe;
long psp_drawframe;
unsigned long psp_csr_x[2];
unsigned long psp_csr_y[2];
unsigned long psp_fgcolor[2];
unsigned long psp_bgcolor[2];
char psp_fgdraw[2];
char psp_bgdraw[2];
char psp_mag[2];

//FONTデータインクルード
#include "font.c"
#include "fontNaga10.c"

char *pspGetVramAddr(unsigned long x,unsigned long y)
{
	return (VRAM_ADDR+(psp_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2+0x40000000);
}

void pspWaitVn(unsigned long count)
{
	for (; count>0; --count) {
		sceDisplayWaitVblankStart();
	}
}

void pspWaitV()
{
	sceDisplayWaitVblankStart();
//	sceDisplayWaitVblank();
}

void pspFillVRAM(unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;

	vptr0=(unsigned char *)pspGetVramAddr(0,0);
	for (i=0; i<FRAMESIZE/2; i++) {
		*(unsigned short *)vptr0=color;
		vptr0+=PIXELSIZE*2;
	}
}

void pspFillBox(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i, j;

	vptr0=(unsigned char *)pspGetVramAddr(0,0);
	for(i=y1; i<=y2; i++){
		for(j=x1; j<=x2; j++){
			((unsigned short *)vptr0)[j*PIXELSIZE + i*LINESIZE] = color;
		}
	}
}

void pspDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned long i;

	vptr0=(unsigned char *)pspGetVramAddr(0,0);
	for(i=x1; i<=x2; i++){
		((unsigned short *)vptr0)[i*PIXELSIZE + y1*LINESIZE] = color;
		((unsigned short *)vptr0)[i*PIXELSIZE + y2*LINESIZE] = color;
	}
	for(i=y1; i<=y2; i++){
		((unsigned short *)vptr0)[x1*PIXELSIZE + i*LINESIZE] = color;
		((unsigned short *)vptr0)[x2*PIXELSIZE + i*LINESIZE] = color;
	}
}

void pspBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	unsigned long xx,yy,mx,my;
	const unsigned short *dd;

	vptr0=(unsigned char *)pspGetVramAddr(x,y);
	for (yy=0; yy<h; yy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			dd=d;
			for (xx=0; xx<w; xx++) {
				for (mx=0; mx<mag; mx++) {
					*(unsigned short *)vptr=*dd;
					vptr+=PIXELSIZE*2;
				}
				dd++;
			}
			vptr0+=LINESIZE*2;
		}
		d+=w;
	}
}

void pspBitBltFull(unsigned long w,unsigned long h,const unsigned short *d)
{	
	int a,b;
	unsigned short *vr;
	unsigned short *v;
	unsigned long xx,yy;
	
	v  = (unsigned short *)d;
	vr = (unsigned short *)pspGetVramAddr(0,0);
	
	for (yy=0;yy<272;yy++)
	{
		b=yy*h/272;
		for (xx=0;xx<480;xx++)
		{
			a=xx*w/480;
			*vr++ = *(d+b*w+a);
		}
		vr += 32;
	}
}

//NesterJ for PSP より
//Parallel blend
static inline unsigned long PBlend(unsigned long c0, unsigned long c1)
{
	return (c0 & c1) + (((c0 ^ c1) & 0x7bde7bde) >> 1);
}
#define __USE_MIPS32R2__
static inline void cpy2x(unsigned long *d, unsigned long cc)
{
#ifdef __USE_MIPS32R2__
	unsigned long wk0;
	asm volatile (
		"		.set	push"				"\n"
		"		.set	noreorder"			"\n"

		"		.set	mips32r2"			"\n"

		"		srl		%0,%2,16"			"\n"
		"		ins 	%2,%2,16,16"		"\n"
		"		sw		%2,0(%1)"			"\n"
		"		ins 	%0,%0,16,16"		"\n"
		"		sw		%0,4(%1)"			"\n"

		"		.set	pop"				"\n"

			:	"=&r" (wk0)		// %0
			:	"r" (d),		// %1
				"r" (cc)		// %2
	);
#else
	*d      = (cc&0x0000ffff)|(cc<<16);
	*(d+1)  = (cc>>16)|(cc&0xffff0000);
#endif
}
//両端8ドットって拡大
void pspBitBltFullScreen(unsigned short *pbyRgbBuf)
{
	int x,y, dy = 0;
	unsigned long *pVram;
	pVram = (unsigned long *)pspGetVramAddr(0, 0);
	for (y = 0; y < 224; y++) {
		for (x = 0; x < 240; x+=2) {
			cpy2x(&pVram[x], (pbyRgbBuf[256*(y + 8)+(x + 8+1)] << 16) + pbyRgbBuf[256*(y + 8)+(x + 8)]);
		}
		pVram += (LINESIZE/2);
		dy += 21429;
		if (dy >= 100000) {
			dy-=100000;
			for (x = 0; x < 240; x+=2) {
				unsigned long ulColor = PBlend((pbyRgbBuf[256*(y + 8)+(x + 8+1)] << 16) + pbyRgbBuf[256*(y + 8)+(x + 8)],
									         (pbyRgbBuf[256*(y + 8)+(x + 8+1)] << 16) + pbyRgbBuf[256*(y + 8)+(x + 8)]);
				cpy2x(&pVram[x], ulColor);
			}
			pVram += (LINESIZE/2);
		}
	}
}

void pspBitBltN1(unsigned long x,unsigned long y,unsigned long w,unsigned long h, signed long *d)
{
	unsigned long *v0;		//pointer to vram
	unsigned long yy;

	v0=(unsigned long *)pspGetVramAddr(x,y);
	for (yy=0; yy<h; yy++) {
		__memcpy4a(v0,(unsigned long *)d,w/2);
		v0+=(LINESIZE/2-w/2);
	}
}

void Draw_Char_Hankaku(int x,int y,const unsigned char c,int col) {
	unsigned short *vr;
	unsigned char  *fnt;
	unsigned char  pt;
	unsigned char ch;
	int x1,y1;

	ch = c;

	// mapping
	if (ch<0x20)
		ch = 0;
	else if (ch<0x80)
		ch -= 0x20;
	else if (ch<0xa0)
		ch = 0;
	else
		ch -= 0x40;

	fnt = (unsigned char *)&hankaku_font10[ch*10];

	// draw
	vr = (unsigned short *)pspGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		for(x1=0;x1<5;x1++) {
			if (pt & 1)
				*vr = col;
			vr++;
			pt = pt >> 1;
		}
		vr += LINESIZE-5;
	}
}

void Draw_Char_Zenkaku(int x,int y,const unsigned char u,unsigned char d,int col) {
	// ELISA100.FNTに存在しない文字
	const unsigned short font404[] = {
		0xA2AF, 11,
		0xA2C2, 8,
		0xA2D1, 11,
		0xA2EB, 7,
		0xA2FA, 4,
		0xA3A1, 15,
		0xA3BA, 7,
		0xA3DB, 6,
		0xA3FB, 4,
		0xA4F4, 11,
		0xA5F7, 8,
		0xA6B9, 8,
		0xA6D9, 38,
		0xA7C2, 15,
		0xA7F2, 13,
		0xA8C1, 720,
		0xCFD4, 43,
		0xF4A5, 1030,
		0,0
	};
	unsigned short *vr;
	unsigned short *fnt;
	unsigned short pt;
	int x1,y1;

	int n;
	unsigned short code;
	int i, j;

	// SJISコードの生成
	code = u;
	code = (code<<8) + d;

	// SJISからEUCに変換
	if(code >= 0xE000) code-=0x4000;
	code = ((((code>>8)&0xFF)-0x81)<<9) + (code&0x00FF);
	if((code & 0x00FF) >= 0x80) code--;
	if((code & 0x00FF) >= 0x9E) code+=0x62;
	else code-=0x40;
	code += 0x2121 + 0x8080;

	// EUCから恵梨沙フォントの番号を生成
	n = (((code>>8)&0xFF)-0xA1)*(0xFF-0xA1)
		+ (code&0xFF)-0xA1;
	j=0;
	while(font404[j]) {
		if(code >= font404[j]) {
			if(code <= font404[j]+font404[j+1]-1) {
				n = -1;
				break;
			} else {
				n-=font404[j+1];
			}
		}
		j+=2;
	}
	fnt = (unsigned short *)&zenkaku_font10[n*10];

	// draw
	vr = (unsigned short *)pspGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		for(x1=0;x1<10;x1++) {
			if (pt & 1)
				*vr = col;
			vr++;
			pt = pt >> 1;
		}
		vr += LINESIZE-10;
	}
}

void pspTextOut(int x,int y,int col,const unsigned char *str) 
{
	unsigned char ch = 0,bef = 0;

	while(*str != 0) {
		ch = *str++;
		if (bef!=0) {
			Draw_Char_Zenkaku(x,y,bef,ch,col);
			x+=10;
			bef=0;
		} else {
			if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
				bef = ch;
			} else {
				Draw_Char_Hankaku(x,y,ch,col);
				x+=5;
			}
		}
	}
}

void pspPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	const unsigned char *cfont;		//pointer to font
	unsigned long cx,cy;
	unsigned long b;
	char mx,my;

	cfont=font+ch*8;
	vptr0=(unsigned char *)pspGetVramAddr(x,y);
	for (cy=0; cy<8; cy++) {
		for (my=0; my<mag; my++) {
			vptr=vptr0;
			b=0x80;
			for (cx=0; cx<8; cx++) {
				for (mx=0; mx<mag; mx++) {
					if ((*cfont&b)!=0) {
						if (drawfg) *(unsigned short *)vptr=color;
					} else {
						if (drawbg) *(unsigned short *)vptr=bgcolor;
					}
					vptr+=PIXELSIZE*2;
				}
				b=b>>1;
			}
			vptr0+=LINESIZE*2;
		}
		cfont++;
	}
}

void pspPrint(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
		pspPutChar(x*8,y*8,color,0,*str,1,0,1);
		str++;
		x++;
		if (x>=CMAX_X) {
			x=0;
			y++;
		}
	}
}

void pspScreenFlip()
{
	psp_showframe=(psp_showframe?0:1);
	psp_drawframe=(psp_drawframe?0:1);
	sceDisplaySetFrameBuf(VRAM_ADDR+(psp_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
}

void pspScreenFlipV()
{
	pspWaitV();
	pspScreenFlip();
}

void pspScreenFrame(long mode,long frame)
{
	psp_screenmode=mode;
	frame=(frame?1:0);
	psp_showframe=frame;
	if (mode==0) {
		//screen off
		psp_drawframe=frame;
		sceDisplaySetFrameBuf(0,0,0,1);
	} else if (mode==1) {
		//show/draw same
		psp_drawframe=frame;
		sceDisplaySetFrameBuf(VRAM_ADDR+(psp_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	} else if (mode==2) {
		//show/draw different
		psp_drawframe=(frame?0:1);
		sceDisplaySetFrameBuf(VRAM_ADDR+(psp_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
	}
}

void pspLocate(unsigned long x, unsigned long y)
{
	if (x>=CMAX_X) x=0;
	if (y>=CMAX_Y) y=0;
	psp_csr_x[psp_drawframe?1:0]=x;
	psp_csr_y[psp_drawframe?1:0]=y;
}

void pspColor(unsigned long fg, unsigned long bg)
{
	psp_fgcolor[psp_drawframe?1:0]=fg;
	psp_bgcolor[psp_drawframe?1:0]=bg;
}

void pspDraw(char drawfg, char drawbg)
{
	psp_fgdraw[psp_drawframe?1:0]=drawfg;
	psp_bgdraw[psp_drawframe?1:0]=drawbg;
}

void pspSetmag(char mag)
{
	psp_mag[psp_drawframe?1:0]=mag;
}

void pspCls()
{
	pspFillVRAM(psp_bgcolor[psp_drawframe]);
	pspLocate(0,0);
}

void pspPutchar_nocontrol(const char ch)
{
	pspPutChar(psp_csr_x  [psp_drawframe]*8, psp_csr_y  [psp_drawframe]*8, 
	           psp_fgcolor[psp_drawframe],   psp_bgcolor[psp_drawframe]  , ch, 
	           psp_fgdraw [psp_drawframe],   psp_bgdraw [psp_drawframe]  , 
	           psp_mag[psp_drawframe]);
	          
	psp_csr_x[psp_drawframe]+=psp_mag[psp_drawframe];
	if (psp_csr_x[psp_drawframe]>CMAX_X-psp_mag[psp_drawframe]) {
		psp_csr_x[psp_drawframe]=0;
		psp_csr_y[psp_drawframe]+=psp_mag[psp_drawframe];
		if (psp_csr_y[psp_drawframe]>CMAX_Y-psp_mag[psp_drawframe]) {
			psp_csr_y[psp_drawframe]=CMAX_Y-psp_mag[psp_drawframe];
		}
	}
}

void pspPutchar(const char ch)
{
	if (ch==0x0d) {
		psp_csr_x[psp_drawframe]=0;
		return;
	}
	if (ch==0x0a) {
		if ((++psp_csr_y[psp_drawframe])>=CMAX_Y) {
			psp_csr_y[psp_drawframe]=CMAX_Y-1;
		}
		return;
	}
	pspPutchar_nocontrol(ch);
}

void pspPuthex2(const unsigned long s)
{
	char ch;
	ch=((s>>4)&0x0f);
	pspPutchar((ch<10)?(ch+0x30):(ch+0x40-9));
	ch=(s&0x0f);
	pspPutchar((ch<10)?(ch+0x30):(ch+0x40-9));
}

void pspPuthex8(const unsigned long s)
{
	pspPuthex2(s>>24);
	pspPuthex2(s>>16);
	pspPuthex2(s>>8);
	pspPuthex2(s);
}

void pspGraphicsInit()
{
	sceDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);

	pspScreenFrame(0,1);
	pspLocate(0,0);
	pspColor(0xffff,0x0000);
	pspDraw(1,1);
	pspSetmag(1);

	pspScreenFrame(0,0);
	pspLocate(0,0);
	pspColor(0xffff,0x0000);
	pspDraw(1,1);
	pspSetmag(1);
	
	pspScreenFrame(2,0);
}

////////////////////////////////////////////////////////////////////////////////
//// Input
PSP_CtrlData psp_PadData;
u32 new_pad;
u32 old_pad;
u32 now_pad;


void pspInputInit()
{
	sceCtrlSetSamplingCycle(0);
}

void pspInputAnalog(int flag)
{
	sceCtrlSetSamplingMode(flag);
}

void pspReadPad(void)
{
	static int n=0;

	sceCtrlReadBufferPositive(&psp_PadData, 1);

	now_pad = psp_PadData.Buttons;
	new_pad = now_pad & ~old_pad;
	if(old_pad==now_pad){
		n++;
		if(n>=25){
			new_pad=now_pad;
			n = 20;
		}
	}else{
		n=0;
		old_pad = now_pad;
	}
}

////////////////////////////////////////////////////////////////////////////////
//// Audio
#define PSP_CHANNELS 1	//3
#define PSP_SAMPLES 512	//256
#define MAXVOLUME 0x8000
#define SOUND_BANKLEN 2048

int		psp_ready=0;
int		psp_handle[PSP_CHANNELS];
short	psp_sndbuf[PSP_CHANNELS][2][PSP_SAMPLES][2];
void	(*psp_channel_callback[PSP_CHANNELS])(void *buf, unsigned long reqn);
int		psp_threadhandle[PSP_CHANNELS];
volatile int psp_terminate=0;

unsigned long cur_play=0;
unsigned long buf_pos = 0;
int wavout_enable=0;
short sound_buf[SOUND_BANKLEN*4*2];

//44100,chan:2固定
static void wavout_snd0_callback(short *buf, unsigned long reqn)
{
	if (!wavout_enable) {
		memset(buf,0,reqn*4);
		return;
	}
/*
	unsigned int ptr=cur_play;
	unsigned int nextptr=ptr+reqn;
	if (nextptr>=SOUND_BANKLEN*4) nextptr=0;
	__memcpy4a((unsigned long *)buf, (unsigned long *)&sound_buf[ptr*2], reqn);
	cur_play=nextptr;
*/
	int i;
	
	if ( (cur_play <= buf_pos) && (buf_pos<=(cur_play+reqn*2) ) ) return;
	for(i=0;i<reqn;i++)
	{
		buf[i*2]  =sound_buf[cur_play];
		buf[i*2+1]=sound_buf[cur_play+1];
		cur_play += 2;
		if (cur_play >= SOUND_BUFLEN) cur_play=0;
	}

}

int pspAudioOutBlocking(unsigned long channel,unsigned long vol1,unsigned long vol2,void *buf)
{
//バッファは64バイト境界じゃなくても大丈夫みたい
//[0]が左、[1]が右
//サンプル速度は44100
//vol1が左
	if (!psp_ready) return -1;
	if (channel>=PSP_CHANNELS) return -1;
	if (vol1>MAXVOLUME) vol1=MAXVOLUME;
	if (vol2>MAXVOLUME) vol2=MAXVOLUME;
	
	return sceAudioOutputPannedBlocking(psp_handle[channel],vol1,vol2,buf);
}

static int psp_channel_thread(unsigned int args, void *argp)
{
	volatile int bufidx=0;
	int channel=*(int *)argp;

	while (psp_terminate==0) {
		void *bufptr=&psp_sndbuf[channel][bufidx];
		void (*callback)(void *buf, unsigned long reqn);
		callback=psp_channel_callback[channel];
		if (callback) {
			callback(bufptr,PSP_SAMPLES);
		} else {
			unsigned long *ptr=(unsigned long *)bufptr;
			int i;
			for (i=0; i<PSP_SAMPLES; ++i) *(ptr++)=0;
		}
		pspAudioOutBlocking(channel,0x8000,0x8000,bufptr);
		bufidx=(bufidx?0:1);
	}
	sceKernelExitThread(0);
	return 0;
}

void pspSetChannelCallback(int channel, void *callback)
{
	psp_channel_callback[channel] = (void(*)(void*, unsigned long ))callback;
}

int pspAudioInit()
{
	int i,ret;
	int failed=0;
	char str[32];

	psp_terminate=0;
	psp_ready=0;

	for (i=0; i<PSP_CHANNELS; i++) {
		psp_handle[i]=-1;
		psp_threadhandle[i]=-1;
		psp_channel_callback[i]=0;
	}
	for (i=0; i<PSP_CHANNELS; i++) {
		if ((psp_handle[i]=sceAudioChReserve(-1,PSP_SAMPLES,0))<0) failed=1;
	}
	if (failed) {
		for (i=0; i<PSP_CHANNELS; i++) {
			if (psp_handle[i]!=-1) sceAudioChRelease(psp_handle[i]);
			psp_handle[i]=-1;
		}
		return -1;
	}
	psp_ready=1;

	strcpy(str,"pspsnd0");
	for (i=0; i<PSP_CHANNELS; i++) {
		str[6]='0'+i;
		psp_threadhandle[i]=sceKernelCreateThread(str,psp_channel_thread,0x12,0x10000,0,0);
		if (psp_threadhandle[i]<0) {
			psp_threadhandle[i]=-1;
			failed=1;
			break;
		}
		ret=sceKernelStartThread(psp_threadhandle[i],sizeof(i),&i);
		if (ret!=0) {
			failed=1;
			break;
		}
	}
	if (failed) {
		psp_terminate=1;
		for (i=0; i<PSP_CHANNELS; i++) {
			if (psp_threadhandle[i]!=-1) {
				sceKernelWaitThreadEnd(psp_threadhandle[i],0);
				sceKernelDeleteThread(psp_threadhandle[i]);
			}
			psp_threadhandle[i]=-1;
		}
		psp_ready=0;
		return -1;
	}
	
	wavout_enable=0;
	pspSetChannelCallback(0,(void *)wavout_snd0_callback);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//// System
int  psp_bExit=0;
char psp_szCurrentPath[MAX_PATH];

// ホームボタン終了時にコールバック
int ExitCallback(int count, int arg, void *common)
{ 
	psp_bExit=1;

	pspExit();
	
	// Exit game 
	sceKernelExitGame(); 

	return 0;
}

// スリープ時や不定期にコールバック
int PowerCallback(int count, int pwrflags, void *common) 
{ 
	int cbid;

	// スリープを含むパワースイッチOFF時だけ各種保存処理
	if(pwrflags & POWER_CB_POWERSW){
		psp_bExit=1;
		pspExit();
	}

	// コールバック関数の再登録
	cbid = sceKernelCreateCallback("Power Callback", PowerCallback, 0);
	scePowerRegisterCallback(0, cbid);
} 

// Thread to create the callbacks and then begin polling 
int pspCallbackThread(unsigned int argc, void *arg) 
{ 
	int cbid; 

	cbid = sceKernelCreateCallback("Exit Callback", ExitCallback, 0); 
	sceKernelRegisterExitCallback(cbid); 
	cbid = sceKernelCreateCallback("Power Callback", PowerCallback, 0); 
	scePowerRegisterCallback(0, cbid); 

	sceKernelSleepThreadCB(); 

	return 0;
} 

/* Sets up the callback thread and returns its thread id */ 
int pspSetupCallbacks(void) 
{ 
	int thid = 0; 

	thid = sceKernelCreateThread("update_thread", pspCallbackThread, 0x11, 0xFA0, 0, 0); 
	if(thid >= 0) 
	{ 
		sceKernelStartThread(thid, 0, 0); 
	} 

	return thid; 
} 

////////////////////////////////////////////////////////////////////////////////
//// C Runtime
int memcmp(const void *buf1, const void *buf2,int n)
{
	int ret;
	int i;
	
	for(i=0; i<n; i++){
		ret = ((unsigned char*)buf1)[i] - ((unsigned char*)buf2)[i];
		if(ret!=0)
			return ret;
	}
	return 0;
}

void* memcpy(void *buf1, const void *buf2, int n)
{
	while(n-->0)
		((unsigned char*)buf1)[n] = ((unsigned char*)buf2)[n];
	return buf1;
}

void* memset(void *buf, int ch, int n)
{
	unsigned char *p = (unsigned char *)buf;
	
	while(n>0)
		p[--n] = ch;
	
	return buf;
}

char* strcat(char *dest, const char *src)
{
	int i;
	int len;
	
	len=strlen(dest);
	for(i=0; src[i]; i++)
		dest[len+i] = src[i];
	dest[len+i] = 0;
	
	return dest;
}

int strcmp(const char *str1, const char *str2)
{
	char c1, c2;
	for(;;){
		c1 = *str1;
		c2 = *str2;
		
		if(c1!=c2)
			return 1;
		else if(c1==0)
			return 0;
		
		str1++; str2++;
	}
}

char* strcpy(char *dest, const char *src)
{
	int i;
	
	for(i=0; src[i]; i++)
		dest[i] = src[i];
	dest[i] = 0;
	
	return dest;
}

int stricmp(const char *str1, const char *str2)
{
	char c1, c2;
	for(;;){
		c1 = *str1;
		if(c1>=0x61 && c1<=0x7A) c1-=0x20;
		c2 = *str2;
		if(c2>=0x61 && c2<=0x7A) c2-=0x20;
		
		if(c1!=c2)
			return 1;
		else if(c1==0)
			return 0;
		
		str1++; str2++;
	}
}

int strlen(const char *s)
{
	int ret;
	
	for(ret=0; s[ret]; ret++)
		;
	
	return ret;
}

char* strncat(char *s1,char *s2,int n) 
{
	char *rt = s1;

	while((*s1!=0) && ((n-1)>0)) {
		s1++;
		n--;
	}

	while((*s2!=0) && ((n-1)>0)) {
		*s1 = *s2;
		s1++;
		s2++;
		n--;
	}
	*s1 = 0;

	return rt;
}

char* strncpy(char *s1,char *s2,int n) {
	char *rt = s1;

	while((*s2!=0) && ((n-1)>0)) {
		*s1 = *s2;
		s1++;
		s2++;
		n--;
	}
	*s1 = 0;

	return rt;
}

char* strrchr(const char *src, int c)
{
	int len;
	
	len=strlen(src);
	while(len>0){
		len--;
		if(*(src+len) == c)
			return (char*)(src+len);
	}
	
	return 0;
}

void _strrev(char *s){
	char tmp;
	int i;
	int len = strlen(s);
	
	for(i=0; i<len/2; i++){
		tmp = s[i];
		s[i] = s[len-1-i];
		s[len-1-i] = tmp;
	}
}

void itoa(int val, char *s) {
	char *t;
	int mod;

	if(val < 0) {
		*s++ = '-';
		val = -val;
	}
	t = s;

	while(val) {
		mod = val % 10;
		*t++ = (char)mod + '0';
		val /= 10;
	}

	if(s == t)
		*t++ = '0';

	*t = '\0';

	_strrev(s);
}

//long配列をコピー。配列境界は4バイトアラインされている必要あり
//s,dは参照渡し扱いになるので、リターン後は変更されていると考えたほうが良い
//コンパイラの最適化によって予期しないコードが生成されるため、十分に注意のこと。__memcpy4のほうが安全。
//といいますかcで書いても全然変わらないような。ｶﾞﾝｶﾞｯﾀのに。
static inline void __memcpy4a(unsigned long *d, unsigned long *s, unsigned long c)
{
	unsigned long wk,counter;

	asm volatile (
		"		.set	push"				"\n"
		"		.set	noreorder"			"\n"

		"		move	%1,%4 "				"\n"
		"1:		lw		%0,0(%3) "			"\n"
		"		addiu	%1,%1,-1 "			"\n"
		"		addiu	%3,%3,4 "			"\n"
		"		sw		%0,0(%2) "			"\n"
		"		bnez	%1,1b "				"\n"
		"		addiu	%2,%2,4 "			"\n"

		"		.set	pop"				"\n"

			:	"=&r" (wk),		// %0
				"=&r" (counter)	// %1
			:	"r" (d),		// %2
				"r" (s),		// %3
				"r" (c)			// %4
	);
}

////////////////////////////////////////////////////////////////////////////////
//// StartUp
int xmain(int argc, char *argv)
{
	// カレントパス設定
#ifdef PSPE
	strcpy(psp_szCurrentPath, "ms0:/PSP/GAME/TEST/");
#else
	char *p;
	
	strcpy(psp_szCurrentPath, argv);
	p = strrchr(psp_szCurrentPath, '/');
	*++p = 0;
#endif
	
	// 電源処理のコールバック設定
	pspSetupCallbacks();

	// 初期化処理
	pspGraphicsInit();
	pspAudioInit();
	pspInputInit();
		
	// アプリケーション呼び出し
	pspMain();
	
	// 終了処理
	
	return 0;
}




