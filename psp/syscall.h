/* system call prototype for PSP */

#ifndef _SYSCALL_H_INCLUDED
#define _SYSCALL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned long	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;

/******************************************************************************/
/* sceDisplay */

#define VRAM_ADDR	((char *)0x04000000)

#define SCREEN_WIDTH	480
#define SCREEN_HEIGHT	272

int sceDisplaySetMode(int iMode, int iDisplayWidth, int iDisplayHeight);
int sceDisplaySetFrameBuf(const void *pFrameBuf, int iFrameWidth, int iPixelFormat, int iUpdateTimingMode);
int sceDisplayWaitVblank(void);
int sceDisplayWaitVblankStart(void);

/******************************************************************************/
/* sceCtrl */

#define CTRL_SELECT		0x00000001	/* SELECTボタン           */
#define CTRL_START		0x00000008	/* STARTボタン            */
#define CTRL_UP			0x00000010	/* 方向キー ↑ボタン      */
#define CTRL_RIGHT		0x00000020	/* 方向キー →ボタン      */
#define CTRL_DOWN		0x00000040	/* 方向キー ↓ボタン      */
#define CTRL_LEFT		0x00000080	/* 方向キー ←ボタン      */
#define CTRL_L			0x00000100	/* Lボタン                */
#define CTRL_R			0x00000200	/* Rボタン                */
#define CTRL_TRIANGLE	0x00001000	/* △ボタン               */
#define CTRL_CIRCLE		0x00002000	/* ○ボタン               */
#define CTRL_CROSS		0x00004000	/* ×ボタン               */
#define CTRL_SQUARE		0x00008000	/* □ボタン               */
#define	CTRL_HOLD		0x00020000	/* HOLDスイッチ           */

#define CTRL_MODE_DIGITALONLY	0	/* デジタルボタンのみ       */
#define CTRL_MODE_DIGITALANALOG	1	/* アナログコントローラ使用 */

typedef struct PSP_CtrlData {
	unsigned int  TimeStamp;	/* タイムスタンプ(μsec)      */
	unsigned int  Buttons;		/* デジタルボタン押し下げ情報 */
	unsigned char Lx;			/* アナログコントローラX軸    */
	unsigned char Ly;			/* アナログコントローラY軸    */
	unsigned char Rsrv[6]; 		/* リザーブ                   */
} PSP_CtrlData;

int sceCtrlSetSamplingCycle(unsigned int uiCycle);
int sceCtrlSetSamplingMode(unsigned int uiMode);
int sceCtrlReadBufferPositive(PSP_CtrlData *pData, int nBufs);
int sceCtrlPeekBufferPositive(PSP_CtrlData *pData, int nBufs);

/******************************************************************************/
/* IoFileMgrForUser */

#define MAX_PATH  512
#define MAX_NAME  256
#define MAX_ENTRY 1024

#define O_RDONLY    0x0001 
#define O_WRONLY    0x0002 
#define O_RDWR      0x0003 
#define O_NBLOCK    0x0010 
#define O_APPEND    0x0100 
#define O_CREAT     0x0200 
#define O_TRUNC     0x0400 
#define O_NOWAIT    0x8000 

enum { 
    TYPE_DIR=0x10, 
    TYPE_FILE=0x20 
}; 

struct dirent_tm {
	u16 unk[2]; //常にゼロ？
	u16 year;
	u16 mon;
	u16 mday;
	u16 hour;
	u16 min;
	u16 sec;
};

typedef struct _dirent_t { 
    u32 unk0; 
    u32 type; 
    u32 size; 
	struct dirent_tm ctime; //作成日時
	struct dirent_tm atime; //最終アクセス日時
	struct dirent_tm mtime; //最終更新日時
	u32 unk[7]; //常にゼロ？
    char name[0x108]; 
} dirent_t; 

int       sceIoRemove	(const char *filename);
int       sceIoMkdir	(const char *dirname, int mode);
int       sceIoRmdir	(const char *dirname);
int       sceIoRename	(const char *oldname, const char *newname);

int       sceIoOpen		(const char *filename, int flag, int mode);
int       sceIoClose	(int fd);
long long sceIoLseek	(int fd, long long offset, int whence);
int       sceIoRead		(int fd, void *buf, unsigned int nbyte);
int       sceIoWrite	(int fd, const void *buf, unsigned int nbyte);

