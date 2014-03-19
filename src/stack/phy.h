#ifndef PHY_H
#define PHY_H

#include "compiler.h"
#include "halStack.h"

//表格18
#define aMaxPHYPacketSize 127//物理层服务数据单元（PSDU）的最大容量为127
#define aTurnaroundTime 12//RX和TX状态转变的最大时间为12个符号周期

#define MAX_TX_TRANSMIT_TIME (SYMBOLS_TO_MACTICKS(300))  //300 MACTICKS = 300*16 us
//最大的传输时间，用于物理层传输超时。

//射频状态枚举：关闭、接收开、发送开、接收发送开。
typedef enum _RADIO_STATUS_ENUM {
  RADIO_STATUS_OFF,
  RADIO_STATUS_RX_ON,
  RADIO_STATUS_TX_ON,
  RADIO_STATUS_RXTX_ON
}RADIO_STATUS_ENUM;

//物理层PAN信息库：物理层当前使用频率、物理层当前信道(0~26)、
//物理层支持的信道、物理层传输功率、物理层CCA模式、物理层数据标志位、
//传输开始时间、当前传输的帧指针和长度。
//物理层数据标志位：一个字节的变量或者一个bits（传输结束和传输缓存锁定）
typedef struct _PHY_PIB {
  PHY_FREQ_ENUM phyCurrentFrequency;        //current frequency in KHz (2405000 = 2.405 GHz)
  BYTE phyCurrentChannel;
  UINT32 phyChannelsSupported;
  BYTE phyTransmitPower;
  BYTE phyCCAMode;
  union _PHY_DATA_flags {
    BYTE val;
    struct {
     unsigned txFinished:1;    //indicates if TX at PHY level is finished...
	 unsigned txBuffLock:1;    //lock the TX buffer.
    }bits;
  }flags;
  UINT32 txStartTime;
  unsigned char *currentTxFrm;   //current frame,首地址指向帧控制域
  BYTE currentTxFlen;   //current TX frame length,不包含FCS域
}PHY_PIB;

/*
//收发机状态枚举
typedef enum _PHY_TRX_STATE_ENUM {
	TRX_OFF,
	RX_ON,
	TX_ON,
	FORCE_TRX_OFF
} PHY_TRX_STATE_ENUM;
*/
//物理层初始化射频的标记，LRWPAN_COMMON_TYPES_H定义了射频标志 RADIO_FLAGS。
typedef union _PHY_ARGS {
  struct _PHY_INIT_RADIO {
    RADIO_FLAGS radio_flags;
  }phy_init_radio_args;
  struct {
  	INT8 EnergyLevel;
  	}ed;
  /*struct {
  	PHY_TRX_STATE_ENUM state;
  	}set_trx;*/
}PHY_ARGS;


//物理层服务：LRPAN服务，物理层ARGS、LRPAN状态。
typedef struct _PHY_SERVICE {
  LRWPAN_SVC_ENUM cmd;
  PHY_ARGS args;
  LRWPAN_STATUS_ENUM status;
}PHY_SERVICE;



//物理层状态：空闲、命令开始、传输等待。
typedef enum _PHY_STATE_ENUM {
  PHY_STATE_IDLE,
  PHY_STATE_COMMAND_START,
  PHY_STATE_TX_WAIT
 } PHY_STATE_ENUM;
//声明外部变量：物理层状态、物理层信息库、物理层服务。
//临时传输的缓存大小为127
extern PHY_STATE_ENUM phyState;//反映物理层状态，用于状态机
extern PHY_PIB phy_pib;
extern PHY_SERVICE a_phy_service;
extern BYTE tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];//Size is 127，用来装载数据，从后往前
//extern PHY_TRX_STATE_ENUM phyTRXSate;

//prototypes
void phyFSM(void);
//void phyDoService(PHY_SERVICE *ps);
void phyInit(void );

#define phyIdle() (phyState == PHY_STATE_IDLE)
#define phyBusy() (phyState != PHY_STATE_IDLE)

#define phyTxLocked()   (phy_pib.flags.bits.txBuffLock == 1)
#define phyTxUnLocked()   (phy_pib.flags.bits.txBuffLock == 0)
#define phyGrabTxLock()	phy_pib.flags.bits.txBuffLock = 1
#define phyReleaseTxLock() phy_pib.flags.bits.txBuffLock = 0

/*
#define RFSTATUS_SFD_MASK	0x02
#define phyTxBusy()		(RFSTATUS&RFSTATUS_SFD_MASK)
*/
#define phyTxBusy()		(phy_pib.flags.bits.txFinished == 0)
#define phyTxIdle()		(phy_pib.flags.bits.txFinished == 1)
#define phySetTxBusy()		phy_pib.flags.bits.txFinished = 0
#define phySetTxIdle()		phy_pib.flags.bits.txFinished = 1

/*
#define RFSTATUS_SFD_MASK	0x02
#define phyRxBusy()		(RFSTATUS&RFSTATUS_SFD_MASK)


#define phyGetTRXState()	phyTRXSate

#define phyStartTx() \
	ISTXCALN;\
	ISTXON;\
	phyTRXSate = TX_ON;

//#define phyStartTx() (ISTXONCCA)

#define phyStartRx()\
	ISRXON;\
	phyTRXSate = RX_ON;

#define phyStopTRX()\
	ISRFOFF;\
	phyTRXSate = TRX_OFF;

*/





//cannot overlap services
//make this a macro to reduce stack depth
#define phyDoService() \
  a_phy_service.status = LRWPAN_STATUS_PHY_INPROGRESS;\
  phyState = PHY_STATE_COMMAND_START;\
  phyFSM();
#endif

