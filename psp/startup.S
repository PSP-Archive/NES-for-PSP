# startup code for PSP

#include <regdef.h>

		.set noreorder

		.text

		.extern xmain


##############################################################################


		.ent _start
		.weak _start
_start: 
      addiu    sp, 0x10 
      sw       ra, 0(sp)    
      sw       s0, 4(sp) 
      sw       s1, 8(sp) 

      la       v0,_gp
      move     gp,v0

      move     s0, a0            # Save args 
      move     s1, a1 

      la       a0, _main_thread_name   # Main thread setup 
      la       a1, xmain 
      li       a2, 0x20            # Priority 
      li       a3, 0x40000         # Stack size 
      lui      t0, 0x8000            # Attributes 
      jal      sceKernelCreateThread 
      move     t1, $0 

      move     a0, v0            # Start thread 
      move     a1, s0 
      jal      sceKernelStartThread 
      move     a2, s1 

      lw       ra, 0(sp) 
      lw       s0, 4(sp) 
      lw       s1, 8(sp) 
      move     v0, $0 
      jr       ra 
      addiu    sp, 0x10 

_main_thread_name: 
      .asciiz   "user_main" 

##############################################################################


		.section	.lib.ent,"wa",@progbits
__lib_ent_top:
		.word 0
		.word 0x80000000
		.word 0x00010104
		.word __entrytable


		.section	.lib.ent.btm,"wa",@progbits
__lib_ent_bottom:
		.word	0


		.section	.lib.stub,"wa",@progbits
__lib_stub_top:


		.section	.lib.stub.btm,"wa",@progbits
__lib_stub_bottom:
		.word	0


##############################################################################

		.section	".xodata.sceModuleInfo","wa",@progbits

__moduleinfo:
		.byte	0,0,1,1

		.ascii	"USERPROG"		#up to 28 char
		.align	5

		.word	_gp
		.word	__lib_ent_top
		.word	__lib_ent_bottom
		.word	__lib_stub_top
		.word	__lib_stub_bottom

##############################################################################

		.section	.rodata.entrytable,"wa",@progbits
__entrytable:
		.word 0xD632ACDB
		.word 0xF01D73A7
		.word _start
		.word __moduleinfo
		.word 0



###############################################################################


	.macro	STUB_START	module,d1,d2

		.section	.rodata.stubmodulename
		.word	0
__stub_modulestr_\@:
		.asciz	"\module"
		.align	2

		.section	.lib.stub
		.word __stub_modulestr_\@
		.word \d1
		.word \d2
		.word __stub_idtable_\@
		.word __stub_text_\@

		.section	.rodata.stubidtable
__stub_idtable_\@:

		.section	.text.stub
__stub_text_\@:

	.endm


	.macro	STUB_END
	.endm


	.macro	STUB_FUNC	funcid,funcname

		.set push
		.set noreorder

		.section	.text.stub
		.weak	\funcname
