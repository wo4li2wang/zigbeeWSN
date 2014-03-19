#ifndef MAC_H
#define MAC_H

//表格70,Page120
#define aBaseSlotDuration 60//超帧顺序为0时,超帧时隙持续时间为60个符号数
#define aNumSuperFrameSlots 16//任何超帧中包含的时隙数为16
#define aBaseSuperFrameDuration (aBaseSlotDuration*aNumSuperFrameSlots)//超帧序列为0时，组成超帧的符号数
#define aMaxBE 5//csma-ca算法中，退避指数的最大值
#define aMinBE 3//回退指数最小值
#define aUnitBackoffPeriod 20   //形成CSMA_CA算法，所使用的基本时间段的符号数为20
#define macMaxCSMABackoffs 4//mac最大的退避次数
#define aMaxBeaconOverhead 75//信标帧最大的报文头
#define aMaxBeaconPayloadLength (aMaxPHYPacketSize-aMaxBeaconOverhead)//信标帧最大的有效载荷长度,127-75=52
#define aMaxFrameOverhead 25//最大的帧头长度,3+20+2
#define aMaxFrameResponseTime 1220
//为等待数据请求帧的响应帧，在信标使能PAN中CAP码位的最大值，或者时在非信标PAN中的最大码位
#define aMaxFrameRetries LRWPAN_MAC_MAX_FRAME_RETRIES//在传输失败时允许的最大重传次数,3
#define aMaxLostBeacons 4//接收机允许连续丢失信标的最大数，超过会声明失去同步
#define aMaxMACFrameSize (aMaxPHYPacketSize-aMaxFrameOverhead)//MAC帧的最大长度,127-25=102
#define aResponseWaitTime (32*aBaseSuperFrameDuration)//设备发出请求命令后，在得到响应命令之前需要等待的最大符号数
//32*60*16
#define aComScheduTime (16*T2CMPVAL*10)
//default timeout on network responses
#ifdef LRWPAN_DEBUG
//give longer due to debugging output
//定义了mac连接请求等待时间和孤立设备等待时间
#define MAC_GENERIC_WAIT_TIME      MSECS_TO_MACTICKS(100)//100*(62500/1000)=6250(MACTICKS)
#define MAC_ASSOC_WAIT_TIME        MAC_GENERIC_WAIT_TIME
#define MAC_ORPHAN_WAIT_TIME       MAC_GENERIC_WAIT_TIME
#else
#define MAC_GENERIC_WAIT_TIME      MSECS_TO_MACTICKS(20)//20*(62500/1000)=1250(MACTICKS)
#define MAC_ASSOC_WAIT_TIME        MAC_GENERIC_WAIT_TIME
#define MAC_ORPHAN_WAIT_TIME       MAC_GENERIC_WAIT_TIME
#endif

//定义了mac接收缓存器的大小，mac接收报文个数的最大值（4）+1
#define MAC_RXBUFF_SIZE LRWPAN_MAX_MAC_RX_PKTS+1//4+1

#define MAC_SPEC_LQI_MAX 0xFF
#define MAC_RADIO_CORR_MAX 110
#define MAC_RADIO_CORR_MIN 50

#define MAC_SCAN_SIZE 16
#define MAC_SPEC_ED_MAX 0xFF
#define MAC_RADIO_RECEIVER_SATURATION_DBM       10  //dBm
#define MAC_RADIO_RECEIVER_SENSITIVITY_DBM      -91 //dBm
#define MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY    10
#define MAC_RADIO_RSSI_OFFSET -45
#define ED_RF_POWER_MIN_DBM   (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define ED_RF_POWER_MAX_DBM   MAC_RADIO_RECEIVER_SATURATION_DBM

#define macGetCurSlotTimes() (mac_pib.macCurrentSlotTimes)

#define macGetShortAddr()   (mac_pib.macShortAddress)
#define macSetDepth(x)      mac_pib.macDepth = x

#define macIdle() (macState == MAC_STATE_IDLE)//判断MAC层状态是否为闲
#define macBusy() (macState != MAC_STATE_IDLE)//判断MAC层状态是否为忙

#define macTXIdle() (!mac_pib.flags.bits.TxInProgress)
#define macTXBusy() (mac_pib.flags.bits.TxInProgress)
#define macSetTxBusy() mac_pib.flags.bits.TxInProgress = 1
#define macSetTxIdle() mac_pib.flags.bits.TxInProgress = 0

#define macDoService() \
	a_mac_service.status = LRWPAN_STATUS_MAC_INPROGRESS;\
	macState = MAC_STATE_COMMAND_START;\
	macFSM();\

