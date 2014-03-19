/*

*2006/08/16 WXL 2.0

*/
#ifndef APS_H
#define APS_H

#define APS_DSTMODE_NONE          0
#define APS_DSTMODE_SHORT         1
#define APS_DSTMODE_LONG          2

#define APS_FRM_TYPE_DATA 0
#define APS_FRM_TYPE_CMD  1
#define APS_FRM_TYPE_ACK  2
#define APS_FRM_TYPE_RSV  3
#define APS_FRM_TYPE_MASK 3






#define APS_GET_FRM_TYPE(x) (x&APS_FRM_TYPE_MASK)
#define APS_IS_CMD(x) ((APS_GET_FRM_TYPE(x)) == APS_FRM_TYPE_CMD)
#define APS_IS_DATA(x) ((APS_GET_FRM_TYPE(x)) == APS_FRM_TYPE_DATA)
#define APS_IS_ACK(x) ((APS_GET_FRM_TYPE(x)) == APS_FRM_TYPE_ACK)
#define APS_IS_RSV(x) ((APS_GET_FRM_TYPE(x)) == APS_FRM_TYPE_RSV)

#define APS_FRM_DLVRMODE_NORMAL    0
#define APS_FRM_DLVRMODE_INDIRECT  (1<<2)
#define APS_FRM_DLVRMODE_BCAST     (2<<2)
#define APS_FRM_DLVRMODE_RSV       (3<<2)
//#define APS_FRM_DLVRMODE_GROUP       (3<<2)
#define APS_FRM_DLVRMODE_MASK      (3<<2)
#define APS_GET_FRM_DLVRMODE(x)    (x&APS_FRM_DLVRMODE_MASK)

//The indirect submode bit is used for indirect frames
//and distiguishes before and after the indirect binding is resolved.
//if the submode is '1', only the srcEP is included and the packet
//is traveling to the coordinator to get the binding resolved.
//If the submode is '1', only the dstEP is included and the packet
//is traveling to the destination EP.

#define APS_FRM_INDIRECT_SUBMODE_MASK      (1<<4)
#define APS_GET_FRM_INDIRECT_SUBMODE(x)    (x&(APS_FRM_INDIRECT_SUBMODE_MASK))
#define APS_FRM_SECURITY_MASK      (1<<5)
#define APS_GET_FRM_SECURITY(x)        (x&(APS_FRM_SECURITY_MASK))
#define APS_FRM_ACKREQ_MASK        (1<<6)
#define APS_GET_FRM_ACKREQ(x)        (x&(APS_FRM_ACKREQ_MASK))

#define AF_TRANS_COUNT_MASK  (0xF)
#define AF_GET_TRANS_COUNT(x) (x &(AF_TRANS_COUNT_MASK))

#define AF_FRM_TYPE_MASK   (0xF <<4)
#define AF_GET_FRM_TYPE(x) (x & (AF_FRM_TYPE_MASK))

#define AF_FRM_TYPE_KVP  (1 <<4)
#define AF_FRM_TYPE_MSG  (2 <<4)

#define APS_RXBUFF_SIZE LRWPAN_MAX_INDIRECT_RX_PKTS//2

#define apsTXIdle() (!aps_pib.flags.bits.TxInProgress)
#define apsTXBusy() (aps_pib.flags.bits.TxInProgress)
#define apsSetTxBusy() aps_pib.flags.bits.TxInProgress = 1
#define apsSetTxIdle() aps_pib.flags.bits.TxInProgress = 0
#define apsIdle() (apsState == APS_STATE_IDLE)
#define apsBusy() (apsState != APS_STATE_IDLE)

#define apsDoService() \
   a_aps_service.status = LRWPAN_STATUS_APS_INPROGRESS;\
   apsState = APS_STATE_COMMAND_START;\
   apsFSM();

#define aplInit()               apsInit()
#define apsGenTSN()             (aps_pib.apsTSN++)

#define aplGetRxDstEp()         (a_aps_rx_data.dstEP)
#define aplGetRxCluster()       (a_aps_rx_data.cluster)
#define aplGetRxSrcEp()         (a_aps_rx_data.srcEP)
#define aplGetRxSrcSADDR()      (a_aps_rx_data.srcSADDR)
#define aplGetRxMsgLen()        (a_aps_rx_data.usrPlen)
#define aplGetRxMsgData()       (a_aps_rx_data.usrPload)
#define aplGetRxRSSI()          (a_aps_rx_data.orgpkt.rssi)

