#ifndef PSPSDK_H
#define PSPSDK_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "syscall.h"

#define RGB(r,g,b) ((((b>>3) & 0x1F)<<10)|(((g>>3) & 0x1F)<<5)|(((r>>3) & 0x1F)<<0)|0x8000)

#define		PIXELSIZE	1				//in short
#define		LINESIZE	512				//in short
#define		FRAMESIZE	0x44000			//in byte

//480*272 = 60*38
#define CMAX_X 60
#define CMAX_Y 34


////////////////////////////////////////////////////////////////////////////////
//アプリが実装
extern int pspMain(void);
extern int pspExit(void);


////////////////////////////////////////////////////////////////////////////////
//アプリへ公開
extern PSP_CtrlData psp_PadData;
extern u32 new_pad;
extern u32 now_pad;
extern char psp_szCurrentPath[MAX_PATH];

#define SOUND_BANKLEN 2048
extern int wavout_enable;
extern unsigned long cur_play;
extern short sound_buf[SOUND_BANKLEN*4*2];
//サウンドバッファ１バンクあたりの容量。４バンクで適当にラウンドロビン
//PGA_SAMPLESの倍数にすること。PGA_SAMPLESと同じだと多分ダメなので注意。 - LCK


////////////////////////////////////////////////////////////////////////////////
//// Graphics
char *pspGetVramAddr(unsigned long x,unsigned long y);
void pspGraphicsInit();
void pspWaitVn(unsigned long count);
void pspWaitV();
void pspFillVRAM(unsigned long color);
void pspFillBox(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color);
void pspDrawFrame(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color);
void pspBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d);
void pspBitBltFullScreen(unsigned short *pbyRgbBuf);
void pspBitBltN1(unsigned long x,unsigned long y,unsigned long w,unsigned long h, signed long *d);
void pspTextOut(int x,int y,int col,const unsigned char *str) ;
void pspPrint(unsigned long x,unsigned long y,unsigned long color,const char *str);
void pspPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag);
void pspScreenFlip();
void pspScreenFlipV();
void pspScreenFrame(long mode,long frame);
void pspLocate(unsigned long x, unsigned long y);
void pspColor(unsigned long fg, unsigned long bg);
void pspDraw(char drawfg, char drawbg);
void pspSetmag(char mag);
void pspCls();


////////////////////////////////////////////////////////////////////////////////
//// Input
void pspInputInit();
void pspReadPad(void);

////////////////////////////////////////////////////////////////////////////////
//// Audio
#define SOUND_BUFLEN 512*2*6

int pspAudioInit();

////////////////////////////////////////////////////////////////////////////////
//// System

////////////////////////////////////////////////////////////////////////////////
//// C Runtime
int		memcmp(const void *buf1, const void *buf2,int n);
void*	memcpy(void *buf1, const void *buf2, int n);
void*	memset(void *buf, int ch, int n);
int		strlen(const char *s);
char*	strcpy(char *dest, const char *src);
char*	strrchr(const char *src, int c);
char*	strcat(char *dest, const char *src);
int		stricmp(const char *str1, const char *str2);
int		strcmp(const char *str1, const char *str2);
char*	strncat(char *s1,char *s2,int n) ;
char*	strncpy(char *s1,char *s2,int n);
void	itoa(int val, char *s);
static inline void __memcpy4a(unsigned long *d, unsigned long *s, unsigned long c);

////////////////////////////////////////////////////////////////////////////////
//// StartUp
int xmain(int argc, char *argv);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