//数据发送或命令发送，需要等待ACK时，相关信息表
//以后可能要加入更多的信息，如目的地址，信道号等，方便上层处理
#define MaxAckWindow 8
typedef struct _ACK_PENDING_TABLE {
  union {
    BYTE val;
    struct {
      unsigned Used:1;  //用于判断是否被用
      unsigned ACKPending:1;  //用于判断是否接收到确认帧
    }bits;
  }options;
  BYTE currentAckRetries;//记录该数据帧已经发送次数
  BYTE DSN;
  LRWPAN_STATUS_ENUM ErrorState;
  BYTE *Ptr;//指向发送数据的首地址，便于重新发送
  UINT32 TxStartTime;//记录发送数据的时间，以便超时计算
}ACK_PENDING_TABLE;

//用于信道扫描中的主动和被动扫描
typedef struct _SCAN_PAN_INFO {
  //BYTE CoordAddrMode;
  UINT16 CoordPANId;
  SADDR ShortAddress;
  LADDR ExtendedAddress;
  BYTE LogicalChannel;
  UINT16 SuperframeSpec;
  BYTE LinkQuality;//待实现
  UINT32 TimeStamp;//待实现
  BYTE ACLEntry;//待实现
  BYTE StackProfile;//为发现网络而添加的
  BYTE ZigBeeVersion;//为发现网络而添加的
  BYTE Depth;//为发现网络而添加的
  union {//待实现
    BYTE val;
    struct {
      unsigned GTSPermit:1;
      unsigned SecurityUse:1;
      unsigned AssociationPermit:1;
      unsigned PANCoordinator:1;
      unsigned BatteryLifeExtension:1;
      unsigned RouterRoom:1;//为'1'表示该节点有空间给ROUTER加入网络
      unsigned RfdRoom:1;//为'1'表示该节点有空间给RFD加入网络
    }bits;
  }options;
}SCAN_PAN_INFO;

//extern SCAN_PAN_INFO PANDesciptorList[MAC_SCAN_SIZE];


//信道扫描类型
typedef enum _SCAN_TYPE_ENUM {
  ENERGY_DETECT,
  ACTIVE,
  PASSIVE,
  ORPHAN
}SCAN_TYPE_ENUM;

//mac层的PAN信息库，见表格71
typedef struct _MAC_PIB {
  UINT32 macAckWaitDuration;//数据帧后面的应答帧所需要等待的时间54（默认）-120
  union _MAC_PIB_flags {
    UINT32 val;
    struct {
      unsigned macAssociationPermit:1;//是否允许本设备连接网络
      unsigned macAutoRequest:1;//地址被列在信标帧中，是否自动发送数据请求命令
      unsigned macBattLifeExt:1;//信标帧在CAP阶段，是否降低接收机操作时间
      unsigned macGTSPermit:1;//是否允许GTS操作
      unsigned macPromiscousMode:1;//混杂模式，是否允许接收物理层来的所有报文
      unsigned macPanCoordinator:1;//本设备是否为协调器	
      unsigned ackPending:1;//是否有ACK未处理
      unsigned TxInProgress:1;//发射机是否在工作中
      unsigned GotBeaconResponse:1;//是否得到信标响应，set to a '1' when get Beacon Response
      unsigned WaitingForBeaconResponse:1; //是否在等到信标响应，set to '1' when waiting for Response
      unsigned macPending:1;//在接收缓存器里，是否有未处理的MAC层命令
      unsigned macIsAssociated:1;//形成网络（coordinator）或加入网络（FFD和RFD）前将其置'0',表示不是网络的成员；成功后将其置'1',表示是网络的成员。在网络层处理
      unsigned WaitingForAssocResponse:1;//在发送连接请求前将其置'1'，接收到响应后将其置'0',以防接收到多次响应。在MAC层处理
      unsigned GotOrphanResponse:1;//是否得到孤立响应
      unsigned WaitingForOrphanResponse:1;//是否在等待孤立响应
      unsigned WaitingForSyncRep:1;
      unsigned TransmittingSyncRep:1;
      unsigned TransmittingSyncReq:1;
     // unsigned TransmittingGTSReq:1;
    }bits;
  }flags;
  LADDR macCoordExtendedAddress;//协调器的64位长地址
  SADDR macCoordShortAddress;//协调器的16位短地址
  LADDR macParentExtendedAddress;//父设备的64位长地址
  SADDR macParentShortAddress;//父设备的16位短地址
  UINT16 macShortAddress;//本设备的短地址
  LADDR macExtendedAddress;//本设备的64位长地址

  BYTE finalSlot,GTSDescriptCount,GTSDirection,macCurrentSlot;
  BYTE pendingSaddrNumber,pendingLaddrNumber;

  BOOL corGTSPermit,corAssociationPermit;
  BYTE localGTSSlot,localGTSLength;
  BOOL localdirection;

  UINT16 macPANID;//16位PAN ID
  BYTE macBeaconOrder;
  BYTE macSuperframeOrder;
  UINT32 macCurrentSlotTimes;
  BYTE macScanNodeCount;
  BYTE macDSN;//MAC最近打包的数据帧序列号
  BYTE macDSNIndex;//MAC最近打包的数据帧存储的索引号
  BYTE macBSN;
  BYTE macDepth;//本设备在网络中的深度
  BYTE macCapInfo;//连接请求命令中的性能信息
  BYTE macMaxAckRetries;//应答重传的最大次数
  struct  {
    unsigned maxMaxCSMABackoffs:3;//回退次数位3。 4
    unsigned macMinBE:2;//最小的回退指数为2。 0-3
  }misc;//用于CSMA-CA算法
  UINT32 txStartTime;    //time that packet was sent
  UINT32 last_data_rx_time;//time that last data rx packet was received that was accepted by this node

  BYTE bcnDepth;//?
  SADDR bcnSADDR;//?
  UINT16 bcnPANID;//?
  BYTE bcnRSSI;//?

  BYTE currentAckRetries;//当前应答重传次数
  BYTE rxTail;             //tail pointer of rxBuff
  BYTE rxHead;             //head pointer of rxBuff
  //fifo for RX pkts, holds LRWPAN_MAX_MAC_RX_PKTS
  MACPKT  rxBuff[MAC_RXBUFF_SIZE];  //buffer for packets not yet processed，4+1

#ifdef LRWPAN_FFD
  //neighbor info
  UINT16 nextChildRFD;//下一个子设备的短地址，用于短地址分配
  UINT16 nextChildRouter;//下一个子路由器的短地址，用于短地址分配
  BYTE   ChildRFDs;         //number of neighbor RFDs
  BYTE   ChildRouters;      //number of neighbor Routers
#endif

}MAC_PIB;