#define aplGetLastTxTime()     (phy_pib.txStartTime)

#define aplGetStatus()            (a_aps_service.status)
#define aplGetMyShortAddress()    (macGetShortAddr())
#define aplGetParentLongAddress() (&mac_pib.macParentExtendedAddress)
#define aplGetParentShortAddress() (mac_pib.macParentShortAddress)

#define aplSetMacMaxFrameRetries(x)  (mac_pib.macMaxAckRetries=x)
#define aplSetApsMaxFrameRetries(x)  (aps_pib.apscMaxFrameRetries=x)

//indicates when the user buffer has been copied and is now free
#define aplIsUsrBufferFree()  (aps_pib.flags.bits.IsUsrBufferFree == 1)

#define aplSendMSG(dstMode, dstADDR, dstEP, cluster,srcEP,pload, plen, tsn, reqack)\
	aplFmtSendMSG(dstMode, dstADDR, dstEP, cluster,srcEP,pload, plen, tsn, reqack);\
	apsDoService();

#define aplSendEndDeviceAnnounce(saddr)\
      while(apsBusy()) apsFSM();\
      a_aps_service.cmd = LRWPAN_SVC_APS_DO_ZEP_TX;\
      a_aps_service.args.zep_tx.clusterID = ZEP_END_DEVICE_ANNOUNCE;\
      a_aps_service.args.zep_tx.dst = saddr;\
      apsDoService();

#define aplSendNodeInfo(saddr)\
      while(apsBusy()) apsFSM();\
      a_aps_service.cmd = LRWPAN_SVC_APS_DO_ZEP_TX;\
      a_aps_service.args.zep_tx.clusterID = ZEP_EXTENDED_CMD;\
      a_aps_service.args.zep_tx.extID = ZEP_EXT_NODE_INFO_RSP;\
      a_aps_service.args.zep_tx.dst = saddr;\
      apsDoService();

#define aplSendAlarm(saddr,mde)\
      while(apsBusy()) apsFSM();\
      a_aps_service.cmd = LRWPAN_SVC_APS_DO_ZEP_TX;\
      a_aps_service.args.zep_tx.clusterID = ZEP_EXTENDED_CMD;\
      a_aps_service.args.zep_tx.extID = ZEP_EXT_SEND_ALARM;\
      a_aps_service.args.zep_tx.dst = saddr;\
      a_aps_service.args.zep_tx.ext.alarm.mode = mde;\
      apsDoService();

#define aplPingParent()\
      while(apsBusy()) apsFSM();\
      a_aps_service.cmd = LRWPAN_SVC_APS_DO_ZEP_TX;\
      a_aps_service.args.zep_tx.clusterID = ZEP_EXTENDED_CMD;\
      a_aps_service.args.zep_tx.extID = ZEP_EXT_PING;\
      a_aps_service.args.zep_tx.dst = mac_pib.macCoordShortAddress;\
      apsDoService();

#define aplPingNode(saddr)\
      while(apsBusy()) apsFSM();\
      a_aps_service.cmd = LRWPAN_SVC_APS_DO_ZEP_TX;\
      a_aps_service.args.zep_tx.clusterID = ZEP_EXTENDED_CMD;\
      a_aps_service.args.zep_tx.extID = ZEP_EXT_PING;\
      a_aps_service.args.zep_tx.dst = saddr;\
      apsDoService();

#ifdef LRWPAN_COORDINATOR
//only the coordinator can form a network
//example of a passthru from APS to NETWORK
#define aplFormNetwork()\
    while(apsBusy()) apsFSM();\
	a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;\
	a_nwk_service.cmd = LRWPAN_SVC_NWK_FORM_NETWORK;\
	a_nwk_service.args.form_network.ScanChannels = LRWPAN_DEFAULT_CHANNEL_MASK;\
	a_nwk_service.args.form_network.ScanDuration = LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION;\
	a_nwk_service.args.form_network.PANID = LRWPAN_DEFAULT_PANID;\
	apsDoService();

#else
//router, RFDs can join a formed network
//#define aplJoinNetwork()
#define aplRejoinNetwork()