\funcname:
		jr	ra
		nop

		.section	.rodata.stubidtable
		.word	\funcid

		.set pop

	.endm


	STUB_START "sceDisplay",0x40010000,0x00100005 
	  STUB_FUNC 0x0e20f177,sceDisplaySetMode 
	  STUB_FUNC 0xdea197d4,sceDisplayGetMode 
	  STUB_FUNC 0xdba6c4c4,sceDisplayGetFramePerSec 
	  STUB_FUNC 0x7ed59bc4,sceDisplaySetHoldMode 
	  STUB_FUNC 0xa544c486,sceDisplaySetResumeMode 
	  STUB_FUNC 0x289d82fe,sceDisplaySetFrameBuf 
	  STUB_FUNC 0xeeda2e54,sceDisplayGetFrameBuf 
	  STUB_FUNC 0xb4f378fa,sceDisplayIsForeground 
	  STUB_FUNC 0x9c6eaad7,sceDisplayGetVcount 
	  STUB_FUNC 0x4d4e10ec,sceDisplayIsVblank 
	  STUB_FUNC 0x36cdfade,sceDisplayWaitVblank 
	  STUB_FUNC 0x8eb9ec49,sceDisplayWaitVblankCB 
	  STUB_FUNC 0x984c27e7,sceDisplayWaitVblankStart 
	  STUB_FUNC 0x46f186c3,sceDisplayWaitVblankStartCB 
	  STUB_FUNC 0x773dd3a3,sceDisplayGetCurrentHcount 
	  STUB_FUNC 0x210eab3a,sceDisplayGetAccumulatedHcount 
	STUB_END 

	STUB_START "sceCtrl",0x40010000,0x000a0005 
	  STUB_FUNC 0x6a2774f3,sceCtrlSetSamplingCycle 
	  STUB_FUNC 0x02baad91,sceCtrlGetSamplingCycle 
	  STUB_FUNC 0x1f4011e6,sceCtrlSetSamplingMode 
	  STUB_FUNC 0xda6b76a1,sceCtrlGetSamplingMode 
	  STUB_FUNC 0x3a622550,sceCtrlPeekBufferPositive 
	  STUB_FUNC 0xc152080a,sceCtrlPeekBufferNegative 
	  STUB_FUNC 0x1f803938,sceCtrlReadBufferPositive 
	  STUB_FUNC 0x60b81f86,sceCtrlReadBufferNegative 
	  STUB_FUNC 0xb1d0e5cd,sceCtrlPeekLatch 
	  STUB_FUNC 0x0b588501,sceCtrlReadLatch 
	STUB_END 

	STUB_START "IoFileMgrForUser",0x40010000,0x00240005 
	  STUB_FUNC 0x3251ea56,sceIoPollAsync 
	  STUB_FUNC 0xe23eec33,sceIoWaitAsync 
	  STUB_FUNC 0x35dbd746,sceIoWaitAsyncCB 
	  STUB_FUNC 0xcb05f8d6,sceIoGetAsyncStat 
	  STUB_FUNC 0xb293727f,sceIoChangeAsyncPriority 
	  STUB_FUNC 0xa12a0514,sceIoSetAsyncCallback 
	  STUB_FUNC 0x810c4bc3,sceIoClose 
	  STUB_FUNC 0xff5940b6,sceIoCloseAsync 
	  STUB_FUNC 0x109f50bc,sceIoOpen 
	  STUB_FUNC 0x89aa9906,sceIoOpenAsync 
	  STUB_FUNC 0x6a638d83,sceIoRead 
	  STUB_FUNC 0xa0b5a7c2,sceIoReadAsync 
	  STUB_FUNC 0x42ec03ac,sceIoWrite 
	  STUB_FUNC 0x0facab19,sceIoWriteAsync 
	  STUB_FUNC 0x27eb27b8,sceIoLseek 
	  STUB_FUNC 0x71b19e77,sceIoLseekAsync 
	  STUB_FUNC 0x68963324,sceIoLseek32 
	  STUB_FUNC 0x1b385d8f,sceIoLseek32Async 
	  STUB_FUNC 0x63632449,sceIoIoctl 
	  STUB_FUNC 0xe95a012b,sceIoIoctlAsync 
	  STUB_FUNC 0xb29ddf9c,sceIoDopen 
	  STUB_FUNC 0xe3eb004c,sceIoDread 
	  STUB_FUNC 0xeb092469,sceIoDclose 
	  STUB_FUNC 0xf27a9c51,sceIoRemove 
	  STUB_FUNC 0x06a70004,sceIoMkdir 
	  STUB_FUNC 0x1117c65f,sceIoRmdir 
	  STUB_FUNC 0x55f4717d,sceIoChdir 
	  STUB_FUNC 0xab96437f,sceIoSync 
	  STUB_FUNC 0xace946e8,sceIoGetstat 
	  STUB_FUNC 0xb8a740f4,sceIoChstat 
	  STUB_FUNC 0x779103a0,sceIoRename 
	  STUB_FUNC 0x54f5fb11,sceIoDevctl 
	  STUB_FUNC 0x08bd7374,sceIoGetDevType 
	  STUB_FUNC 0xb2a628c1,sceIoAssign 
	  STUB_FUNC 0x6d08a871,sceIoUnassign 
	  STUB_FUNC 0xe8bc6571,sceIoCancel 
	STUB_END 

	STUB_START "sceSuspendForUser",0x40010000,0x00030005 
	  STUB_FUNC 0xeadb1bd7,sceKernelPowerLock 
	  STUB_FUNC 0x3aee7261,sceKernelPowerUnlock 
	  STUB_FUNC 0x090ccb3f,sceKernelPowerTick 
	STUB_END 

	STUB_START "ThreadManForUser",0x40010000,0x006b0005 
	  STUB_FUNC 0x0c106e53,sceKernelRegisterThreadEventHandler 
	  STUB_FUNC 0x72f3c145,sceKernelReleaseThreadEventHandler 
	  STUB_FUNC 0x369eeb6b,sceKernelReferThreadEventHandlerStatus 
	  STUB_FUNC 0xe81caf8f,sceKernelCreateCallback 
	  STUB_FUNC 0xedba5844,sceKernelDeleteCallback 
	  STUB_FUNC 0xc11ba8c4,sceKernelNotifyCallback 
	  STUB_FUNC 0xba4051d6,sceKernelCancelCallback 
	  STUB_FUNC 0x2a3d44ff,sceKernelGetCallbackCount 
	  STUB_FUNC 0x349d6d6c,sceKernelCheckCallback 
	  STUB_FUNC 0x730ed8bc,sceKernelReferCallbackStatus 
	  STUB_FUNC 0x9ace131e,sceKernelSleepThread 
	  STUB_FUNC 0x82826f70,sceKernelSleepThreadCB 
	  STUB_FUNC 0xd59ead2f,sceKernelWakeupThread 
	  STUB_FUNC 0xfccfad26,sceKernelCancelWakeupThread 
	  STUB_FUNC 0x9944f31f,sceKernelSuspendThread 
	  STUB_FUNC 0x75156e8f,sceKernelResumeThread 
	  STUB_FUNC 0x278c0df5,sceKernelWaitThreadEnd 
	  STUB_FUNC 0x840e8133,sceKernelWaitThreadEndCB 
	  STUB_FUNC 0xceadeb47,sceKernelDelayThread 
	  STUB_FUNC 0x68da9e36,sceKernelDelayThreadCB 
	  STUB_FUNC 0xbd123d9e,sceKernelDelaySysClockThread 
	  STUB_FUNC 0x1181e963,sceKernelDelaySysClockThreadCB 
	  STUB_FUNC 0xd6da4ba1,sceKernelCreateSema 
	  STUB_FUNC 0x28b6489c,sceKernelDeleteSema 
	  STUB_FUNC 0x3f53e640,sceKernelSignalSema 
	  STUB_FUNC 0x4e3a1105,sceKernelWaitSema 
	  STUB_FUNC 0x6d212bac,sceKernelWaitSemaCB 
	  STUB_FUNC 0x58b1f937,sceKernelPollSema 
	  STUB_FUNC 0x8ffdf9a2,sceKernelCancelSema 
	  STUB_FUNC 0xbc6febc5,sceKernelReferSemaStatus 
	  STUB_FUNC 0x55c20a00,sceKernelCreateEventFlag 
	  STUB_FUNC 0xef9e4c70,sceKernelDeleteEventFlag 
	  STUB_FUNC 0x1fb15a32,sceKernelSetEventFlag 
	  STUB_FUNC 0x812346e4,sceKernelClearEventFlag 
	  STUB_FUNC 0x402fcf22,sceKernelWaitEventFlag 
	  STUB_FUNC 0x328c546a,sceKernelWaitEventFlagCB 
	  STUB_FUNC 0x30fd48f0,sceKernelPollEventFlag 
	  STUB_FUNC 0xcd203292,sceKernelCancelEventFlag 
	  STUB_FUNC 0xa66b0120,sceKernelReferEventFlagStatus 
	  STUB_FUNC 0x8125221d,sceKernelCreateMbx 
	  STUB_FUNC 0x86255ada,sceKernelDeleteMbx 
	  STUB_FUNC 0xe9b3061e,sceKernelSendMbx 
	  STUB_FUNC 0x18260574,sceKernelReceiveMbx 
	  STUB_FUNC 0xf3986382,sceKernelReceiveMbxCB 
	  STUB_FUNC 0x0d81716a,sceKernelPollMbx 
	  STUB_FUNC 0x87d4dd36,sceKernelCancelReceiveMbx 
	  STUB_FUNC 0xa8e8c846,sceKernelReferMbxStatus 
	  STUB_FUNC 0x7c0dc2a0,sceKernelCreateMsgPipe 
	  STUB_FUNC 0xf0b7da1c,sceKernelDeleteMsgPipe 
	  STUB_FUNC 0x876dbfad,sceKernelSendMsgPipe 
	  STUB_FUNC 0x7c41f2c2,sceKernelSendMsgPipeCB 
	  STUB_FUNC 0x884c9f90,sceKernelTrySendMsgPipe 
	  STUB_FUNC 0x74829b76,sceKernelReceiveMsgPipe 
	  STUB_FUNC 0xfbfa697d,sceKernelReceiveMsgPipeCB 
	  STUB_FUNC 0xdf52098f,sceKernelTryReceiveMsgPipe 
	  STUB_FUNC 0x349b864d,sceKernelCancelMsgPipe 
	  STUB_FUNC 0x33be4024,sceKernelReferMsgPipeStatus 
	  STUB_FUNC 0x56c039b5,sceKernelCreateVpl 
	  STUB_FUNC 0x89b3d48c,sceKernelDeleteVpl 
	  STUB_FUNC 0xbed27435,sceKernelAllocateVpl 
	  STUB_FUNC 0xec0a693f,sceKernelAllocateVplCB 
	  STUB_FUNC 0xaf36d708,sceKernelTryAllocateVpl 
	  STUB_FUNC 0xb736e9ff,sceKernelFreeVpl 
	  STUB_FUNC 0x1d371b8a,sceKernelCancelVpl 
	  STUB_FUNC 0x39810265,sceKernelReferVplStatus 
	  STUB_FUNC 0xc07bb470,sceKernelCreateFpl 
	  STUB_FUNC 0xed1410e0,sceKernelDeleteFpl 
	  STUB_FUNC 0xd979e9bf,sceKernelAllocateFpl 
	  STUB_FUNC 0xe7282cb6,sceKernelAllocateFplCB 
	  STUB_FUNC 0x623ae665,sceKernelTryAllocateFpl 
	  STUB_FUNC 0xf6414a71,sceKernelFreeFpl 
	  STUB_FUNC 0xa8aa591f,sceKernelCancelFpl 
	  STUB_FUNC 0xd8199e4c,sceKernelReferFplStatus 
	  STUB_FUNC 0xdb738f35,sceKernelGetSystemTime 
	  STUB_FUNC 0x82bc5777,sceKernelGetSystemTimeWide 
	  STUB_FUNC 0x369ed59d,sceKernelGetSystemTimeLow 
	  STUB_FUNC 0x6652b8ca,sceKernelSetAlarm 
	  STUB_FUNC 0xb2c25152,sceKernelSetSysClockAlarm 
	  STUB_FUNC 0x7e65b999,sceKernelCancelAlarm 
	  STUB_FUNC 0xdaa3f564,sceKernelReferAlarmStatus 
	  STUB_FUNC 0x20fff560,sceKernelCreateVTimer 
	  STUB_FUNC 0x328f9e52,sceKernelDeleteVTimer 
	  STUB_FUNC 0xb3a59970,sceKernelGetVTimerBase 
	  STUB_FUNC 0x034a921f,sceKernelGetVTimerTime 
	  STUB_FUNC 0x542ad630,sceKernelSetVTimerTime 
	  STUB_FUNC 0xc68d9437,sceKernelStartVTimer 
	  STUB_FUNC 0xd0aeee87,sceKernelStopVTimer 
	  STUB_FUNC 0xd8b299ae,sceKernelSetVTimerHandler 
	  STUB_FUNC 0xd2d615ef,sceKernelCancelVTimerHandler 
	  STUB_FUNC 0x5f32beaa,sceKernelReferVTimerStatus 
	  STUB_FUNC 0x446d8de6,sceKernelCreateThread 
	  STUB_FUNC 0x9fa03cd3,sceKernelDeleteThread 
	  STUB_FUNC 0xf475845d,sceKernelStartThread 
	  STUB_FUNC 0xaa73c935,sceKernelExitThread 
	  STUB_FUNC 0x809ce29b,sceKernelExitDeleteThread 
	  STUB_FUNC 0x616403ba,sceKernelTerminateThread 
	  STUB_FUNC 0x383f7bcc,sceKernelTerminateDeleteThread 
	  STUB_FUNC 0x3ad58b8c,sceKernelSuspendDispatchThread 
	  STUB_FUNC 0x27e22ec2,sceKernelResumeDispatchThread 
	  STUB_FUNC 0x71bc9871,sceKernelChangeThreadPriority 
	  STUB_FUNC 0x2c34e053,sceKernelReleaseWaitThread 
	  STUB_FUNC 0x293b45b8,sceKernelGetThreadId 
	  STUB_FUNC 0x94aa61ee,sceKernelGetThreadCurrentPriority 
	  STUB_FUNC 0xd13bde95,sceKernelCheckThreadStack 
	  STUB_FUNC 0x627e6f3a,sceKernelReferSystemStatus 
	  STUB_FUNC 0x94416130,sceKernelGetThreadmanIdList 
	  STUB_FUNC 0x57cf62dd,sceKernelGetThreadmanIdType 
	STUB_END 

	STUB_START "LoadExecForUser",0x40010000,0x00040005 
	  STUB_FUNC 0xbd2f1094,sceKernelLoadExec 
	  STUB_FUNC 0x2ac9954b,sceKernelExitGameWithStatus 
	  STUB_FUNC 0x05572a5f,sceKernelExitGame 
	  STUB_FUNC 0x4ac57943,sceKernelRegisterExitCallback 
	STUB_END 

	STUB_START "scePower",0x40010000,0x00210005 
	  STUB_FUNC 0xefd3c963,scePowerTick 
	  STUB_FUNC 0xedc13fe5,scePowerGetIdleTimer 
	  STUB_FUNC 0x7f30b3b1,scePowerIdleTimerEnable 
	  STUB_FUNC 0x972ce941,scePowerIdleTimerDisable 
	  STUB_FUNC 0x27f3292c,scePowerBatteryUpdateInfo 
	  STUB_FUNC 0x87440f5e,scePowerIsPowerOnline 
	  STUB_FUNC 0x0afd0d8b,scePowerIsBatteryExist 
	  STUB_FUNC 0x1e490401,scePowerIsBatteryCharging 
	  STUB_FUNC 0xb4432bc8,scePowerGetBatteryChargingStatus 
	  STUB_FUNC 0xd3075926,scePowerIsLowBattery 
	  STUB_FUNC 0x2085d15d,scePowerGetBatteryLifePercent 
	  STUB_FUNC 0x8efb3fa2,scePowerGetBatteryLifeTime 
	  STUB_FUNC 0x28e12023,scePowerGetBatteryTemp 
	  STUB_FUNC 0x862ae1a6,scePowerGetBatteryElec 
	  STUB_FUNC 0x483ce86b,scePowerGetBatteryVolt 
	  STUB_FUNC 0xd6d016ef,scePowerLock 
	  STUB_FUNC 0xca3d34c1,scePowerUnlock 
	  STUB_FUNC 0xdb62c9cf,scePowerCancelRequest 
	  STUB_FUNC 0x7fa406dd,scePowerIsRequest 
	  STUB_FUNC 0x2b7c7cf4,scePowerRequestStandby 
	  STUB_FUNC 0xac32c9cc,scePowerRequestSuspend 
	  STUB_FUNC 0x0074ef9b,scePowerGetResumeCount 
	  STUB_FUNC 0x04b7766e,scePowerRegisterCallback 
	  STUB_FUNC 0xdfa8baf8,scePowerUnregisterCallback 
	  STUB_FUNC 0x843fbf43,scePowerSetCpuClockFrequency 
	  STUB_FUNC 0xb8d7b3fb,scePowerSetBusClockFrequency 
	  STUB_FUNC 0xfee03a2f,scePowerGetCpuClockFrequency 
	  STUB_FUNC 0x478fe6f5,scePowerGetBusClockFrequency 
	  STUB_FUNC 0xfdb5bfe9,scePowerGetCpuClockFrequencyInt 
	  STUB_FUNC 0xbd681969,scePowerGetBusClockFrequencyInt 
	  STUB_FUNC 0xb1a52c83,scePowerGetCpuClockFrequencyFloat 
	  STUB_FUNC 0x9badb3eb,scePowerGetBusClockFrequencyFloat 
	  STUB_FUNC 0x737486f2,scePowerSetClockFrequency 
	STUB_END 

	STUB_START "UtilsForUser",0x40010000,0x00160005 
	  STUB_FUNC 0xbfa98062,sceKernelDcacheInvalidateRange 
	  STUB_FUNC 0xc8186a58,sceKernelUtilsMd5Digest 
	  STUB_FUNC 0x9e5c5086,sceKernelUtilsMd5BlockInit 
	  STUB_FUNC 0x61e1e525,sceKernelUtilsMd5BlockUpdate 
	  STUB_FUNC 0xb8d24e78,sceKernelUtilsMd5BlockResult 
	  STUB_FUNC 0x840259f1,sceKernelUtilsSha1Digest 
	  STUB_FUNC 0xf8fcd5ba,sceKernelUtilsSha1BlockInit 
	  STUB_FUNC 0x346f6da8,sceKernelUtilsSha1BlockUpdate 
	  STUB_FUNC 0x585f1c09,sceKernelUtilsSha1BlockResult 
	  STUB_FUNC 0xe860e75e,sceKernelUtilsMt19937Init 
	  STUB_FUNC 0x06fb8a63,sceKernelUtilsMt19937UInt 
	  STUB_FUNC 0x37fb5c42,sceKernelGetGPI 
	  STUB_FUNC 0x6ad345d7,sceKernelSetGPO 
	  STUB_FUNC 0x91e4f6a7,sceKernelLibcClock 
	  STUB_FUNC 0x27cc57f0,sceKernelLibcTime 
	  STUB_FUNC 0x71ec4271,sceKernelLibcGettimeofday 
	  STUB_FUNC 0x79d1c3fa,sceKernelDcacheWritebackAll 
	  STUB_FUNC 0xb435dec5,sceKernelDcacheWritebackInvalidateAll 
	  STUB_FUNC 0x3ee30821,sceKernelDcacheWritebackRange 
	  STUB_FUNC 0x34b9fa9e,sceKernelDcacheWritebackInvalidateRange 
	  STUB_FUNC 0x80001c4c,sceKernelDcacheProbe 
	  STUB_FUNC 0x4fd31c9d,sceKernelIcacheProbe 
	STUB_END 

	STUB_START "sceAudio",0x40010000,0x000f0005 
	  STUB_FUNC 0x8c1009b2,sceAudioOutput 
	  STUB_FUNC 0x136caf51,sceAudioOutputBlocking 
	  STUB_FUNC 0xe2d56b2d,sceAudioOutputPanned 
	  STUB_FUNC 0x13f592bc,sceAudioOutputPannedBlocking 
	  STUB_FUNC 0x5ec81c55,sceAudioChReserve 
	  STUB_FUNC 0x6fc46853,sceAudioChRelease 
	  STUB_FUNC 0xe9d97901,sceAudioGetChannelRestLen 
	  STUB_FUNC 0xcb2e439e,sceAudioSetChannelDataLen 
	  STUB_FUNC 0x95fd0c2d,sceAudioChangeChannelConfig 
	  STUB_FUNC 0xb7e1d8e7,sceAudioChangeChannelVolume 
	  STUB_FUNC 0x086e5895,sceAudioInputBlocking 
	  STUB_FUNC 0x6d4bec68,sceAudioInput 
	  STUB_FUNC 0xa708c6a6,sceAudioGetInputLength 
	  STUB_FUNC 0x87b2e651,sceAudioWaitInputEnd 
	  STUB_FUNC 0x7de61688,sceAudioInputInit 
	STUB_END 

	STUB_START "SysMemUserForUser",0x40010000,0x00050005 
	  STUB_FUNC 0xa291f107,sceKernelMaxFreeMemSize 
	  STUB_FUNC 0xf919f628,sceKernelTotalFreeMemSize 
	  STUB_FUNC 0x237dbd4f,sceKernelAllocPartitionMemory 
	  STUB_FUNC 0xb6d61d02,sceKernelFreePartitionMemory 
	  STUB_FUNC 0x9d9a5ba1,sceKernelGetBlockHeadAddr 
	STUB_END 


###############################################################################

    .text

    .end _start