//used for parsing of RX data，解析收到的数据
typedef struct _MAC_RX_DATA {
  MACPKT *orgpkt;//original packet
  BYTE fcflsb;//帧控制域前八位
  BYTE fcfmsb;//帧控制域后八位
  UINT16 DestPANID;//目的PAN符
  LADDR_UNION DestAddr; //dst address, either short or long
  UINT16 SrcPANID;//源PAN符
  LADDR_UNION SrcAddr;  //src address, either short or long
  BYTE LQI;//根据底层corr值计算得来
  //BYTE ED;//根据底层rssi值计算得来
  BYTE pload_offset;    //start of payload
}MAC_RX_DATA;

//发送的数据
typedef struct _MAX_TX_DATA {
	UINT16 DestPANID;//目的PAN符
	LADDR_UNION DestAddr;//dst address, either short or long
	UINT16 SrcPANID;//源PAN符
	SADDR SrcAddr;         //src address, either short or long, this holds short address version
	                       //if long is needed, then get this from HAL layer
	BYTE fcflsb;//frame control bits specify header bits
	BYTE fcfmsb;
	union  {
		BYTE val;
		struct _MAC_TX_DATA_OPTIONS_bits {
			unsigned gts:1;//通过GTS传输
			unsigned indirect:1;//间接传输
		}bits;
	}options;		
}MAC_TX_DATA;

typedef struct _GTS_Allocate{
	SADDR address1;
	BYTE GTSInformation;
	BYTE gtsDirection;
}GTSAllocate;

//用于上层给下层传递参数，以后完善
typedef union _MAC_ARGS {
  struct {
    LADDR DeviceAddress;
    BYTE DisassociateReason;
    BOOL SecurityEnable;
  }disassoc_req;
  union {
    struct {
      SCAN_TYPE_ENUM ScanType;
      UINT32 ScanChannels;
      BYTE ScanDuration;
    }request;
    struct {
      SCAN_TYPE_ENUM ScanType;
      UINT32 UnscanChannels;
      BYTE ResultListSize;
    }confirm;
  }scan;
  struct {
    union  {
      BYTE val;
      struct {
        unsigned PANCoordinator:1;//是否作为一个新的PAN的协调器
        unsigned BatteryLifeExtension:1;//
        unsigned CoordRealignment:1;//
        unsigned SecurityEnable:1;//
      }bits;
    }options;
    UINT16 PANID;
    BYTE LogicalChannel;
    //UINT32 StartTime;
    BYTE BeaconOrder;
    BYTE SuperframeOrder;
  }start;
  union {
    struct {
      BYTE LogicalChannel;
      BYTE CoordAddrMode;
      UINT16 CoordPANID;
      LADDR_UNION CoordAddress;	
      BYTE SecurityEnable;
      BYTE CapabilityInformation;
    }request;
    struct {
      SADDR AssocShortAddress;

    }confirm;
  }associate;
  struct {
    BOOL SetDefaultPIB;
  }reset;
  struct {
    BYTE LogicalChannel;
  }beacon_req;
  struct {
    LRWPAN_STATUS_ENUM status;
  }error;//出错状态
  struct {
    SADDR saddr;
  }ping_node;
  struct{
    BYTE requestLength;
    BOOL requestDirection;
    BOOL requestCharacter;
  }GTSRequest;
}MAC_ARGS;