///*此处正在更改，以后更新
#define aplJoinNetwork()\
    while(apsBusy()) apsFSM();\
	a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;\
	a_nwk_service.cmd = LRWPAN_SVC_NWK_JOIN_NETWORK;\
	a_nwk_service.args.join_network.ScanChannels = LRWPAN_DEFAULT_CHANNEL_MASK;\
	a_nwk_service.args.join_network.ScanDuration = LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION;\
	a_nwk_service.args.join_network.PANID = LRWPAN_DEFAULT_PANID;\
	apsDoService();
/*此处正在更改，以后更新
#define aplRejoinNetwork()\
    while(apsBusy()) apsFSM();\
	a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;\
	a_nwk_service.cmd = LRWPAN_SVC_NWK_JOIN_NETWORK;\
	a_nwk_service.args.join_network.ScanChannels = LRWPAN_DEFAULT_CHANNEL_MASK;\
	a_nwk_service.args.join_network.ScanDuration = LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION;\
	a_nwk_service.args.join_network.PANid = LRWPAN_DEFAULT_PANID;\
	a_nwk_service.args.join_network.RejoinNetwork = TRUE;\
	apsDoService();

*/
#endif

//add now
#define aplLeaveNetwork()\
	while(apsBusy()) apsFSM();\
	a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;\
	a_nwk_service.cmd = LRWPAN_SVC_NWK_LEAVE_NETWORK;\
	a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
        a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
        a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
        a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
        a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
        a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
        a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
        a_nwk_service.args.leave_network.DeviceAddress.bytes[0] = 0;\
	apsDoService();

#define aplDiscoveryNetwork()\
  while(apsBusy()) apsFSM();\
  a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;\
  a_nwk_service.cmd = LRWPAN_SVC_NWK_DISC_NETWORK;\
  a_nwk_service.args.discovery_network.request.ScanChannels = LRWPAN_DEFAULT_CHANNEL_MASK;\
  a_nwk_service.args.discovery_network.request.ScanDuration = LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION;\
  apsDoService();

#define aplGTSRequest(length, direction, character)	\
	while(apsBusy()) apsFSM();\
	a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;\
	a_nwk_service.cmd = LRWPAN_SVC_NWK_GTS_REQ;\
	a_nwk_service.args.GTSRequest.requestLength=length;\
	a_nwk_service.args.GTSRequest.requestDirection=direction;\
	a_nwk_service.args.GTSRequest.requestCharacter=character;\
	apsDoService();

//right now, all this structure has in it is the endpoint number.
typedef struct _APS_EP_ELEM {
  BYTE epNum;
}APS_EP_ELEM;

typedef enum _APS_RXSTATE_ENUM {
  APS_RXSTATE_IDLE,
  APS_RXSTATE_START,
  APS_RXSTATE_RESOLVE_INDIRECT,
  APS_RXSTATE_ACK_SEND_WAIT
} APS_RXSTATE_ENUM;

typedef struct _APS_RX_DATA {
  MACPKT orgpkt;
  BYTE apsOffset;
  BYTE afOffset;
  union _APS_RX_DATA_FLAGS{
    BYTE val;
    struct {
      unsigned srcEP:1;
      unsigned dstEP:1;
    }bits;
  }flags;
  //parse these out of the packet for reference
  BYTE dstEP;
  BYTE srcEP;
  BYTE cluster;
  UINT16 profile;
  SADDR dstSADDR;   //used for indirect msgs
  SADDR srcSADDR;
  BYTE af_fcf;
  BYTE aps_fcf;
  //these fields are for AF MSG data
  BYTE tsn;
  BYTE *usrPload;
  BYTE usrPlen;
}APS_RX_DATA;

typedef struct _APS_TX_DATA {
  union _APS_TX_DATA_FLAGS{
    BYTE val;
    struct {
      unsigned loopback:1;
    }bits;
  }flags;
  BYTE aps_fcf;
  BYTE af_fcf;
  BYTE dstEP;
  BYTE dstMode;
  SADDR dstSADDR;
  BYTE *dstLADDR;
  SADDR srcSADDR;
  BYTE cluster;
  BYTE srcEP;
  BYTE  tsn;
  BYTE aps_flen;  //used for retries.
  BYTE *aps_ptr;  //used for retries
  BYTE *usrPload;
  BYTE usrPlen;
}APS_TX_DATA;

//arguments for extended ZEP commands
typedef union _ZEP_EXT_ARGS {
  struct {
    BYTE   mode;
  }alarm;
}ZEP_EXT_ARGS;