int       sceIoDopen	(const char *dirname);
int       sceIoDclose	(int fd);
int       sceIoDread	(int fd, dirent_t *buf);

/******************************************************************************/
/* sceSuspendForUser */

int sceKernelPowerLock(int locktype);		/* POWERスイッチ操作ロック        */
int sceKernelPowerUnlock(int locktype);		/* POWERスイッチ操作アンロック    */

/******************************************************************************/
/* ThreadManForUser */

typedef struct PSP_KernelThreadOptParam  {
    unsigned int    size;
    unsigned int    stackMpid;
} PSP_KernelThreadOptParam;

typedef int (*PspKernelThreadEntry)(unsigned int argSize, void *argBlock);
typedef int (*PspKernelCallbackFunction)(int count, int arg, void *common);

int sceKernelCreateThread(const char *name, PspKernelThreadEntry entry, int initPriority, unsigned int stackSize, unsigned int attr, const PSP_KernelThreadOptParam  *optParam);
int sceKernelDeleteThread(int thid);
int sceKernelStartThread(int thid, unsigned int argSize, const void *argBlock);
int sceKernelExitThread(int exitStatus);

int sceKernelSleepThread(void);
int sceKernelSleepThreadCB(void);
int sceKernelWaitThreadEnd(int thid, unsigned int *timeout);

int sceKernelCreateCallback(const char *name, PspKernelCallbackFunction callback, void *common);

/******************************************************************************/
/* LoadExecForUser */

int sceKernelExitGame(void);
int sceKernelRegisterExitCallback(int uid);

/******************************************************************************/
/* scePower */

#define POWER_CB_BATTERY_CAP	0x0000007F	/* バッテリー残容量[%] (0～100) */
#define POWER_CB_BATTERYEXIST	0x00000080	/* バッテリが存在している       */
#define POWER_CB_LOWBATTERY		0x00000100	/* ローバッテリ                 */
#define POWER_CB_POWERONLINE	0x00001000	/* 外部電源供給あり             */
#define POWER_CB_SUSPENDING		0x00010000	/* サスペンド処理中             */
#define POWER_CB_RESUMING		0x00020000	/* レジューム処理中             */
#define POWER_CB_RESUME_COMP	0x00040000	/* レジューム完了               */
#define POWER_CB_STANDINGBY		0x00080000	/* スタンバイ処理中             */
#define POWER_CB_HOLDSW			0x40000000	/* HOLDスイッチ状態             */
#define POWER_CB_POWERSW		0x80000000	/* POWERスイッチ状態            */

#define POWER_ERROR_NO_BATTERY	0x802B0100	/* バッテリが存在しない         */
#define POWER_ERROR_DETECTING	0x802B0101	/* バッテリ情報の取得中         */

int scePowerIsPowerOnline(void);
int scePowerIsBatteryExist(void);
int scePowerIsBatteryCharging(void);
int scePowerIsLowBattery(void);
int scePowerGetBatteryLifePercent(void);
int scePowerGetBatteryLifeTime(void);

int scePowerRegisterCallback(int slot, int cbid);
int scePowerUnregisterCallback(int slot);

int scePowerSetClockFrequency(int cpufreq,int pplfreq,int busfreq);
int scePowerSetCpuClockFrequency(int cpufreq);
int scePowerSetBusClockFrequency(int busfreq);

/******************************************************************************/
/* UtilsForUser */

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct sce_timezone {
  int tz_minuteswest;
  int tz_dsttime;
};


unsigned long sceKernelLibcClock(void);
long          sceKernelLibcTime(long *pTime);

/******************************************************************************/
/* sceAudio */

void sceAudioOutputBlocking();
void sceAudioOutputPanned();
long sceAudioOutputPannedBlocking(long, long, long, void *);
long sceAudioChReserve(long, long samplecount, long);//init buffer? returns handle, minus if error
void sceAudioChRelease(long handle);//free buffer?
void sceAudioGetChannelRestLen();
long sceAudioSetChannelDataLen(long, long);
void sceAudioChangeChannelConfig();
void sceAudioChangeChannelVolume();

/******************************************************************************/
/* SysMemUserForUser */

int		sceKernelAllocPartitionMemory(void* buf,int size); 
void	sceKernelFreePartitionMemory (void* buf);






#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _SYSCALL_H_INCLUDED