//MAC状态：空闲，命令开始，一个的传输等待、一个的传输等待和锁定、孤立设备通知、孤立设备等待、
//关联请求等待、发送信标响应、发送关联响应
typedef enum _MAC_STATE_ENUM {
  MAC_STATE_IDLE,
  MAC_STATE_COMMAND_START
 } MAC_STATE_ENUM;

//MAC服务机构体：服务名称、ARGS、状态
typedef struct _MAC_SERVICE {
  LRWPAN_SVC_ENUM cmd;//命令接口
  MAC_ARGS args;//参数接口
  LRWPAN_STATUS_ENUM status;//返回状态接口
}MAC_SERVICE;

typedef struct _MAC_SYNC_TIME{
	UINT32 ReceiveTime;UINT32 ReceiveTime_1;
	UINT32 TS1;UINT32 TS1_1;
	UINT32 TM1;UINT32 TM1_1;
	UINT32 TM2;UINT32 TM2_1;
	UINT32 TS2;UINT32 TS2_1;
	UINT32 Offset;UINT32 Offset_1;
}MAC_SYNC_TIME;

//MAC层接收状态枚举
typedef enum _MAC_RXSTATE_ENUM {
  MAC_RXSTATE_IDLE,
  MAC_RXSTATE_NWK_HANDOFF,
  MAC_RXSTATE_CMD_PENDING
} MAC_RXSTATE_ENUM;

extern MAC_SYNC_TIME mac_sync_time;
extern MAC_PIB mac_pib;
extern MAC_SERVICE a_mac_service;
extern MAC_STATE_ENUM macState;
extern MAC_TX_DATA a_mac_tx_data;
extern MAC_RX_DATA a_mac_rx_data;


void macInitAckPendingTable(void);
void macResetAckPendingTable(void);
void macInitRxBuffer(void);
void macResetRxBuffer(void);
void macInit(void);
void macFSM(void);

LRWPAN_STATUS_ENUM macInitRadio(BYTE channel, UINT16 panid);
LRWPAN_STATUS_ENUM macWarmStartRadio(void);
void macSetPANID(UINT16 panid);
void macSetChannel(BYTE channel);
void macSetShortAddr(UINT16 saddr);
//local functions
void macApplyAckTable(BYTE *ptr, BYTE flen);
void macTxData(void);
void macTxFSM(void);
static void macParseHdr(void);
void macRxFSM(void);
BOOL macRxBuffFull(void);
BOOL macRxBuffEmpty(void);
MACPKT *macGetRxPacket(void);
void macFreeRxPacket(BOOL freemem);
static BOOL macCheckLaddrSame(LADDR *laddr1,LADDR *laddr2);

static void macParseBeacon(void);

static void macFormatDiassociateRequest(void);
void macSendDisassociateRequest(void);
static BOOL macCheckDataRejection(void);
void macFormatOrphanNotify(void);

void macFormatBeaconRequest(void);
void macSendBeaconRequest(void);

//网络连接请求函数
//#ifndef LRWPAN_COORDINATOR
void macFormatAssociateRequest(void);
void macSendAssociateRequest(void);
void macNetworkAssociateRequest(void);
//#endif
//发送孤立通告命令
void macSendOrfhanNotify(void);

UINT8 macRadioComputeED(INT8 rssiDbm);
BYTE macRadioComputeLQI(UINT8 corr);
void macScan(void);
BOOL macGetPANInfo(BYTE logicalchannel,BYTE beacomnum);
BOOL macGetCoordRealignInfo(void);

void macPibReset(void);

//#ifndef LRWPAN_COORDINATOR
static void macParseOrphanResponse(void);
//#endif


#ifdef LRWPAN_FFD
/*//mini
#ifdef LRWPAN_COORDINATOR
BOOL laddr_permit (LADDR *ptr);
#endif
//mini*/
void macFormatBeacon(void);
void macSendBeacon(void);
void macNetworkAssociateResponse(void);
void macFormatAssociateResponse(void);
void macSendAssociateResponse(void);
void macFormatCoordRealign(SADDR orphan_saddr);
void macSendCoordRealign(SADDR orphan_saddr);
void macOrphanNotifyResponse(void);
void macStartNewSuperframe(void);
#endif

SADDR macParseAssocResponse(void);



#endif