typedef union _APS_ARGS {
  struct {
    BYTE   clusterID;
    BYTE   extID;
    SADDR  dst;      //destination for this command
    ZEP_EXT_ARGS ext;
  }zep_tx;
}APS_ARGS;

typedef enum _APS_STATE_ENUM {
  APS_STATE_IDLE,
  APS_STATE_COMMAND_START,
  APS_STATE_GENERIC_TX_WAIT,
  APS_STATE_NWK_PASSTHRU_WAIT,
  APS_STATE_INDIRECT_GETDST,
  APS_STATE_INDIRECT_TX,
#ifdef LRWPAN_COORDINATOR
  APS_STATE_INJECT_INDIRECT,
#endif
  APS_STATE_ACK_SEND_START,
  APS_STATE_INDIRECT_TX_WAIT,
  APS_STATE_INJECT_LOOPBACK,
  APS_STATE_INDIRECT_LOOPBACK
} APS_STATE_ENUM;


typedef struct _APS_SERVICE {
  LRWPAN_SVC_ENUM cmd;
  APS_ARGS args;
  LRWPAN_STATUS_ENUM status;
}APS_SERVICE;


typedef struct _APS_PIB{
  union _APS_PIB_FLAGS{
    BYTE val;
    struct {
      unsigned indirectPending:1;
      unsigned ackPending:1;         //expecting an ACK
      unsigned ackSendPending:1;     //we need to send an APS ack!
      unsigned TxInProgress:1;       //TX in progress
      unsigned IsUsrBufferFree:1;
    }bits;
  }flags;
  BYTE activeEPs;          // num of active EPs
  UINT32 tx_start_time;
  BYTE apsTSN; //transaction sequence number
  UINT32 apscAckWaitDuration; // in mac TICs
  UINT16 apsAckWaitMultiplier;
  UINT16 apsAckWaitMultiplierCntr;
  BYTE apscMaxFrameRetries;
  BYTE currentAckRetries;
#ifdef LRWPAN_COORDINATOR
  BYTE rxCnt;          //number of packets currently in buffer
  BYTE rxTail;        //tail pointer for buffer
  BYTE rxHead;        //head pointer for buffer
  //fifo for RX pkts, holds LRWPAN_MAX_NWK_RX_PKTS
  APS_RX_DATA  rxBuff[APS_RXBUFF_SIZE];  //buffer for APS packets to be redirected
#endif
}APS_PIB;

//only use for this right now is to allow user to
//regiser endpoints with the APS level so that they
//can be reported to the coordinator.
extern APS_EP_ELEM apsEndPoints[LRWPAN_MAX_ENDPOINTS];//6
extern APS_SERVICE a_aps_service;
extern APS_TX_DATA a_aps_tx_data;
extern APS_STATE_ENUM apsState;
extern APS_PIB aps_pib;
extern APS_RX_DATA a_aps_rx_data;

extern  APS_RXSTATE_ENUM apsRxState;
extern LRWPAN_STATUS_ENUM apsTxFSM_status;


void apsFSM(void);
void apsInit(void);


BOOL apsRxBusy(void);
void apsRxHandoff(void);

LRWPAN_STATUS_ENUM usrRxPacketCallback(void);
LRWPAN_STATUS_ENUM aplRegisterEndPoint(BYTE epNum);

void aplShutdown(void);
void aplWarmstart(void);

void apsFSM(void);
void apsTxData(BOOL copy_payload);

static void apsParseHdr(BYTE *ptr);
static void apsRxFSM(void);
static BOOL apsCheckAck(void);
static void apsFormatAck(void);
static void apsTxFSM(void);
static void apsInjectPacket(BOOL indirect_flag);
static void apsInjectTxPacket(void);

#ifdef LRWPAN_COORDINATOR
void apsRxBuffInit(void);
BOOL apsRxBuffFull(void);
BOOL apsRxBuffEmpty(void);
APS_RX_DATA *apsGetRxPacket(void);
void apsFreeRxPacket(BOOL freemem);
void apsRxBuffAdd (APS_RX_DATA *ptr);
#endif

void aplFmtSendMSG (BYTE dstMode,
				 LADDR_UNION *dstADDR,
				 BYTE dstEP,
				 BYTE cluster,
				 BYTE srcEP,
				 BYTE* pload,
				 BYTE  plen,
				 BYTE  tsn,
				 BYTE  reqack);

UINT32 aplMacTicksToUs(UINT32 ticks);   //utility function

#endif


