#include "compiler.h"
#include "lrwpan_config.h"         //user configurations
#include "lrwpan_common_types.h"   //types common acrosss most files
#include "ieee_lrwpan_defs.h"
#include "memalloc.h"
#include "hal.h"
#include "halStack.h"
#include "phy.h"
#include "mac.h"
#include "nwk.h"
#include "neighbor.h"
/*
#define MAC_GLOBAL
#ifdef MAC_GLOBAL
#define MAC_EXTERN
#else
#define MAC_EXTERN extern
#endif
*/
static void macFormatGTSRequest(void);
static void macFormatSyncRequest(void);
static void macParseSyncRsp(void);
#ifdef LRWPAN_COORDINATOR
static void macFormatSyncRsp(void);
static void macParseGTSRequest(void);
#endif

MAC_SYNC_TIME mac_sync_time;
MAC_RXSTATE_ENUM macRxState;
MAC_PIB mac_pib;
MAC_SERVICE a_mac_service;
MAC_STATE_ENUM macState;
GTSAllocate gtsAllocate[7];

ACK_PENDING_TABLE AckPendingTable[MaxAckWindow];
BYTE EnergyDetectList[MAC_SCAN_SIZE];
SCAN_PAN_INFO PANDescriptorList[MAC_SCAN_SIZE];

//there can only be one TX in progress at a time, so a_mac_tx_data contains the arguments for that TX.
MAC_TX_DATA a_mac_tx_data;

MAC_RX_DATA a_mac_rx_data;//this is used for parsing of current packet.

LRWPAN_STATUS_ENUM macTxFSM_status;

UINT32 mac_utility_timer;   //utility timer

/*函数功能:初始化AckPendingTable*/
void macInitAckPendingTable(void)
{
  BYTE i;
  for (i=0;i<MaxAckWindow;i++) {
    AckPendingTable[i].options.bits.Used = 0;
  }
}

/*函数功能:复位AckPendingTable，释放缓存数据的内存，清标志位*/
void macResetAckPendingTable(void)
{
  BYTE i;
  for (i=0;i<MaxAckWindow;i++) {
    if (AckPendingTable[i].options.bits.Used) {
      MemFree(AckPendingTable[i].Ptr);
      AckPendingTable[i].options.bits.Used = 0;
      AckPendingTable[i].options.bits.ACKPending = 0;
    }
  }

}

/*函数功能:初始化MAC层接收缓存器*/
void macInitRxBuffer(void)
{
  mac_pib.rxTail = 0;
  mac_pib.rxHead = 0;
}

/*函数功能:复位MAC层接收缓存器，如果为空，复位指针；如果非空，先释放内存再复位指针*/
void macResetRxBuffer(void)
{
  if (macRxBuffEmpty()) {
    mac_pib.rxTail = 0;
    mac_pib.rxHead = 0;
  } else {
    while (1) {
      macGetRxPacket();
      macFreeRxPacket(TRUE);
      if (macRxBuffEmpty()) {
        mac_pib.rxTail = 0;
        mac_pib.rxHead = 0;
        break;
      }
    }
  }

}


//does not turn on radio.
/*函数功能:MAC初始化，涉及变量 macState,macRxState,mac_pib,没有打开radio*/
void macInit(void){
  macState = MAC_STATE_IDLE;
  macRxState = MAC_RXSTATE_IDLE;
  mac_pib.macCoordShortAddress = 0;
  mac_pib.flags.val = 0;
  mac_pib.macCurrentSlot = 0;
  mac_pib.macPANID = LRWPAN_DEFAULT_PANID;//0x1347，该参数在组网过程中应该更改
  mac_pib.macMaxAckRetries = aMaxFrameRetries;//3
  mac_pib.macShortAddress = LRWPAN_BCAST_SADDR;
  macInitRxBuffer();
  macInitAckPendingTable();
#ifdef LRWPAN_COORDINATOR
  mac_pib.macDepth = 0;
  mac_pib.flags.bits.macPanCoordinator = 1;
  mac_pib.flags.bits.macAssociationPermit=1;
  mac_pib.flags.bits.macGTSPermit=1;
#else
  mac_pib.macDepth = 1; //depth will be at least one
  mac_pib.flags.bits.macPanCoordinator = 0;
  mac_pib.flags.bits.macAssociationPermit=0;
  mac_pib.flags.bits.macGTSPermit=0;
#endif
  mac_sync_time.ReceiveTime=0;
  mac_sync_time.ReceiveTime_1=0;
  mac_sync_time.Offset=0;
  mac_sync_time.Offset_1=0;
  mac_sync_time.TM1=0;
  mac_sync_time.TM2=0;
  mac_sync_time.TS1=0;
  mac_sync_time.TS2=0;
  mac_sync_time.TM1_1=0;
  mac_sync_time.TM2_1=0;
  mac_sync_time.TS1_1=0;
  mac_sync_time.TS2_1=0;
  mac_pib.finalSlot=15;
  mac_pib.GTSDescriptCount=0;
  mac_pib.GTSDirection=0;
  mac_pib.pendingSaddrNumber=0;
  mac_pib.pendingLaddrNumber=0;
  mac_pib.corGTSPermit=0;
  mac_pib.corAssociationPermit=0;
  mac_pib.localGTSLength=0;
  mac_pib.localGTSSlot=0;
  mac_pib.localdirection=0;
  mac_pib.bcnDepth = 0xFF; //remembers depth of node that responded to beacon other capability information
  mac_pib.macDSN = halGetRandomByte();
  mac_pib.macBSN = halGetRandomByte();
  mac_pib.macCapInfo = 0;
#ifdef LRWPAN_ALT_COORDINATOR     //not supported, included for completeness
  LRWPAN_SET_CAPINFO_ALTPAN(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_FFD
  LRWPAN_SET_CAPINFO_DEVTYPE(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_ACMAIN_POWERED
  LRWPAN_SET_CAPINFO_PWRSRC(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_RCVR_ON_WHEN_IDLE
  LRWPAN_SET_CAPINFO_RONIDLE(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_SECURITY_CAPABLE
  LRWPAN_SET_CAPINFO_SECURITY(mac_pib.macCapInfo);
#endif
  //always allocate a short address
  LRWPAN_SET_CAPINFO_ALLOCADDR(mac_pib.macCapInfo);


}



/*函数功能:RF部分的暖启动，物理层初始化，设置信道，PINID和短地址*/
LRWPAN_STATUS_ENUM macWarmStartRadio(void){
 halWarmstart();
 a_phy_service.cmd = LRWPAN_SVC_PHY_INIT_RADIO; //no args
  a_phy_service.args.phy_init_radio_args.radio_flags.bits.listen_mode = 0;
#ifdef LRWPAN_COORDINATOR
  a_phy_service.args.phy_init_radio_args.radio_flags.bits.pan_coordinator = 1;
#else
  a_phy_service.args.phy_init_radio_args.radio_flags.bits.pan_coordinator = 0;
#endif
  phyDoService();
  halSetChannel(phy_pib.phyCurrentChannel);
  halSetRadioPANID(mac_pib.macPANID); //listen on this PANID
  halSetRadioShortAddr(macGetShortAddr());  //non-broadcast, reserved
  return(a_phy_service.status);
}

//this assumes that phyInit, macInit has previously been called.
//turns on the radio
/*函数功能:MAC层初始化RF函数，包括频段，信道等参数设置*/
LRWPAN_STATUS_ENUM macInitRadio(BYTE channel, UINT16 panid) {

	phy_pib.phyCurrentChannel = channel;
	phy_pib.phyCurrentFrequency =LRWPAN_DEFAULT_FREQUENCY;
	if (phy_pib.phyCurrentChannel < 11){
		mac_pib.macAckWaitDuration = SYMBOLS_TO_MACTICKS(400);
	}
	else {
		//mac_pib.macAckWaitDuration = SYMBOLS_TO_MACTICKS(54);
                mac_pib.macAckWaitDuration = SYMBOLS_TO_MACTICKS(400);
	}
	
	a_phy_service.cmd = LRWPAN_SVC_PHY_INIT_RADIO; //no args
	a_phy_service.args.phy_init_radio_args.radio_flags.bits.listen_mode = 0;
#ifdef LRWPAN_COORDINATOR
	a_phy_service.args.phy_init_radio_args.radio_flags.bits.pan_coordinator = 1;
#else
	a_phy_service.args.phy_init_radio_args.radio_flags.bits.pan_coordinator = 0;
#endif

	phyDoService();

	halSetRadioPANID(panid);      //broadcast
	
	halSetRadioShortAddr(0xFFFE);  //non-broadcast, reserved,有待思考
	return(a_phy_service.status);

}

/*函数功能:设置PINID，在 mac_pib和寄存器中*/
void macSetPANID(UINT16 panid){
  mac_pib.macPANID = panid;
  halSetRadioPANID(mac_pib.macPANID);
}

/*函数功能:在mac_pib中设置信道值，在寄存器中设置频率值*/
void macSetChannel(BYTE channel){
  phy_pib.phyCurrentChannel = channel;
  halSetChannel(channel);
}

/*函数功能:若是RFD，初始化地址表。将本设备的地址加到地址表，在寄存器中设置短地址*/
void macSetShortAddr(UINT16 saddr) {
  BYTE i,laddr[8];
  halGetProcessorIEEEAddress(&laddr[0]);//取扩展地址，从flash存储器
  for (i=0;i<8;i++) {
    mac_pib.macExtendedAddress.bytes[i] = laddr[i];
  }
  mac_pib.macShortAddress = saddr;
  halSetRadioShortAddr(saddr);
}



void macFSM(void)
{

  BYTE cmd;


  phyFSM();

  macTxFSM();

  macRxFSM();

  switch (macState) {

  case MAC_STATE_IDLE:
    if (mac_pib.flags.bits.macPending ) {//在MAC_STATE_IDLE状态下，有命令挂起时，先处理
      cmd = *(a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset);//Get the command
      switch (cmd) {//很多命令没有做，如断开连接通告命令，数据请求命令，PAN ID冲突通告命令，调器重新同步命令，GTS分配和解除命令
      case LRWPAN_MACCMD_BCN_REQ://信标帧请求命令
#ifdef LRWPAN_RFD   //as an RFD, I do not handle this. Release this.
        mac_pib.flags.bits.macPending = 0;
#else
        //as a Coordinator or Router, I will only respond
        //only respond if association permitted
        //as this is the stack's only use of beacons
/*//mini
#ifdef LRWPAN_COORDINATOR
      if(laddr_permit(&a_mac_rx_data.SrcAddr.laddr)){
#endif
//mini*/
        if (mac_pib.flags.bits.macAssociationPermit) {
          if (phyTxUnLocked()) {
            phyGrabTxLock(); //grab the lock
            macSendBeacon();
            mac_pib.flags.bits.macPending = 0; //release packet
          }
        } else { //release packet.
          mac_pib.flags.bits.macPending = 0;
        }
/*//mini
#ifdef LRWPAN_COORDINATOR
      	}else{
      	 mac_pib.flags.bits.macPending = 0;
      		}
#endif
//mini*/
#endif
        break;

	case LRWPAN_MACCMD_SYNC_REQ:
	  #ifdef LRWPAN_COORDINATOR
          if (phyTxUnLocked()){
          phyGrabTxLock();
	  macFormatSyncRsp();
	  mac_pib.flags.bits.macPending = 0;
	  }
	  #endif
          break;

      case LRWPAN_MACCMD_ORPHAN://孤立通告命令
#ifdef LRWPAN_RFD
          //as an RFD, I do not handle this. Release this.
          mac_pib.flags.bits.macPending = 0;
#else
          //will keep spinning through here until TX buffer unlocked
          if (phyTxUnLocked()) {
            phyGrabTxLock(); //grab the lock
            macOrphanNotifyResponse();
            mac_pib.flags.bits.macPending = 0; //release packet
          }
#endif
          break;
      case LRWPAN_MACCMD_ASSOC_REQ://连接请求命令
#ifdef LRWPAN_RFD
        //as an RFD, I do not handle this. Release this.
        mac_pib.flags.bits.macPending = 0;
#else
        //as a Coordinator or Router, I can respond only respond if association permitted
        if (mac_pib.flags.bits.macAssociationPermit) {
          //will keep spinning through here until TX buffer unlocked
          if (phyTxUnLocked()) {
            phyGrabTxLock(); //grab the lock
            macNetworkAssociateResponse();
            mac_pib.flags.bits.macPending = 0;
          }
        }else {
          mac_pib.flags.bits.macPending = 0;
        }
#endif
        break;
      case LRWPAN_MACCMD_DISASSOC:
#ifdef LRWPAN_RFD
        if (macCheckLaddrSame(&a_mac_rx_data.SrcAddr.laddr, &mac_pib.macCoordExtendedAddress)) {
          //RFD设备收到断开网络命令，先检查是否是父设备发送的，若是则处理相关标志位和信息，以后待改进
          mac_pib.flags.bits.macIsAssociated = 0;
          mac_pib.flags.bits.macPending = 0;
          //DEBUG_STRING(DBG_INFO,"MAC: Recieved MAC CMD that is LRWPAN_MACCMD_DISASSOC.\n");
          break;
        } else {
          mac_pib.flags.bits.macPending = 0;
          //DEBUG_STRING(DBG_ERR, "MAC: Recieved MAC CMD that is LRWPAN_MACCMD_DISASSOC, which is not form the coordinator.\n");
          break;
        }
#else
        if (*(a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset+1)==DEVICE_DISASSOC_WITH_NETWORK) {//从设备想离开该网络
          if (ntDelNbrByLADDR(&a_mac_rx_data.SrcAddr.laddr)) {
            mac_pib.flags.bits.macPending = 0;
            //DEBUG_STRING(DBG_INFO, "MAC: Recieved MAC CMD that is LRWPAN_MACCMD_DISASSOC, which is a PAN RFD or FFD.\n");
            break;
          } else {
            mac_pib.flags.bits.macPending = 0;
            //DEBUG_STRING(DBG_INFO, "MAC: Recieved MAC CMD that is LRWPAN_MACCMD_DISASSOC, which is not a PAN RFD or FFD.\n");
            break;
          }
        } else {//父设备强迫自己离开网络
          if (macCheckLaddrSame(&a_mac_rx_data.SrcAddr.laddr, &mac_pib.macCoordExtendedAddress)) {
            mac_pib.flags.bits.macPending = 0;
            mac_pib.flags.bits.macIsAssociated = 0;
            //DEBUG_STRING(DBG_INFO,"MAC: Recieved MAC CMD that is LRWPAN_MACCMD_DISASSOC,which is from the coordinator\n");
            //清除路由表,邻居表，和地址表
            ntInitTable();
            //MAC层复位
            macPibReset();
            break;
          } else {
            mac_pib.flags.bits.macPending = 0;
            //DEBUG_STRING(DBG_ERR, "MAC: Recieved MAC CMD that is LRWPAN_MACCMD_DISASSOC, which is not form the coordinator.\n");
            break;
          }
        }
#endif
      default:
        //DEBUG_STRING(1,"MAC: Received MAC CMD that is not currently implemented, discarding.\n");
        mac_pib.flags.bits.macPending = 0;
      }
    }
    break;

  case MAC_STATE_COMMAND_START://在MAC_STATE_COMMAND_START状态下，执行a_mac_service.cmd，一般是由上层发起的
    switch(a_mac_service.cmd) {
    case LRWPAN_SVC_MAC_ERROR://dummy service, just return the status that was passed in
      a_mac_service.status = a_mac_service.args.error.status;
      macState = MAC_STATE_IDLE;
      break;
    case LRWPAN_SVC_MAC_GENERIC_TX:
      macTxData();
      macState = MAC_STATE_IDLE;
      break;
    case LRWPAN_SVC_MAC_ORPHAN_NOTIFY:
      if (phyTxLocked())
        break;
      phyGrabTxLock();  //Grab the lock
      macSendOrfhanNotify();
      macState = MAC_STATE_IDLE;
      break;
    case LRWPAN_SVC_MAC_BEACON_REQ:
      if (phyTxLocked())
        break;
      phyGrabTxLock();  //Grab the lock
      macSendBeaconRequest();
      macState = MAC_STATE_IDLE;
      break;

    case LRWPAN_SVC_MAC_SYNC_REQ:
       if (phyTxLocked())
        break;
       phyGrabTxLock();
       macFormatSyncRequest();
       macTxData();
       macState = MAC_STATE_IDLE;
       break;
    case LRWPAN_SVC_MAC_GTS_REQ:
       if (phyTxLocked())
        break;
       macFormatGTSRequest();
       macTxData();
       macState = MAC_STATE_IDLE;
       break;

//#ifndef LRWPAN_COORDINATOR
           case LRWPAN_SVC_MAC_ASSOC_REQ:
             if (phyTxLocked()) {

               break;
             }
             phyGrabTxLock();  //Grab the lock
             macNetworkAssociateRequest();
             macState = MAC_STATE_IDLE;	
             break;
//#endif
           case LRWPAN_SVC_MAC_DISASSOC_REQ:
               if (phyTxLocked()) break;
               phyGrabTxLock();
               macSendDisassociateRequest();
               macState = MAC_STATE_IDLE;	
               break;
           case LRWPAN_SVC_MAC_RESET_REQ:
             if (mac_pib.flags.bits.macIsAssociated) {
               a_mac_service.status = LRWPAN_STATUS_MAC_NOT_DISASSOCIATED;
               //DEBUG_STRING(DBG_ERR,"Reset failed, it is not disassociated from network!");
             } else
               macPibReset();
             macState = MAC_STATE_IDLE;	
             break;

           case LRWPAN_SVC_MAC_SCAN_REQ:
             macScan();
             macState = MAC_STATE_IDLE;	
             break;
#ifdef LRWPAN_FFD
           case LRWPAN_SVC_MAC_START_REQ:
             if (phyTxLocked()) break;
               phyGrabTxLock();
             macStartNewSuperframe();
             macState = MAC_STATE_IDLE;	
             break;
#endif
           default: break;
           }
           break;
	 default: break;
  }
}


//called by HAL when TX for current packet is finished
/*函数功能:判断是否需要确认帧，若要，则设置标志位和记录发送时间，在底层发送数据程序中调用*/
/*
void macTxCallback(void) {
  if (LRWPAN_GET_ACK_REQUEST(*(phy_pib.currentTxFrm))) {
    mac_pib.flags.bits.ackPending = 1;  //we are requesting an ack for this packet
    //record the time of this packet
    mac_pib.tx_start_time = halISRGetMACTimer();
  }
}
*/

//we pass in the pointer to the packet
//first byte is packet length
/*函数功能:判断物理层接收的数据包是否为确认帧(通通过帧长度，帧类型和序列号)，若是，取消标志位*/
/*         若不是，返回指向数据的指针和RSSI，在接收完毕中断中调用*/
void macRxCallback(BYTE *ptr, INT8 rssi, UINT8 corr) {
  BYTE i,ackdsn;

  //if this is an ACK, update the current timeout, else indicate
  // that an ACK is pending
  //   check length                      check frame control field
  ackdsn = *(ptr+3);
  if ((*ptr == LRWPAN_ACKFRAME_LENGTH ) && LRWPAN_IS_ACK(*(ptr+1))) {
    //do not save ACK frames
    //this is an ACK, see if it is our ACK, check DSN
    //DEBUG_STRING(DBG_INFO, "Receive a ACK,   DSN : ");
    //DEBUG_UINT8(DBG_INFO, ackdsn);
    //DEBUG_STRING(DBG_INFO, "   \n");

    for (i=0;i<MaxAckWindow;i++) {
      if (AckPendingTable[i].options.bits.Used) {
        if (ackdsn==AckPendingTable[i].DSN) {
          AckPendingTable[i].options.bits.ACKPending = 0;
          //DEBUG_STRING(DBG_INFO, "Receive a ACK,DSN : ");
          //DEBUG_UINT8(DBG_INFO, ackdsn);
          //DEBUG_STRING(DBG_INFO, "\n");
          break;
        }
      }
    }
  } else {
    //save the packet, we assume the Physical/Hal layer has already checked
    //if the MAC buffer has room
    mac_pib.rxHead++;
    if (mac_pib.rxHead == MAC_RXBUFF_SIZE) mac_pib.rxHead = 0;
    //save it.
    mac_pib.rxBuff[mac_pib.rxHead].data = ptr;     //save pointer
    mac_pib.rxBuff[mac_pib.rxHead].rssi = rssi;    //save RSSI value
    mac_pib.rxBuff[mac_pib.rxHead].corr = corr;    //save CORR value
  }
}

/*函数功能:判断MAC层接收缓存器是否为满，'1'－full,'0'－empty，rxHead--in,rxTail--out
			在接收完毕时，准备存储数据时调用*/
BOOL macRxBuffFull(void){
  BYTE tmp;
  //if next write would go to where Tail is, then buffer is full
  tmp = mac_pib.rxHead+1;
  if (tmp == MAC_RXBUFF_SIZE) tmp = 0;
  return(tmp == mac_pib.rxTail);
}

/*函数功能:判断MAC层接收缓存器是否为空，'0'－full,'1'－empty，在macrxFSM函数中调用*/
BOOL macRxBuffEmpty(void)
{
  return(mac_pib.rxTail == mac_pib.rxHead);
}

//this does NOT remove the packet from the buffer
/*函数功能:从MAC层接收缓存器取一个数据包，若为空，返回空指针，若非空，返回一个指向MACPKT的指针*/
MACPKT *macGetRxPacket(void)
{
  BYTE tmp;	
  if (mac_pib.rxTail == mac_pib.rxHead) return(NULL);
  tmp = mac_pib.rxTail+1;
  if (tmp == MAC_RXBUFF_SIZE) tmp = 0;
  return(&mac_pib.rxBuff[tmp]);
}

//frees the first packet in the buffer.
/*函数功能:从MAC层接收缓存器删除一个数据包，并且释放内存，在macrxFSM函数中调用*/
void macFreeRxPacket(BOOL freemem)
{
  mac_pib.rxTail++;
  if (mac_pib.rxTail == MAC_RXBUFF_SIZE) mac_pib.rxTail = 0;
  if (freemem) MemFree(mac_pib.rxBuff[mac_pib.rxTail].data);
}




/************
The TxInProgress bit is set when the macTxData function
is called the first time to actually format the header and
send the packet. After that, each loop through the macTxFSM
checks to see if the TX started and finished correctly, and
if an ACK was received if one was requested.  The FSM becomes
idle if:
a. the PHY TX returns an error
b. the PHY TX returned success and either no ACK was
requested or we received the correct ACK
c. the PHY TX returned success and we exhausted retries.

**************/
/*macTxFSM 函数原来的功能:MAC层发送状态机，在(!macTXIdle && phyIdle)时，
	根据发送是否成功，释放等待确认帧，和超时来做处理*/
/*函数功能:查看AckPendingTable,看是否有等待确认帧的缓存数据，若超时，则重发*/
void macTxFSM(void)
{
  BYTE i;
  if (phyTxBusy())
    return;

  for (i=0;i<MaxAckWindow;i++) {
    if (!AckPendingTable[i].options.bits.Used)
      continue;

      if (AckPendingTable[i].options.bits.ACKPending) {//等待ACK
        if (AckPendingTable[i].currentAckRetries) {
          if (halMACTimerNowDelta(AckPendingTable[i].TxStartTime)>mac_pib.macAckWaitDuration) {
            while (phyTxBusy()) {
              if (halMACTimerNowDelta(phy_pib.txStartTime) > MAX_TX_TRANSMIT_TIME) {
                phySetTxIdle();
                break;
              }
              phySetTxBusy();
              halRepeatSendPacket(AckPendingTable[i].DSN);
            }
          }
        } else {//超过一定次数，释放内存，向上层报错
          MemFree(AckPendingTable[i].Ptr);
          AckPendingTable[i].options.bits.Used = 0;
          AckPendingTable[i].ErrorState = LRWPAN_STATUS_SEND_OVERTIME;
        }
      } else {//接收到ACK
        MemFree(AckPendingTable[i].Ptr);
        AckPendingTable[i].options.bits.Used = 0;
        AckPendingTable[i].ErrorState = LRWPAN_STATUS_SUCCESS;
      }
  }

}






/*函数功能：用来比较两个长地址是否相同，返回‘1’表示相同，返回‘0’表示不相同，add now*/
static BOOL macCheckLaddrSame(LADDR *laddr1,LADDR *laddr2)
{
	BYTE i;
	for (i=0;i<8;i++) {
		if (laddr1->bytes[i]==laddr2->bytes[i])
                  continue;
                break;
		}
	if (i==8)
		return (TRUE);
	else
		return (FALSE);
}


//call back from HAL to here, can be empty functions
//not needed in this stack
void macApplyAckTable(BYTE *ptr, BYTE flen) {
  BYTE i;
  BYTE *memptr;
  if (LRWPAN_GET_ACK_REQUEST(*ptr)) {
    for (i=0;i<MaxAckWindow;i++) {
      if (!AckPendingTable[i].options.bits.Used) {
        //DEBUG_STRING(DBG_INFO,"Apply a ack pending table.\n");
        mac_pib.macDSNIndex = i;
        AckPendingTable[i].options.bits.Used = 1;
        AckPendingTable[i].options.bits.ACKPending = 1;
        AckPendingTable[i].DSN = mac_pib.macDSN;
        AckPendingTable[i].currentAckRetries = mac_pib.macMaxAckRetries;
        flen++;
        memptr = MemAlloc(flen);
        if (memptr==NULL) {
          AckPendingTable[i].options.bits.Used = 0;
          //DEBUG_STRING(DBG_ERR,"The mem has no room for applying for a AckPendingTable!\n");
          break;
        }
        AckPendingTable[i].Ptr = memptr;
        *(memptr) = flen;
        memptr++;
        flen--;
        while (flen) {
          *(memptr) = *ptr;
          memptr++;
          ptr++;
          flen--;
        }
        break;
      }
    }
    //if (i == MaxAckWindow)
      //DEBUG_STRING(DBG_ERR, "The AckPendingTable has no room!\n");
  }
}

//format the packet and send it
//this is NOT used for either beacon or ack frames, only for data and MAC frames
//a side effect of this function is that if the source address mode is SHORT
//and our MAC short address is 0xFFFE, then the source address mode is forced to
//long as per the IEEE SPEC.

//this builds the header in reverse order since we adding header bytes to
//headers already added by APS, NWK layers.

//Add the MAC header, then send it to PHY
/*函数功能:根据a_mac_tx_data结构体信息，将上层数据打包成MAC帧，交给物理层状态机进行发送数据。
			设置相关参数，mac_pib.flags.bits.ackPending = 0;	macSetTxBusy();
   				 mac_pib.currentAckRetries = mac_pib.macMaxAckRetries;
    					macTxFSM_status = LRWPAN_STATUS_MAC_INPROGRESS;*/
void macTxData(void)
{
  BYTE c;
  BYTE dstmode, srcmode;

  if (macTXIdle()) {
    //first time we are sending this packet, format the header
    //used static space for header. If need to store, will copy it later
    //format the header

    dstmode = LRWPAN_GET_DST_ADDR(a_mac_tx_data.fcfmsb);
    srcmode = LRWPAN_GET_SRC_ADDR(a_mac_tx_data.fcfmsb);

    //如果MAC PANID is 0xFFFE,并且源地址模式为短地址，则将其改为扩展地址
    if (mac_pib.macPANID == 0xFFFE && srcmode == LRWPAN_ADDRMODE_SADDR) {
      //our short address is 0xFFFE, force srcmode to long address
      srcmode = LRWPAN_ADDRMODE_LADDR;
      a_mac_tx_data.fcfmsb = a_mac_tx_data.fcfmsb & ~LRWPAN_FCF_SRCMODE_MASK;//clear src mode
      LRWPAN_SET_SRC_ADDR(a_mac_tx_data.fcfmsb,LRWPAN_ADDRMODE_LADDR);//set to long address
    }

    //format src Address
    switch(srcmode){
    case LRWPAN_ADDRMODE_NOADDR:
      break;
    case LRWPAN_ADDRMODE_SADDR:
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE)(a_mac_tx_data.SrcAddr >> 8);
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE)a_mac_tx_data.SrcAddr;	
      phy_pib.currentTxFlen=phy_pib.currentTxFlen+2;
      break;
    case LRWPAN_ADDRMODE_LADDR:  //this has to be our own long address, get it
      halGetProcessorIEEEAddress(phy_pib.currentTxFrm-8);
      phy_pib.currentTxFlen=phy_pib.currentTxFlen+8;
      phy_pib.currentTxFrm = phy_pib.currentTxFrm-8;
      break;
    default:
      break;
    }

    //format src PANID,注意源PANID存在的条件
    if ( !LRWPAN_GET_INTRAPAN(a_mac_tx_data.fcflsb) && srcmode != LRWPAN_ADDRMODE_NOADDR) {
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE) (a_mac_tx_data.SrcPANID >> 8);
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE)a_mac_tx_data.SrcPANID;
      phy_pib.currentTxFlen=phy_pib.currentTxFlen+2;
    }

    //format dst Address
    switch(dstmode){
    case LRWPAN_ADDRMODE_NOADDR:
      break;
    case LRWPAN_ADDRMODE_SADDR:
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE)(a_mac_tx_data.DestAddr.saddr >> 8);
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE)a_mac_tx_data.DestAddr.saddr;
      phy_pib.currentTxFlen=phy_pib.currentTxFlen+2;
      break;
    case LRWPAN_ADDRMODE_LADDR:
      for(c=0;c<8;c++) {
        phy_pib.currentTxFrm--;
        *phy_pib.currentTxFrm = a_mac_tx_data.DestAddr.laddr.bytes[7-c];
      }
      phy_pib.currentTxFlen=phy_pib.currentTxFlen+8;
      break;
    default:
      break;
    }

    //format dst PANID, will be present if both dst is nonzero
    if (dstmode != LRWPAN_ADDRMODE_NOADDR){
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE) (a_mac_tx_data.DestPANID >> 8);
      phy_pib.currentTxFrm--;
      *phy_pib.currentTxFrm = (BYTE)a_mac_tx_data.DestPANID;					
      phy_pib.currentTxFlen=phy_pib.currentTxFlen+2;
    }

    //format dsn
    mac_pib.macDSN++;
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm = mac_pib.macDSN; //set DSN		

    //format MSB Fcontrol
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm = a_mac_tx_data.fcfmsb;

    //format LSB Fcontrol
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm = a_mac_tx_data.fcflsb;		
	//*phy_pib.currentTxFrm &= 0xDF;

    phy_pib.currentTxFlen = phy_pib.currentTxFlen + 3; //DSN, FCFLSB, FCFMSB

    // at this point, we will attempt a TX
    mac_pib.flags.bits.ackPending = 0;

    //now send the data, ignore the GTS and INDIRECT bits for now
    //DEBUG_STRING(DBG_TX,"\n");
    //DEBUG_STRING(DBG_TX,"TX DSN: ");
    //DEBUG_UINT8(DBG_TX,mac_pib.macDSN);


    macSetTxBusy();
    mac_pib.currentAckRetries = mac_pib.macMaxAckRetries;
    macTxFSM_status = LRWPAN_STATUS_MAC_INPROGRESS;
  }

  //macApplyAckTable(phy_pib.currentTxFrm, phy_pib.currentTxFlen);


  a_phy_service.cmd = LRWPAN_SVC_PHY_TX_DATA;
  phyDoService();
  macSetTxIdle();//原版协议栈中，会在发送等待中设置发送空闲，此处，设置发送空闲
  a_mac_service.status = a_phy_service.status;
}


//might be able to simplify this later.
/*函数功能:MAC层接收状态机，macRxState为MAC_RXSTATE_IDLE时，看接收缓存区是否空，若不，看接收帧的类型，
			若出错直接返回，若是信标帧和命令帧，进行相应操作，再释放空间，若是数据帧交给网络层处理。*/
void macRxFSM(void)
{
  MACPKT *pkt;
  BYTE cmd;

macRxFSM_start:
  switch(macRxState)  {
  case MAC_RXSTATE_IDLE:
    if (macRxBuffEmpty()) break;   //no data, break
    pkt = macGetRxPacket();//buffer not empty, start decode
    //dbgPrintPacket(pkt->data+1, *(pkt->data))，	must be either a DATA, BCN, or MAC packet
    //at this point, we throw away BCN packets if are not waiting for a beacon response

	/* if ((LRWPAN_IS_BCN(*(pkt->data+1))) &&
        !mac_pib.flags.bits.WaitingForBeaconResponse) {//是信标帧，但我们不是在等待它，则放弃
          DEBUG_STRING(DBG_INFO,"MAC: Received BCN pkt, discarding.\n");
          macFreeRxPacket(TRUE);
          break;
        }*/

    if (LRWPAN_IS_ACK(*(pkt->data+1))) {
      //This should not happen. ACK packets should be parsed in the HAL layer that copies ACK packets to temp storage.
      //will keep this for completeness.
      //DEBUG_STRING(DBG_INFO,"MAC: Received ACK pkt in macStartRxDecode, discarding, check ack packet parsing..\n");
      macFreeRxPacket(TRUE);
      break;
    }
    //at this point, we have a DATA, MAC CMD, or BCN packet.. do something with it.
    //need to parse the header information get to the payload.
    a_mac_rx_data.orgpkt = pkt;
    macParseHdr();
    if ((LRWPAN_IS_BCN(*(pkt->data+1)))){
      //DEBUG_STRING(DBG_INFO,"MAC: Parsing BCN pkt.\n");
      //now finished with it.
      macParseBeacon();
      macFreeRxPacket(TRUE);

      macRxState = MAC_RXSTATE_IDLE;//收到信标帧并解析后，可以退出macRxFSM，也可以重新开始读取数据，我们采用后者。
      goto macRxFSM_start;
      //break;
    }

    if (LRWPAN_IS_DATA(*(pkt->data+1))){
		//this is a data packet, check if we should reject it
        if (!macCheckDataRejection()) {
            //we need to reject this packet.
          //DEBUG_STRING(DBG_INFO,"MAC: Rejecting Data packet from unassociated node, rejecting.\n");
          macFreeRxPacket(TRUE);
            break;
        }
       mac_pib.last_data_rx_time = halGetMACTimer();  //time of last data or mac command
      //at this point, will accept packet, indicate this to network layer
      //set a flag, and pass the nwkStart offset to the NWK layer
      //RX buffer.
      macRxState = MAC_RXSTATE_NWK_HANDOFF;
      goto macRxFSM_start;
    }

    //at this point, we have a MAC command packet, lets do something with it.
    //DEBUG_STRING(DBG_INFO,"MAC: Received MAC cmd packet, proceeding.\n");
    //there are some MAC CMDs that we can handle right here.
    //If it is a response, we handle it here. If it is a request,
    //that has to be handled in the main FSM.
    cmd = *(a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset);
    switch(cmd) {

//mini
/*
    case LRWPAN_MACCMD_ASSOC_RSP:
#ifndef LRWPAN_COORDINATOR
      if (mac_pib.flags.bits.WaitingForAssocResponse){
        macParseAssocResponse();
      }					
#endif				
      //free this packet, we are finished with it.
      macFreeRxPacket(TRUE);

      macRxState = MAC_RXSTATE_IDLE;//收到信标帧并解析后，可以退出macRxFSM，也可以重新开始读取数据，我们采用后者。
      goto macRxFSM_start;

      //break;
*/
//mini

	case LRWPAN_MACCMD_COORD_REALIGN:
#ifndef LRWPAN_COORDINATOR
      if (mac_pib.flags.bits.WaitingForOrphanResponse){
        macParseOrphanResponse();
      }	
#endif				
      //free this packet, we are finished with it.
      macFreeRxPacket(TRUE);

      macRxState = MAC_RXSTATE_IDLE;//收到信标帧并解析后，可以退出macRxFSM，也可以重新开始读取数据，我们采用后者。
      goto macRxFSM_start;

    case LRWPAN_MACCMD_SYNC_RSP:
	{mac_sync_time.TS2=mac_sync_time.ReceiveTime;
	mac_sync_time.TS2_1=mac_sync_time.ReceiveTime_1;
	macParseSyncRsp();
	macFreeRxPacket(TRUE);}
	break;
    case LRWPAN_MACCMD_GTS_REQ:
#ifdef LRWPAN_COORDINATOR
	if(mac_pib.flags.bits.macGTSPermit)
	macParseGTSRequest();	
#endif
	macFreeRxPacket(TRUE);
        break;

    case LRWPAN_MACCMD_DISASSOC:
#ifdef LRWPAN_FFD
      //only FFDs handle this
    case LRWPAN_MACCMD_SYNC_REQ:
      {mac_sync_time.TM1=mac_sync_time.ReceiveTime;
      mac_sync_time.TM1_1=mac_sync_time.ReceiveTime_1;}
    case LRWPAN_MACCMD_BCN_REQ:
    case LRWPAN_MACCMD_ASSOC_REQ:
    case LRWPAN_MACCMD_ORPHAN:

      //requests must be handled in the main FSM. We need to signal that a MAC
      //CMD packet is packet is pending, and freeze the RX FSM until the
      //main FSM has taken care of it.
#endif
      mac_pib.flags.bits.macPending = 1;
      macRxState = MAC_RXSTATE_CMD_PENDING;
      break;


    default:
      //unhandled MAC packets
      //DEBUG_STRING(DBG_INFO,"MAC: Received MAC CMD that is not implemented or not handled by this node, discarding.\n");
      //DEBUG_STRING(DBG_INFO,"Cmd is: ");
      //DEBUG_UINT8(DBG_INFO,cmd);
      //DEBUG_STRING(DBG_INFO,"\n");
      macFreeRxPacket(TRUE);
      break;

    }			

    break;
  case MAC_RXSTATE_NWK_HANDOFF:
    if (nwkRxBusy()) break;    //nwkRX is still busy
    //handoff the current packet
    nwkRxHandoff();
    //we are finished with this packet.
    //free the MAC resource, but not the memory. The NWK layer
    // or above has to free the memory
    macFreeRxPacket(FALSE);
    macRxState = MAC_RXSTATE_IDLE;
    break;
  case MAC_RXSTATE_CMD_PENDING:
    if (mac_pib.flags.bits.macPending ) break;
    //when macPending is cleared, this means main FSM is finished with packet.
    //So free the packet, and start parsing new packets again
    macFreeRxPacket(TRUE);
    macRxState = MAC_RXSTATE_IDLE;
    goto macRxFSM_start;
  default: break;
  }
}

//parse(解析) the header currently in a_mac_rx_data，return the offset to the network header.
/*功能:根据物理层接收到的数据，解析MAC帧头相关信息，存储到a_mac_rx_data结构体*/
static void macParseHdr() {
  BYTE *ptr;
  BYTE len,i;
  BYTE srcmode, dstmode;

  ptr = a_mac_rx_data.orgpkt->data;

  //skip first byte since the first byte in the a_mac_rx_data.orgpkt is the packet length
  len = 1;ptr++;//第一个字节存的是帧长度


  a_mac_rx_data.fcflsb = *ptr; ptr++;
  a_mac_rx_data.fcfmsb = *ptr; ptr++;
  dstmode = LRWPAN_GET_DST_ADDR(a_mac_rx_data.fcfmsb);
  srcmode = LRWPAN_GET_SRC_ADDR(a_mac_rx_data.fcfmsb);

  //skip DSN   忽略序列号
  ptr++;
  len = len +3;

  if (dstmode != LRWPAN_ADDRMODE_NOADDR){//get the DEST PANDID
    a_mac_rx_data.DestPANID = *ptr;
    ptr++;
    a_mac_rx_data.DestPANID += (((UINT16)*ptr) << 8);
    ptr++;
    len = len + 2;
  }

  if (dstmode == LRWPAN_ADDRMODE_SADDR) {//DST address
    a_mac_rx_data.DestAddr.saddr = *ptr;
    ptr++;
    a_mac_rx_data.DestAddr.saddr += (((UINT16)*ptr) << 8);
    ptr++;
    len = len + 2;

  }else if (dstmode == LRWPAN_ADDRMODE_LADDR) {
    for (i=0;i<8;i++) {
      a_mac_rx_data.DestAddr.laddr.bytes[i] = *ptr;
      ptr++;
    }
    len = len + 8;
  }


  if ( !LRWPAN_GET_INTRAPAN(a_mac_rx_data.fcflsb) &&
      srcmode != LRWPAN_ADDRMODE_NOADDR
        ) {//PANID present if INTRAPAN is zero, and src is nonzero
          a_mac_rx_data.SrcPANID = *ptr;
          ptr++;
          a_mac_rx_data.SrcPANID += (((UINT16)*ptr) << 8);
          ptr++;
          len = len + 2;
        }

  if (srcmode == LRWPAN_ADDRMODE_SADDR) {//SRC address
    a_mac_rx_data.SrcAddr.saddr = *ptr;
    ptr++;
    a_mac_rx_data.SrcAddr.saddr += (((UINT16)*ptr) << 8);
    ptr++;
    len = len + 2;

  }else if (srcmode == LRWPAN_ADDRMODE_LADDR) {
    for (i=0;i<8;i++) {
      a_mac_rx_data.SrcAddr.laddr.bytes[i] = *ptr;
      ptr++;
    }
    len = len + 8;
  }

  a_mac_rx_data.pload_offset = len;  //save offset.
  //a_mac_rx_data.ED = macRadioComputerED(a_mac_rx_data.orgpkt->rssi);
  a_mac_rx_data.LQI = macRadioComputeLQI(a_mac_rx_data.orgpkt->corr);
}

/*#ifdef LRWPAN_FFD
//Beacon payload format
// nwk magic number ( 4 bytes) | mac depth
void macFormatBeacon(void){
  BYTE i;
  BYTE flen=0;
  DEBUG_STRING(DBG_INFO,"format beacon!");
  //fill in the beacon payload, we have the TX buffer lock
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
#ifndef LRWPAN_ZIGBEE_BEACON_COMPLY
  //fill in the magic number
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B3;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B2;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B1;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B0;
#endif

  //next three bytes are zero for timestep difference
  //for multi-hop beaconing networks. This is currently filled in
  //as zero
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =0;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =0;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =0;

  //see if I have space for an END device
  --phy_pib.currentTxFrm;
  if (mac_pib.ChildRFDs == LRWPAN_MAX_NON_ROUTER_CHILDREN) {
	  *phy_pib.currentTxFrm =0; //no room
  } else {
      *phy_pib.currentTxFrm =1;  //have space.
  }

  //fill in my depth
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = mac_pib.macDepth;

  //see if I have space for a ROUTER device
  --phy_pib.currentTxFrm;
  if (mac_pib.ChildRouters == LRWPAN_MAX_ROUTERS_PER_PARENT) {
	  *phy_pib.currentTxFrm =0; //no room
  } else {
      *phy_pib.currentTxFrm =1;  //have space.
  }

   //network protocol version
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =LRWPAN_ZIGBEE_PROTOCOL_VER;

   //stack protocol
   --phy_pib.currentTxFrm;
   *phy_pib.currentTxFrm =LRWPAN_STACK_PROFILE;

   //Zigbee protocol ID
   --phy_pib.currentTxFrm;
   *phy_pib.currentTxFrm =LRWPAN_ZIGBEE_PROTOCOL_ID;


  //pending address field
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = 0;  //makes this a NOP


for(i=0;i<mac_pib.GTSDescriptCount;i++){
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = gtsAllocate[i].GTSInformation;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =(BYTE)(gtsAllocate[i].address1 >>8);
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =(BYTE)(gtsAllocate[i].address1);
  flen+=3;
}
if(mac_pib.GTSDescriptCount !=0){  //GTS directions field
    --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =mac_pib.GTSDirection;
  flen+=1;}

      --phy_pib.currentTxFrm;//GTS specification
  *phy_pib.currentTxFrm =mac_pib.GTSDescriptCount;
  if(mac_pib.flags.bits.macGTSPermit)
  	*phy_pib.currentTxFrm |=0x80;
  flen+=1;


  //2 bytes of superframe
#ifdef LRWPAN_COORDINATOR
  i = LRWPAN_BEACON_SF_PAN_COORD_MASK;
#else
  i = 0;
#endif
  if (mac_pib.flags.bits.macAssociationPermit) {
    i = i | LRWPAN_BEACON_SF_ASSOC_PERMIT_MASK;
  }
    i = i | mac_pib.finalSlot;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = i;  //MSB of superframe

  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = mac_pib.macBeaconOrder;
  *phy_pib.currentTxFrm += (mac_pib.macSuperframeOrder<<4);//LSB of superframe

  phy_pib.currentTxFlen = LRWPAN_NWK_BEACON_SIZE+flen;

	a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_BEACON;
	a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_NOADDR|LRWPAN_FCF_SRCMODE_SADDR;
	a_mac_tx_data.SrcAddr = macGetShortAddr();
	a_mac_tx_data.SrcPANID = mac_pib.macPANID;

}   */



#ifdef LRWPAN_FFD
//Beacon payload format
// nwk magic number ( 4 bytes) | mac depth
static void macFormatBeacon(void){
  BYTE i;

  //fill in the beacon payload, we have the TX buffer lock
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
#ifndef LRWPAN_ZIGBEE_BEACON_COMPLY
  //fill in the magic number
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B3;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B2;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B1;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B0;
#endif

  //next three bytes are zero for timestep difference
  //for multi-hop beaconing networks. This is currently filled in
  //as zero
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =0;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =0;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =0;

  //see if I have space for an END device
  --phy_pib.currentTxFrm;
  if (mac_pib.ChildRFDs == LRWPAN_MAX_NON_ROUTER_CHILDREN) {
	  *phy_pib.currentTxFrm =0; //no room
  } else {
      *phy_pib.currentTxFrm =1;  //have space.
  }

  //fill in my depth
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = mac_pib.macDepth;

  //see if I have space for a ROUTER device
  --phy_pib.currentTxFrm;
  if (mac_pib.ChildRouters == LRWPAN_MAX_ROUTERS_PER_PARENT) {
	  *phy_pib.currentTxFrm =0; //no room
  } else {
      *phy_pib.currentTxFrm =1;  //have space.
  }

   //network protocol version
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm =LRWPAN_ZIGBEE_PROTOCOL_VER;

   //stack protocol
   --phy_pib.currentTxFrm;
   *phy_pib.currentTxFrm =LRWPAN_STACK_PROFILE;

   //Zigbee protocol ID
   --phy_pib.currentTxFrm;
   *phy_pib.currentTxFrm =LRWPAN_ZIGBEE_PROTOCOL_ID;

  //pending address field
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = 0;  //makes this a NOP

  //GTS directions field
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = 0;  //makes this a NOP

  //2 bytes of superframe
#ifdef LRWPAN_COORDINATOR
  i = LRWPAN_BEACON_SF_PAN_COORD_MASK;
#else
  i = 0;
#endif
  if (mac_pib.flags.bits.macAssociationPermit) {
    i = i | LRWPAN_BEACON_SF_ASSOC_PERMIT_MASK;
  }
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = i;  //MSB of superframe

  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = 0xFF; //LSB of superframe

  phy_pib.currentTxFlen = LRWPAN_NWK_BEACON_SIZE;

}

/*函数功能:发送信标帧，目的地址模式为空地址，源地址模式为短地址或长地址*/
void macSendBeacon(void)
{
  macFormatBeacon();
  a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_BEACON;
  //a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_NOADDR|LRWPAN_FCF_SRCMODE_SADDR;
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_NOADDR|LRWPAN_FCF_SRCMODE_LADDR;
  a_mac_tx_data.SrcAddr = macGetShortAddr();
  a_mac_tx_data.SrcPANID = mac_pib.macPANID;
  macTxData();
}

#endif

//parse the beacon
static void macParseBeacon(void){

  BYTE *ptr;
  BYTE i,depth,gtsmsf,gtscount;
  SADDR adress4;


  ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;
  //DEBUG_STRING(DBG_INFO,"parsing beacon!\n");
  mac_pib.macBeaconOrder=*ptr & 0x0F;
  mac_pib.macSuperframeOrder=(*ptr>>4) &0x0F;
  ptr++;
  mac_pib.corAssociationPermit=*ptr & 0x80;
  mac_pib.finalSlot= *ptr&0x0F;
  ptr++;
  mac_pib.corGTSPermit=*ptr & 0x80;
  gtscount=*ptr &0x07;
  ptr++;
  if(gtscount!=0){
  gtsmsf=*ptr &0x7F;
  adress4=macGetShortAddr();
  ptr++;
  for(i=0;i<gtscount;i++){
    if(*ptr!=(BYTE)(adress4)) {ptr+=3;continue;
    }
  ptr++;
  if(*ptr==(BYTE)(adress4>>8))	{
    ptr++;mac_pib.localGTSSlot= *ptr & 0x0F;
    //DEBUG_STRING(DBG_INFO,"have local address!");
    mac_pib.localGTSLength= (*ptr >>4) &0x0F;
    mac_pib.localdirection=gtsmsf &(1<< (16-mac_pib.localGTSSlot));
   }
  }
}


ptr++;//pengdingaddress
  //skip if any mismatches on protocol ID/Ver, stack profile, etc.
  //check protocol ID
  if (*ptr != LRWPAN_ZIGBEE_PROTOCOL_ID) return;
  ptr++;
  //check stack profile
  if (*ptr != LRWPAN_STACK_PROFILE) return;
  ptr++;
  //check protocol version
  if (*ptr != LRWPAN_ZIGBEE_PROTOCOL_VER) return;
  ptr++;

  //check router capacity
  //for right now, if I am a router, I have to join as a router.
  //no option as of now for a router joining as an end-device
#ifdef LRWPAN_FFD
  //only routers have to check this
  if (*ptr == 0);  //no room to join as router
#endif
  ptr++;
  //get the depth
  depth = *ptr;
  ptr++;

  //check end device capacity
#ifdef LRWPAN_RFD
  //only end devices have to check this.
  if (*ptr == 0);  //no room to join as end device
#endif
  ptr++;

  //skip the next three bytes, only for beaconing.
  ptr = ptr + 3;

#ifndef LRWPAN_ZIGBEE_BEACON_COMPLY
  //check the magic number
  if (*ptr != LRWPAN_NWK_MAGICNUM_B0) return;
  ptr++;
  if (*ptr != LRWPAN_NWK_MAGICNUM_B1) return;
  ptr++;
  if (*ptr != LRWPAN_NWK_MAGICNUM_B2) return;
  ptr++;
  if (*ptr != LRWPAN_NWK_MAGICNUM_B3) return;
  ptr++;
#endif

  //at this point, we could accept this node as a parent
  if ((mac_pib.bcnDepth == 0xFF) ||
      (a_mac_rx_data.orgpkt->rssi > mac_pib.bcnRSSI)) {
        //either our first response, or from a closer node
        //save this information.
        //use value with higher RSSI.
        //the RSSI byte is assumed to be formatted as HIGHER IS BETTER
        //the HAL layer converts any native signed RSSI to an unsigned value

        mac_pib.bcnDepth = depth;  //get depth
        mac_pib.bcnRSSI = a_mac_rx_data.orgpkt->rssi;
        mac_pib.bcnSADDR = a_mac_rx_data.SrcAddr.saddr;
        mac_pib.bcnPANID = a_mac_rx_data.SrcPANID;

      }

mac_pib.flags.bits.GotBeaconResponse = 1;
}

/*函数功能:解析信标帧，若长度出错，不允许连接，相关参数出错，没有连接空间，则直接返回。

函数功能:形成MAC命令帧结构的MAC载荷，连接请求命令,原来版本，更新版本如下
static void macFormatAssocRequest(void){
  //fill in payload of request
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];

#ifndef IEEE_802_COMPLY
  //put the magic number in the association request
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B3;

  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B2;

  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B1;

  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = LRWPAN_NWK_MAGICNUM_B0;
#endif

  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = mac_pib.macCapInfo;

  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = LRWPAN_MACCMD_ASSOC_REQ;

  phy_pib.currentTxFlen = LRWPAN_MACCMD_ASSOC_REQ_PAYLOAD_LEN;

}

*/

//#ifndef LRWPAN_COORDINATOR


/*函数功能:形成MAC命令帧结构的MAC载荷，连接请求命令*/
void macFormatAssociateRequest(void)
{
	phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
	phy_pib.currentTxFrm--;
	*phy_pib.currentTxFrm = a_mac_service.args.associate.request.CapabilityInformation;
	phy_pib.currentTxFrm--;
	*phy_pib.currentTxFrm = LRWPAN_MACCMD_ASSOC_REQ;
	phy_pib.currentTxFlen = LRWPAN_MACCMD_ASSOC_REQ_PAYLOAD_LEN;        	
}

/*函数功能:发送连接请求命令*/
void macSendAssociateRequest(void)
{
	macFormatAssociateRequest();
        //a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC;
        if (a_mac_service.args.associate.request.SecurityEnable==1) {
          //a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_SECURITY_MASK|LRWPAN_FCF_ACKREQ_MASK;
          a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_SECURITY_MASK;
       }
       else {
          //a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_ACKREQ_MASK;
         a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC;
       }
	if (a_mac_service.args.associate.request.CoordAddrMode==LRWPAN_ADDRMODE_SADDR) {
	//mini//a_mac_tx_data.fcfmsb = LRWPAN_FCF_SRCMODE_LADDR|LRWPAN_FCF_DSTMODE_SADDR;
       //mini//a_mac_tx_data.DestAddr.saddr = a_mac_service.args.associate.request.CoordAddress.saddr;
               // a_mac_tx_data.DestAddr.saddr = 0x0001;    //mini
               a_mac_tx_data.fcfmsb = LRWPAN_FCF_SRCMODE_LADDR|LRWPAN_FCF_DSTMODE_LADDR;//mini
               a_mac_tx_data.DestAddr.laddr=a_mac_rx_data.SrcAddr.laddr;//mini
	}
	else {
		a_mac_tx_data.fcfmsb = LRWPAN_FCF_SRCMODE_LADDR|LRWPAN_FCF_DSTMODE_LADDR;
		halUtilMemCopy(&a_mac_tx_data.DestAddr.laddr.bytes[0],&a_mac_service.args.associate.request.CoordAddress.laddr.bytes[0],8);
      }
	a_mac_tx_data.DestPANID = a_mac_service.args.associate.request.CoordPANID;
	a_mac_tx_data.SrcPANID = LRWPAN_BCAST_PANID;

	macTxData();

}

//#endif


/*函数功能：形成MAC命令帧结构的MAC载荷，断开连接通告*/
static void macFormatDiassocRequest(void)//add now
{
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = a_mac_service.args.disassoc_req.DisassociateReason;
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = LRWPAN_MACCMD_DISASSOC;
  phy_pib.currentTxFlen = LRWPAN_MACCMD_DISASSOC_PAYLOAD_LEN;
}


void macSendDisassociateRequest(void)
{
  macFormatDiassocRequest();
  if (a_mac_service.args.disassoc_req.SecurityEnable)
    a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_SECURITY_MASK|LRWPAN_FCF_ACKREQ_MASK;
  else
    a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_ACKREQ_MASK;
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_LADDR;
  a_mac_tx_data.DestPANID = mac_pib.macPANID;
  a_mac_tx_data.SrcPANID = mac_pib.macPANID;
  if (a_mac_service.args.disassoc_req.DisassociateReason == DEVICE_DISASSOC_WITH_NETWORK) {
    halUtilMemCopy( &a_mac_tx_data.DestAddr.laddr.bytes[0],&mac_pib.macCoordExtendedAddress.bytes[0],8);
  } else if (a_mac_service.args.disassoc_req.DisassociateReason == FORCE_DEVICE_OUTOF_NETWORK) {
    halUtilMemCopy( &a_mac_tx_data.DestAddr.laddr.bytes[0], &a_mac_service.args.disassoc_req.DeviceAddress.bytes[0], 8);
  }
  macTxData();
  //善后处理，待处理

}

//#ifndef LRWPAN_COORDINATOR
//parse the association response
/*函数功能:解析连接响应命令相关信息，若出错(长度出错，连接没有成功)，则直接返回。
			若连接成功，读取短地址，网络号，协调器短地址和扩展地址并存储，处理标志位*/
SADDR macParseAssocResponse(void)
{
  BYTE *ptr;
  SADDR saddr;
  //wrong length
  if ( (*(a_mac_rx_data.orgpkt->data)-a_mac_rx_data.pload_offset-PACKET_FOOTER_SIZE +1) != LRWPAN_MACCMD_ASSOC_RSP_PAYLOAD_LEN ) {
    return 0xFFFF;
  }
  ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;
  //failed to join
  if (LRWPAN_GET_ASSOC_STATUS(*(ptr+LRWPAN_MACCMD_ASSOC_RSP_PAYLOAD_LEN-1)) != LRWPAN_ASSOC_STATUS_SUCCESS) {
    return 0xFFFF;
  }
  ptr++; //skip command byte

  //successful join, get my short SADDR
  saddr = (BYTE) *ptr;
  ptr++;
  saddr += (((UINT16) *ptr) << 8);
  if (saddr == LRWPAN_BCAST_SADDR)
     return 0xFFFF;
  macSetShortAddr(saddr);
  ptr++;

  //our PANID is our parent's panid.
  mac_pib.macPANID = a_mac_rx_data.SrcPANID;
  halSetRadioPANID(a_mac_rx_data.SrcPANID);

//mini
  /*
  //copy the SRC long address as my coordinator long address
  mac_pib.macCoordExtendedAddress.bytes[0] = a_mac_rx_data.SrcAddr.laddr.bytes[0];
  mac_pib.macCoordExtendedAddress.bytes[1] = a_mac_rx_data.SrcAddr.laddr.bytes[1];
  mac_pib.macCoordExtendedAddress.bytes[2] = a_mac_rx_data.SrcAddr.laddr.bytes[2];
  mac_pib.macCoordExtendedAddress.bytes[3] = a_mac_rx_data.SrcAddr.laddr.bytes[3];
  mac_pib.macCoordExtendedAddress.bytes[4] = a_mac_rx_data.SrcAddr.laddr.bytes[4];
  mac_pib.macCoordExtendedAddress.bytes[5] = a_mac_rx_data.SrcAddr.laddr.bytes[5];
  mac_pib.macCoordExtendedAddress.bytes[6] = a_mac_rx_data.SrcAddr.laddr.bytes[6];
  mac_pib.macCoordExtendedAddress.bytes[7] = a_mac_rx_data.SrcAddr.laddr.bytes[7];
  */

  mac_pib.macCoordShortAddress = a_mac_rx_data.SrcAddr.saddr;
//mini

  //indicate that the association was successful
  mac_pib.flags.bits.macIsAssociated = 1;
  mac_pib.flags.bits.WaitingForAssocResponse = 0;

  return saddr;
}
//#endif


//#ifndef  LRWPAN_COORDINATOR
//Parse the coordinator realignment (Orphan response)
/*函数功能:解析孤立响应(协调器重新同步命令)相关信息，若长度出错，则直接返回。
			若对，读取PAN标志符，协调器短地址，逻辑信道，设备短地址并进行存储*/
static void macParseOrphanResponse(void){
  BYTE *ptr;
  UINT16 tmp;


  //first, ensure that the payload length is correct
  //the +1 is because the offset takes into account the lenght byte at the start of the packet
  if ( (*(a_mac_rx_data.orgpkt->data)-a_mac_rx_data.pload_offset-PACKET_FOOTER_SIZE +1)
      != LRWPAN_MACCMD_COORD_REALIGN_PAYLOAD_LEN ) {
        return; //wrong length
      }
  ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;

  mac_pib.flags.bits.GotOrphanResponse = 1;
  mac_pib.flags.bits.macIsAssociated = 1;  //we are associated with somebody!
  //mac_pib.flags.bits.ackPending = 0;

  ptr++; //skip command byte

  //get the PANID
  tmp = (BYTE) *ptr;
  ptr++;
  tmp += (((UINT16) *ptr) << 8);
  ptr++;
  macSetPANID(tmp);

  //get the coordinator short address
  mac_pib.macCoordShortAddress = (BYTE) *ptr;
  ptr++;
  mac_pib.macCoordShortAddress += (((UINT16) *ptr) << 8);
  ptr++;

  tmp =(BYTE) *ptr; //get the channel
  ptr++;

  macSetChannel(tmp);  //set the channel

#ifndef LRWPAN_COORDINATOR

  //copy the SRC long address as my coordinator long address
  mac_pib.macCoordExtendedAddress.bytes[0] = a_mac_rx_data.SrcAddr.laddr.bytes[0];
  mac_pib.macCoordExtendedAddress.bytes[1] = a_mac_rx_data.SrcAddr.laddr.bytes[1];
  mac_pib.macCoordExtendedAddress.bytes[2] = a_mac_rx_data.SrcAddr.laddr.bytes[2];
  mac_pib.macCoordExtendedAddress.bytes[3] = a_mac_rx_data.SrcAddr.laddr.bytes[3];
  mac_pib.macCoordExtendedAddress.bytes[4] = a_mac_rx_data.SrcAddr.laddr.bytes[4];
  mac_pib.macCoordExtendedAddress.bytes[5] = a_mac_rx_data.SrcAddr.laddr.bytes[5];
  mac_pib.macCoordExtendedAddress.bytes[6] = a_mac_rx_data.SrcAddr.laddr.bytes[6];
  mac_pib.macCoordExtendedAddress.bytes[7] = a_mac_rx_data.SrcAddr.laddr.bytes[7];


#endif

#ifdef LRWPAN_RFD
  //get our short address
  tmp = (BYTE) *ptr;
  ptr++;
  tmp += (((UINT16) *ptr) << 8);
  ptr++;
  macSetShortAddr(tmp);
#else
  //this is a router
   //get our short ADDR

   tmp = (BYTE) *ptr;
   ptr++;
   tmp += (((UINT16) *ptr) << 8);
   ptr++;

   if (tmp != macGetShortAddr()) {
     //our short address has changed!
     //everything may have changed,
     //clear neighbor table, and address map
     ntInitTable();
  }
  macSetShortAddr(tmp);
#endif

}

//#endif


/*函数功能:形成MAC命令帧结构中的MAC载荷，孤立通告命令，有效长度为1字节*/
void macFormatOrphanNotify(void){
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE-1];
  *phy_pib.currentTxFrm = LRWPAN_MACCMD_ORPHAN;
   phy_pib.currentTxFlen = 1;
}

void macFormatBeaconRequest(void)
{
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE-1];
  *phy_pib.currentTxFrm = LRWPAN_MACCMD_BCN_REQ;
  phy_pib.currentTxFlen = LRWPAN_MACCMD_BCN_REQ_PAYLOAD_LEN;
}

void macSendBeaconRequest(void)
{
  macFormatBeaconRequest();
  a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC;
  //mini//a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_NOADDR;
  //mini
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
  //mini
  a_mac_tx_data.DestPANID = LRWPAN_BCAST_PANID;
  a_mac_tx_data.DestAddr.saddr = LRWPAN_BCAST_SADDR;
  mac_pib.flags.bits.GotBeaconResponse =0;//will be set when get response
  mac_pib.flags.bits.WaitingForBeaconResponse = 1;  //will be cleared when get response
  macTxData();
}

#ifdef LRWPAN_FFD
/*函数功能:形成MAC命令帧结构中的MAC载荷，协调器重新同步命令，有效长度为8字节
			如果响应孤立通告命令，orphan_saddr为该设备地址，
			若更新超帧配置，orphan_saddr为0xFFFF*/
void macFormatCoordRealign(SADDR orphan_saddr){  //format and send the realignment packet
  //first is the orphans short address
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE-1];
  *phy_pib.currentTxFrm = (BYTE) (orphan_saddr >>8);

  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (orphan_saddr);

  //logical channel
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = phy_pib.phyCurrentChannel;

   //our short addresss
 --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (macGetShortAddr()>>8);

  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (macGetShortAddr());

  //our PANID

--phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (mac_pib.macPANID>>8);

  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (mac_pib.macPANID);


  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_MACCMD_COORD_REALIGN;


  phy_pib.currentTxFlen = LRWPAN_MACCMD_COORD_REALIGN_PAYLOAD_LEN;//8
}

void macSendCoordRealign(SADDR orphan_saddr)
{
  if (orphan_saddr==LRWPAN_BCAST_SADDR) {
    a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC;
    a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
    a_mac_tx_data.DestPANID = LRWPAN_BCAST_PANID;
    a_mac_tx_data.DestAddr.saddr = LRWPAN_BCAST_SADDR;
  } else {
    a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_ACKREQ_MASK;
    a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_LADDR;
    a_mac_tx_data.DestPANID = LRWPAN_BCAST_PANID;
    a_mac_tx_data.DestAddr.laddr.bytes[0] = a_mac_rx_data.SrcAddr.laddr.bytes[0];
    a_mac_tx_data.DestAddr.laddr.bytes[1] = a_mac_rx_data.SrcAddr.laddr.bytes[1];
    a_mac_tx_data.DestAddr.laddr.bytes[2] = a_mac_rx_data.SrcAddr.laddr.bytes[2];
    a_mac_tx_data.DestAddr.laddr.bytes[3] = a_mac_rx_data.SrcAddr.laddr.bytes[3];
    a_mac_tx_data.DestAddr.laddr.bytes[4] = a_mac_rx_data.SrcAddr.laddr.bytes[4];
    a_mac_tx_data.DestAddr.laddr.bytes[5] = a_mac_rx_data.SrcAddr.laddr.bytes[5];
    a_mac_tx_data.DestAddr.laddr.bytes[6] = a_mac_rx_data.SrcAddr.laddr.bytes[6];
    a_mac_tx_data.DestAddr.laddr.bytes[7] = a_mac_rx_data.SrcAddr.laddr.bytes[7];
  }
  a_mac_tx_data.SrcPANID = mac_pib.macPANID;
  macFormatCoordRealign(orphan_saddr);
  macTxData();	
}


void macOrphanNotifyResponse(void)
{
  NAYBORENTRY *nt_ptr;

  nt_ptr = ntFindByLADDR(&a_mac_rx_data.SrcAddr.laddr);
  if (!nt_ptr) { //not my orphan, ignoring
    //also unlock TX buffer
    phyReleaseTxLock();
    return;
  }
  if (nt_ptr->saddr == LRWPAN_BCAST_SADDR) {
    return;
  } else
 //at this point, we have an orphan. Send a response.
  macSendCoordRealign(nt_ptr->saddr);
}

/*//mini
#ifdef LRWPAN_COORDINATOR
BOOL laddr_permit (LADDR *ptr){
    if(ptr->bytes[7]==0)
    return (FALSE);
  else
    return (TRUE);
}
#endif
//mini*/

/*函数功能:形成MAC命令帧结构中的MAC载荷，连接响应命令，有效长度为7 or 4字节*/
/*在接收到连接请求命令后，对相关信息进行检查(连接请求命令长度，地址表是否有空间)，再做判断，给予响应*/
void macNetworkAssociateResponse(void){
  NAYBORENTRY *ntptr;
  UINT16 new_saddr;
  BYTE tmp, capinfo;


  new_saddr = 0xFFFF;
  tmp = LRWPAN_ASSOC_STATUS_DENIED;  //default status


  //now, see if this node is in the table
  /*该设备在邻居表中已存在，两种处理方案:1.保持邻居表信息不变2.删除原来所有信息，填写新的信息*/
  ntptr = ntFindByLADDR(&a_mac_rx_data.SrcAddr.laddr);//查看设备是否已存在邻居表，若是，不用分配短地址，直接回复
  if (ntptr) {
	   new_saddr = ntptr->saddr;
           tmp = LRWPAN_ASSOC_STATUS_SUCCESS;
           goto macFormatAssociationResponse_dopkt;
  }

  //node is not in table. Look at capability info byte and see if we have room for this node type
  capinfo = *(a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset + 1);

/*
  //node is not in table. Do final check with user
  if (!usrJoinVerifyCallback(&a_mac_rx_data.SrcAddr.laddr, capinfo)) {
    tmp = LRWPAN_ASSOC_STATUS_DENIED;
    goto macFormatAssociationResponse_dopkt;
  }
*/

  if ( ((LRWPAN_GET_CAPINFO_DEVTYPE(capinfo)) && (mac_pib.ChildRouters == LRWPAN_MAX_ROUTERS_PER_PARENT))
      ||
        (!(LRWPAN_GET_CAPINFO_DEVTYPE(capinfo)) && (mac_pib.ChildRFDs == LRWPAN_MAX_NON_ROUTER_CHILDREN)))
  {    //no room

    tmp = LRWPAN_ASSOC_STATUS_NOROOM;
    goto macFormatAssociationResponse_dopkt;

  }

  //not in table, Add this node，在这个邻居表里加入该节点，以后得改善
  new_saddr = ntAddNeighbor(&a_mac_rx_data.SrcAddr.laddr.bytes[0],capinfo);

  if (new_saddr == LRWPAN_BCAST_SADDR) { //this is an error indication, adding neighbor failed
      tmp = LRWPAN_ASSOC_STATUS_NOROOM;
      goto macFormatAssociationResponse_dopkt;
  }

  tmp = LRWPAN_ASSOC_STATUS_SUCCESS;

macFormatAssociationResponse_dopkt:
  //format the packet
  //status byte
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE-1];
  *phy_pib.currentTxFrm = tmp;

  //new short address for the RFD
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (new_saddr>>8);
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (new_saddr);

  //CMD
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = LRWPAN_MACCMD_ASSOC_RSP;

  phy_pib.currentTxFlen = LRWPAN_MACCMD_ASSOC_RSP_PAYLOAD_LEN;

  //send the packet
  a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_ACKREQ_MASK;
  //mini//a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_LADDR;
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_SADDR;//mini
  a_mac_tx_data.DestAddr.laddr.bytes[0] = a_mac_rx_data.SrcAddr.laddr.bytes[0];
  a_mac_tx_data.DestAddr.laddr.bytes[1] = a_mac_rx_data.SrcAddr.laddr.bytes[1];
  a_mac_tx_data.DestAddr.laddr.bytes[2] = a_mac_rx_data.SrcAddr.laddr.bytes[2];
  a_mac_tx_data.DestAddr.laddr.bytes[3] = a_mac_rx_data.SrcAddr.laddr.bytes[3];
  a_mac_tx_data.DestAddr.laddr.bytes[4] = a_mac_rx_data.SrcAddr.laddr.bytes[4];
  a_mac_tx_data.DestAddr.laddr.bytes[5] = a_mac_rx_data.SrcAddr.laddr.bytes[5];
  a_mac_tx_data.DestAddr.laddr.bytes[6] = a_mac_rx_data.SrcAddr.laddr.bytes[6];
  a_mac_tx_data.DestAddr.laddr.bytes[7] = a_mac_rx_data.SrcAddr.laddr.bytes[7];
  a_mac_tx_data.DestPANID = mac_pib.macPANID;
  a_mac_tx_data.SrcPANID = mac_pib.macPANID;
  macTxData();
}

#endif

//#ifndef LRWPAN_COORDINATOR
void macNetworkAssociateRequest(void)
{
  MACPKT *pkt;
  SADDR saddr;
  BYTE *ptr;
  UINT32 slot,time;
  //更新 phyCurrentChannel,macPANID
  phy_pib.phyCurrentChannel = a_mac_service.args.associate.request.LogicalChannel;
  halSetChannel(a_mac_service.args.associate.request.LogicalChannel);
  mac_pib.macPANID = a_mac_service.args.associate.request.CoordPANID;
  halSetRadioPANID(mac_pib.macPANID);
  if (a_mac_service.args.associate.request.CoordAddrMode==LRWPAN_ADDRMODE_SADDR)
    mac_pib.macCoordShortAddress = a_mac_service.args.associate.request.CoordAddress.saddr;
  else if (a_mac_service.args.associate.request.CoordAddrMode==LRWPAN_ADDRMODE_LADDR)
    halUtilMemCopy(&mac_pib.macCoordExtendedAddress.bytes[0], &a_mac_service.args.associate.request.CoordAddress.laddr.bytes[0], 8);

  mac_pib.flags.bits.WaitingForAssocResponse = 1;
  macResetRxBuffer();

  //此处应该添加，打开发射机
  phyGrabTxLock();
  macSendAssociateRequest();
  slot =  macGetCurSlotTimes();
  time = halGetMACTimer();
  //此处应该添加，打开接收机
/*
  //等到协调器的响应
  while (AckPendingTable[mac_pib.macDSNIndex].currentAckRetries) {
    while (halMACTimerNowDelta(AckPendingTable[mac_pib.macDSNIndex].TxStartTime)<mac_pib.macAckWaitDuration) {
      if (!AckPendingTable[mac_pib.macDSNIndex].options.bits.ACKPending) {
        break;
      }
    }
    if (!AckPendingTable[mac_pib.macDSNIndex].options.bits.ACKPending)
      break;
    else{      //是否有必要存在
      while (phyTxBusy()) {
        if (halMACTimerNowDelta(phy_pib.txStartTime) > MAX_TX_TRANSMIT_TIME) {
          phySetTxIdle();
          break;
        }
      }
      halRepeatSendPacket(mac_pib.macDSNIndex);
    }
  }

  //在接收到确认帧后，或重发mac_pib.macMaxAckRetries次后，删除缓存数据，清标志位
  if ((!AckPendingTable[mac_pib.macDSNIndex].currentAckRetries)||(!AckPendingTable[mac_pib.macDSNIndex].options.bits.ACKPending)) {
    MemFree(AckPendingTable[mac_pib.macDSNIndex].Ptr);
    AckPendingTable[mac_pib.macDSNIndex].options.bits.Used = 0;
  }
  //请求连接设备没有接收到确认帧，返回状态“LRWPAN_STATUS_MAC_NO_ACK”
  if (AckPendingTable[mac_pib.macDSNIndex].options.bits.ACKPending) {
    a_mac_service.status = LRWPAN_STATUS_MAC_NO_ACK;
    mac_pib.flags.bits.WaitingForAssocResponse = 0;
    macResetRxBuffer();
    return;
  }
*/
  //在aResponseWaitTime内，等待连接请响应求命令
   while(halMACTimerDelta(slot,time)<aResponseWaitTime) {
    if (!macRxBuffEmpty()) {
      pkt = macGetRxPacket();
      if (LRWPAN_IS_MAC(*(pkt->data+1))) {
        a_mac_rx_data.orgpkt = pkt;
        macParseHdr();
        ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;
        if (*ptr==LRWPAN_MACCMD_ASSOC_RSP) {
          saddr = macParseAssocResponse();
          if (saddr==0xFFFF) {
            a_mac_service.status = LRWPAN_STATUS_MAC_ASSOCIATION_DENY;
          } else {
            a_mac_service.status = LRWPAN_STATUS_SUCCESS;
            a_mac_service.args.associate.confirm.AssocShortAddress = saddr;
          }
          macFreeRxPacket(TRUE);//如果是连接响应命令帧，获取结果，跳出循环，释放内存
          macResetRxBuffer();
          mac_pib.flags.bits.WaitingForAssocResponse = 0;
          return;
        }
      }
      macFreeRxPacket(TRUE);//释放其它数据包内存
    }
    mac_pib.flags.bits.WaitingForAssocResponse = 0;
    a_mac_service.status = LRWPAN_STATUS_MAC_NO_DATA;
  }	

}
//#endif

void macSendOrfhanNotify(void)
{
  //UINT32 mac_utility_timer;

  a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC;
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
  a_mac_tx_data.DestAddr.saddr = LRWPAN_BCAST_SADDR;
  a_mac_tx_data.DestPANID = LRWPAN_BCAST_PANID;
  a_mac_tx_data.SrcPANID = LRWPAN_BCAST_PANID;
  macFormatOrphanNotify();
  mac_pib.flags.bits.GotOrphanResponse = 0;
  mac_pib.flags.bits.WaitingForOrphanResponse = 1;
  macTxData();
/*
  if (a_mac_service.status != LRWPAN_STATUS_SUCCESS) {
    DEBUG_STRING(DBG_INFO,"Orphan Notify TX failed!\n");
    mac_pib.flags.bits.WaitingForOrphanResponse = 0;
    return;
  }

  mac_utility_timer = halGetMACTimer();
  while ((halMACTimerNowDelta(mac_utility_timer)) < MAC_ORPHAN_WAIT_TIME) {
    if (mac_pib.flags.bits.GotOrphanResponse) {//rejoin successfull
      mac_pib.flags.bits.WaitingForOrphanResponse = 0;
      break;
    }
  }

  if (mac_pib.flags.bits.GotOrphanResponse) {//rejoin successfull
    a_mac_service.status = LRWPAN_STATUS_SUCCESS;
    mac_pib.flags.bits.GotOrphanResponse = 0;
    DEBUG_STRING(DBG_INFO,"MAC: Got Orphan response.\n");
  } else {//timeout on rejoin, give it up
    a_mac_service.status = LRWPAN_STATUS_MAC_ORPHAN_TIMEOUT;
    mac_pib.flags.bits.WaitingForOrphanResponse = 0;
    DEBUG_STRING(DBG_INFO,"MAC: Orphan response timeout.\n");
  }
  */
}

BOOL macGetPANInfo(BYTE logicalchannel,BYTE beaconnum)
{
  MACPKT *pkt;
  BYTE *ptr;
  BYTE i;
  //BYTE addrmode;

  if (logicalchannel<11||logicalchannel>26){
    return FALSE;
  }
  if (macRxBuffEmpty()) {
    return FALSE;
  }

  pkt = macGetRxPacket();

  if (LRWPAN_IS_BCN(*(pkt->data+1))) {

    a_mac_rx_data.orgpkt = pkt;
    macParseHdr();

//mini
  //copy the SRC long address as my coordinator long address
  mac_pib.macCoordExtendedAddress.bytes[0] = a_mac_rx_data.SrcAddr.laddr.bytes[0];
  mac_pib.macCoordExtendedAddress.bytes[1] = a_mac_rx_data.SrcAddr.laddr.bytes[1];
  mac_pib.macCoordExtendedAddress.bytes[2] = a_mac_rx_data.SrcAddr.laddr.bytes[2];
  mac_pib.macCoordExtendedAddress.bytes[3] = a_mac_rx_data.SrcAddr.laddr.bytes[3];
  mac_pib.macCoordExtendedAddress.bytes[4] = a_mac_rx_data.SrcAddr.laddr.bytes[4];
  mac_pib.macCoordExtendedAddress.bytes[5] = a_mac_rx_data.SrcAddr.laddr.bytes[5];
  mac_pib.macCoordExtendedAddress.bytes[6] = a_mac_rx_data.SrcAddr.laddr.bytes[6];
  mac_pib.macCoordExtendedAddress.bytes[7] = a_mac_rx_data.SrcAddr.laddr.bytes[7];
//mini

    ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;
    /*
    if (!(*(ptr+1) & LRWPAN_BEACON_SF_PAN_COORD_MASK)) {//如果是协调器，则继续
      DEBUG_STRING(DBG_INFO,"Have recieve a router beacon response!\n ");
      return FALSE;
    }
    */
    if (LRWPAN_GET_SECURITY_ENABLED(a_mac_rx_data.fcfmsb))
      PANDescriptorList[beaconnum].options.bits.SecurityUse = 1;
    else
      PANDescriptorList[beaconnum].options.bits.SecurityUse = 0;
    for (i=0;i<8;i++) {
      PANDescriptorList[beaconnum].ExtendedAddress.bytes[i] = a_mac_rx_data.SrcAddr.laddr.bytes[i];
      }
    /*
    addrmode = LRWPAN_GET_SRC_ADDR(a_mac_rx_data.fcfmsb);
    if (addrmode==LRWPAN_ADDRMODE_SADDR) {
      PANDescriptorList[beaconnum].ShortAddress = a_mac_rx_data.SrcAddr.saddr;
    } else if (addrmode==LRWPAN_ADDRMODE_LADDR) {
      for (i=0;i<8;i++) {
        PANDescriptorList[beaconnum].ExtendedAddress.bytes[i] = a_mac_rx_data.SrcAddr.laddr.bytes[i];
      }
    }
    */
    PANDescriptorList[beaconnum].CoordPANId = a_mac_rx_data.SrcPANID;
    PANDescriptorList[beaconnum].LogicalChannel = logicalchannel;
    PANDescriptorList[beaconnum].LinkQuality = a_mac_rx_data.LQI;

    PANDescriptorList[beaconnum].SuperframeSpec = *ptr;
    ptr++;
    PANDescriptorList[beaconnum].SuperframeSpec += (((UINT16)*ptr)<<8);
    if (*ptr & LRWPAN_BEACON_SF_PAN_COORD_MASK)
      PANDescriptorList[beaconnum].options.bits.PANCoordinator = 1;
    else
      PANDescriptorList[beaconnum].options.bits.PANCoordinator = 0;
    if (*ptr & LRWPAN_BEACON_SF_ASSOC_PERMIT_MASK)
      PANDescriptorList[beaconnum].options.bits.AssociationPermit = 1;
    else
      PANDescriptorList[beaconnum].options.bits.AssociationPermit = 0;
    if (*ptr & LRWPAN_BEACON_SF_BATTLIFE_EXTEN_PERMIT_MASK)
      PANDescriptorList[beaconnum].options.bits.BatteryLifeExtension = 1;
    else
      PANDescriptorList[beaconnum].options.bits.BatteryLifeExtension = 0;

    ptr++;//GTS
    if (*ptr & LRWPAN_BEACON_GF_GTS_PERMIT_MASK)
      PANDescriptorList[beaconnum].options.bits.GTSPermit = 1;
    else
      PANDescriptorList[beaconnum].options.bits.GTSPermit = 0;

    ptr++;//Pending address fields

    ptr++;//Zigbee protocol ID

    ptr++;//stack protocol
    PANDescriptorList[beaconnum].StackProfile = *ptr;

    ptr++;//protocol version
    PANDescriptorList[beaconnum].ZigBeeVersion = *ptr;

    ptr++;
    if (*ptr == 0)
      PANDescriptorList[beaconnum].options.bits.RouterRoom = 0;
    else
      PANDescriptorList[beaconnum].options.bits.RouterRoom = 1;

    ptr++;
    PANDescriptorList[beaconnum].Depth = *ptr;

    ptr++;
    if (*ptr == 0)
      PANDescriptorList[beaconnum].options.bits.RfdRoom = 0;
    else
      PANDescriptorList[beaconnum].options.bits.RfdRoom = 1;

    ptr++;
    PANDescriptorList[beaconnum].ShortAddress = *ptr;
    ptr++;
    PANDescriptorList[beaconnum].ShortAddress += (((UINT16)*ptr)<<8);

    macFreeRxPacket(TRUE);
    return TRUE;
  } else {
    macFreeRxPacket(TRUE);
    return FALSE;
  }		
	
}


BOOL macGetCoordRealignInfo(void)
{
	BYTE *ptr;

	if (macRxBuffEmpty())
		return FALSE;
	a_mac_rx_data.orgpkt = macGetRxPacket();
	macParseHdr();
	ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;
	if (*ptr==LRWPAN_MACCMD_COORD_REALIGN) {
		macParseOrphanResponse();

		macFreeRxPacket(TRUE);
		return TRUE;
	} else {
		macFreeRxPacket(TRUE);
		return FALSE;
	}		
}

UINT8 macRadioComputeED(INT8 rssiDbm)
{
  UINT8 ed;
  //UINT8 diff;
  if (rssiDbm < ED_RF_POWER_MIN_DBM) {
    rssiDbm = ED_RF_POWER_MIN_DBM;
  } else if (rssiDbm > ED_RF_POWER_MAX_DBM) {
    rssiDbm = ED_RF_POWER_MAX_DBM;
  }
  //ed = (255*(rssiDbm-(-81)))/(10-(-81));这样计算不合理，应更改
  ed = (MAC_SPEC_ED_MAX * (rssiDbm - ED_RF_POWER_MIN_DBM)) / (ED_RF_POWER_MAX_DBM - ED_RF_POWER_MIN_DBM);
  //diff = (UINT8)(rssiDbm + 81);
  //ed = diff<<1;
  //ed = ed + (((UINT16)diff)<<3)/10;
  return(ed);
}


UINT8 macRadioComputeLQI(UINT8 corr)
{
  UINT8 lqi;

  if (corr > MAC_RADIO_CORR_MAX)
    corr = MAC_RADIO_CORR_MAX;
  else if (corr < MAC_RADIO_CORR_MIN)
    corr = MAC_RADIO_CORR_MIN;
  //lqi = (255*(corr-50))/(110-50);这样计算不合理，应更改
  lqi = (MAC_SPEC_LQI_MAX * (corr - MAC_RADIO_CORR_MIN)) / (MAC_RADIO_CORR_MAX - MAC_RADIO_CORR_MIN);
  //lqi = (corr - MAC_RADIO_CORR_MIN)<<2 + (corr - MAC_RADIO_CORR_MIN)>>2;
  return(lqi);

}


void macScan(void)
{
  INT8 maxRssi,rssiDbm;
  BYTE i,j,beaconnum;
  SCAN_TYPE_ENUM scantype;
  UINT32 startscantime;
  UINT32 startscanslot;
  UINT32 scandurationpertimes;
  UINT16 scanchannels;
  //(60*16)<<4 + 60*16 = 16320
  scandurationpertimes = ((UINT32)aBaseSuperFrameDuration<<a_mac_service.args.scan.request.ScanDuration) + aBaseSuperFrameDuration;
  scanchannels = (UINT16)(a_mac_service.args.scan.request.ScanChannels>>11);
	
  scantype = a_mac_service.args.scan.request.ScanType;

  switch (scantype) {

  case ENERGY_DETECT:
    //打开接收机
    /*a_phy_service.cmd = LRWPAN_SVC_PHY_SET_TRX;
    a_phy_service.args.set_trx.state = RX_ON;
    phyDoService();*/

    a_mac_service.args.scan.confirm.ScanType = a_mac_service.args.scan.request.ScanType;
    a_mac_service.args.scan.confirm.UnscanChannels = a_mac_service.args.scan.request.ScanChannels;
    a_mac_service.args.scan.confirm.ResultListSize = 0;

    for (j=0,i=11;j<16;j++,i++) {
      maxRssi = -128;

      if ((scanchannels>>j)&&0x0001){

        if (macInitRadio(i, LRWPAN_BCAST_PANID) != LRWPAN_STATUS_SUCCESS) {
          continue;
        }

        startscanslot = macGetCurSlotTimes();
        startscantime = halGetMACTimer();
        do {
          a_phy_service.cmd = LRWPAN_SVC_PHY_ED;
          phyDoService();
          if (a_phy_service.status==LRWPAN_STATUS_SUCCESS) {
            if (a_phy_service.args.ed.EnergyLevel>maxRssi)
              maxRssi = a_phy_service.args.ed.EnergyLevel;
          } else {
            //打开接收机
            /*a_phy_service.cmd = LRWPAN_SVC_PHY_SET_TRX;
            a_phy_service.args.set_trx.state = RX_ON;
            phyDoService();*/
          }
        } while(halMACTimerDelta(startscanslot,startscantime)<scandurationpertimes);
        a_mac_service.args.scan.confirm.ResultListSize++;
        a_mac_service.args.scan.confirm.UnscanChannels &= ~(0x00000001<<i);
      }
      rssiDbm = maxRssi + MAC_RADIO_RSSI_OFFSET;
      EnergyDetectList[j] = macRadioComputeED(rssiDbm);

    }
    a_mac_service.status = LRWPAN_STATUS_SUCCESS;
    break;

  case ACTIVE:
    a_mac_service.args.scan.confirm.ScanType = a_mac_service.args.scan.request.ScanType;
    a_mac_service.args.scan.confirm.UnscanChannels = a_mac_service.args.scan.request.ScanChannels;
    a_mac_service.args.scan.confirm.ResultListSize = 0;
    beaconnum = 0;
    for (j=0,i=11;j<16;j++,i++) {
      if ((scanchannels>>j)&&0x0001){


        if (macInitRadio(i, LRWPAN_BCAST_PANID) != LRWPAN_STATUS_SUCCESS) {
          continue;
        }

        //主动扫描中，应在发送信标请求帧前，清空接收缓存器释放内存，以后实现。
        macResetRxBuffer();

        phyGrabTxLock();//发送装载机肯定为空，将其锁住，准备发送信标请求命令
        macSendBeaconRequest();

        //开启接收机，准备接收信标帧响应
        /*a_phy_service.cmd = LRWPAN_SVC_PHY_SET_TRX;
        a_phy_service.args.set_trx.state = RX_ON;
        phyDoService();*/

        startscanslot = macGetCurSlotTimes();
        startscantime = halGetMACTimer();
        do {
          //从接收缓存器中，获取信标帧，便进行存储
          if (macGetPANInfo(i,beaconnum)) {
            beaconnum++;
          }
         // halWait(10);
          if (beaconnum==16)
            break;
        } while(halMACTimerDelta(startscanslot,startscantime)<scandurationpertimes);
        a_mac_service.args.scan.confirm.UnscanChannels &= ~(0x00000001<<i);
      }

      if (beaconnum==16)
        break;
    }

    if (beaconnum==0)
      a_mac_service.status = LRWPAN_STATUS_MAC_NO_BEACON;
    else
      a_mac_service.status = LRWPAN_STATUS_SUCCESS;

    a_mac_service.args.scan.confirm.ResultListSize = beaconnum;
    break;

  case PASSIVE:
    a_mac_service.args.scan.confirm.ScanType = a_mac_service.args.scan.request.ScanType;
    a_mac_service.args.scan.confirm.UnscanChannels = a_mac_service.args.scan.request.ScanChannels;
    a_mac_service.args.scan.confirm.ResultListSize = 0;
    beaconnum = 0;

    for (j=0,i=11;j<16;j++,i++) {
      if ((scanchannels>>j)&&0x0001){
         if (macInitRadio(i, LRWPAN_BCAST_PANID) != LRWPAN_STATUS_SUCCESS) {
          continue;
        }

        //被动扫描中，应在准备接收该信道的信标帧前，清空接收缓存器释放内存，以后实现。
        macResetRxBuffer();

        //开启接收机，准备接收信标帧响应
        /*a_phy_service.cmd = LRWPAN_SVC_PHY_SET_TRX;
        a_phy_service.args.set_trx.state = RX_ON;
        phyDoService();*/

        //maxenergy = 0;
        startscanslot = macGetCurSlotTimes();
        startscantime = halGetMACTimer();
        do {
          //从接收缓存器中，获取信标帧，便进行存储
          if (macGetPANInfo(i,beaconnum)) {
            beaconnum++;
          }
          if (beaconnum==16)
            break;
        } while(halMACTimerDelta(startscanslot,startscantime)<scandurationpertimes);
        a_mac_service.args.scan.confirm.UnscanChannels &= ~(0x00000001<<i);
      }

      if (beaconnum==16)
        break;
    }

    if (beaconnum==0)
      a_mac_service.status = LRWPAN_STATUS_MAC_NO_BEACON;
    else
      a_mac_service.status = LRWPAN_STATUS_SUCCESS;

    a_mac_service.args.scan.confirm.ResultListSize = beaconnum;
    break;

  case ORPHAN:
    a_mac_service.args.scan.confirm.ScanType = a_mac_service.args.scan.request.ScanType;
    beaconnum = 0;
    for (i=0;i<16;i++) {
      if ((a_mac_service.args.scan.request.ScanChannels>>(i+11))&&1){
        macSetChannel(i+11);//设置信道频率
        a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC;
        a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
        a_mac_tx_data.DestAddr.saddr = LRWPAN_BCAST_PANID;
        a_mac_tx_data.DestPANID = LRWPAN_BCAST_PANID;
        a_mac_tx_data.SrcPANID = LRWPAN_BCAST_PANID;
        macFormatOrphanNotify();
        mac_pib.flags.bits.GotOrphanResponse = 0;
        mac_pib.flags.bits.WaitingForOrphanResponse = 1;
        macTxData();		
        //开启接收机，准备接收信标帧响应
        /*a_phy_service.cmd = LRWPAN_SVC_PHY_SET_TRX;
        a_phy_service.args.set_trx.state = RX_ON;
        phyDoService();*/

        startscanslot = macGetCurSlotTimes();
        startscantime = halGetMACTimer();
        do {
          //从接收缓存器中，获取孤立响应(协调器再连接命令)，便进行存储
          if (macGetCoordRealignInfo()) {
            //a_mac_service.status = LRWPAN_STATUS_SUCCESS;
            beaconnum++;
            break;
          }
        } while(halMACTimerNowDelta(startscantime)<aResponseWaitTime);
      }
      if (beaconnum==1) {
        a_mac_service.status = LRWPAN_STATUS_SUCCESS;
        break;
      } else
        a_mac_service.status = LRWPAN_STATUS_MAC_NO_BEACON;
    }
    break;
  default: break;
  }
}

/*函数功能：将MAC层的相关参数设置为最初状态和关闭设备的收发机。
          升级超帧配置和连接请求之前，将自己同网络断开之后，一般要执行复位操作*/
void macPibReset(void)
{
  if (a_mac_service.args.reset.SetDefaultPIB) {//将所有的MAC PIB属性值设置为缺省值
    mac_pib.macCoordShortAddress = 0;
    mac_pib.flags.val = 0;
    mac_pib.macMaxAckRetries = aMaxFrameRetries;//3

    mac_pib.macBeaconOrder = 15;
    mac_pib.macSuperframeOrder =15;

    macResetRxBuffer();
    macResetAckPendingTable();

#ifdef LRWPAN_COORDINATOR
    mac_pib.macDepth = 0;
    mac_pib.flags.bits.macPanCoordinator = 1;
#else
    mac_pib.macDepth = 1; //depth will be at least one
    mac_pib.flags.bits.macPanCoordinator = 0;
#endif

    mac_pib.bcnDepth = 0xFF; //remembers depth of node that responded to beacon other capability information
    mac_pib.macCapInfo = 0;
#ifdef LRWPAN_ALT_COORDINATOR     //not supported, included for completeness
    LRWPAN_SET_CAPINFO_ALTPAN(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_FFD
    LRWPAN_SET_CAPINFO_DEVTYPE(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_ACMAIN_POWERED
    LRWPAN_SET_CAPINFO_PWRSRC(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_RCVR_ON_WHEN_IDLE
    LRWPAN_SET_CAPINFO_RONIDLE(mac_pib.macCapInfo);
#endif
#ifdef LRWPAN_SECURITY_CAPABLE
    LRWPAN_SET_CAPINFO_SECURITY(mac_pib.macCapInfo);
#endif
    //always allocate a short address
    LRWPAN_SET_CAPINFO_ALLOCADDR(mac_pib.macCapInfo);

#ifdef LRWPAN_FFD
    //初始化邻居设备数目和短地址分配
    ntInitAddressAssignment();
#endif
  } else {//将所有的MAC PIB属性值保留在复位请求之前的值
    macResetRxBuffer();
    macResetAckPendingTable();
#ifdef LRWPAN_FFD
    //初始化邻居设备数目和短地址分配
    ntInitAddressAssignment();
#endif
  }

  //关闭设备的收发机
  //如果关闭设备的收发机成功，返回状态为LRWPAN_STATUS_SUCCESS，否则为LRWPAN_STATUS_MAC_DISABLE_TRX_FAILURE
  a_mac_service.status = LRWPAN_STATUS_SUCCESS;
}


#ifdef LRWPAN_FFD
void macStartNewSuperframe(void)
{
  if (mac_pib.macShortAddress==LRWPAN_BCAST_SADDR) {
    a_mac_service.status = LRWPAN_STATUS_MAC_NO_SHORT_ADDRESS;
    return;
  }

  mac_pib.macBeaconOrder = a_mac_service.args.start.BeaconOrder;
  if (mac_pib.macBeaconOrder==15)
    mac_pib.macSuperframeOrder = 15;
  else		
    mac_pib.macSuperframeOrder = a_mac_service.args.start.SuperframeOrder;
  if (a_mac_service.args.start.options.bits.PANCoordinator) {
    mac_pib.macPANID = a_mac_service.args.start.PANID;
    phy_pib.phyCurrentChannel = a_mac_service.args.start.LogicalChannel;
  }
  mac_pib.flags.bits.macBattLifeExt = a_mac_service.args.start.options.bits.BatteryLifeExtension;

  //开启发射机
  /*a_phy_service.cmd = LRWPAN_SVC_PHY_SET_TRX;
  a_phy_service.args.set_trx.state = TX_ON;
  phyDoService();*/

  macSendBeacon();//广播信标帧

}
#endif

//check for DATA packets
//For RFD, just check if this packet came from parent
//For Routers, if this uses SHORT addressing, then check
//to see if this is associated with us
/*函数功能:MAC层接收缓存区已经接收一个数据包，检查是否应该接受，true 表示接受*/
static BOOL macCheckDataRejection(void){

	BYTE AddrMode,i;

	
	//if not associated, reject
/*如果该设备不是协调器，看是否已被连接，若没有，则拒绝;如果该设备是协调器，就不用检查该步*/
#ifndef LRWPAN_COORDINATOR		
	if (!mac_pib.flags.bits.macIsAssociated) {
		
		return(FALSE);
	}
#endif
    AddrMode = LRWPAN_GET_DST_ADDR(a_mac_rx_data.fcfmsb);
/*目的地址模式为扩展地址时，则接受*/
	if (AddrMode == LRWPAN_ADDRMODE_LADDR) {//this packet send directly to our long address. accept it.
		return(TRUE);
	}

	//check the parent
/*根据源地址，判断是否是协调器发送的数据，若是，则接受*/
	AddrMode = LRWPAN_GET_SRC_ADDR(a_mac_rx_data.fcfmsb);
	if (AddrMode == LRWPAN_ADDRMODE_SADDR) {
		//check parent short address
		if (a_mac_rx_data.SrcAddr.saddr == mac_pib.macCoordShortAddress)
			return(TRUE);
	} else if (AddrMode == LRWPAN_ADDRMODE_LADDR){
		//check parent long address.
		for (i=0;i<8;i++) {
			if (a_mac_rx_data.SrcAddr.laddr.bytes[i] !=
				mac_pib.macCoordExtendedAddress.bytes[i])
				break;
		}
		if (i==8) return(TRUE); //have a match
	}
#ifdef LRWPAN_RFD
	return(FALSE);
#else
	//ok, for FFDs, check the neighbor table
	if (AddrMode == LRWPAN_ADDRMODE_SADDR){
		if (ntFindBySADDR (a_mac_rx_data.SrcAddr.saddr) !=(NAYBORENTRY *) NULL)
			return(TRUE);
	}else if (AddrMode == LRWPAN_ADDRMODE_LADDR){
        if (ntFindByLADDR (&a_mac_rx_data.SrcAddr.laddr))
			return(TRUE);
	}

	return(FALSE);
#endif
}

static void macFormatSyncRequest(){
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =LRWPAN_MACCMD_SYNC_REQ;
  phy_pib.currentTxFlen =1;
  a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_ACKREQ_MASK;
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_SADDR;
  a_mac_tx_data.DestAddr.saddr = mac_pib.bcnSADDR;
  a_mac_tx_data.DestPANID = mac_pib.bcnPANID;
  mac_pib.flags.bits.WaitingForSyncRep = 1;
  a_mac_tx_data.SrcPANID = mac_pib.bcnPANID;
  a_mac_tx_data.SrcAddr = macGetShortAddr();
  mac_pib.flags.bits.TransmittingSyncReq=1;
}

#ifdef LRWPAN_COORDINATOR
static void macFormatSyncRsp(){
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
  phy_pib.currentTxFrm-=6;
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM1);
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM1>>8);
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM1>>16);
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM1_1);
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM1_1>>8);
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM1_1>>16);
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =LRWPAN_MACCMD_SYNC_RSP;
  phy_pib.currentTxFlen =7;
  a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_ACKREQ_MASK;
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_SADDR;
  a_mac_tx_data.DestPANID = a_mac_rx_data.SrcPANID;
  a_mac_tx_data.DestAddr=a_mac_rx_data.SrcAddr;
  a_mac_tx_data.SrcPANID = mac_pib.macPANID;
  a_mac_tx_data.SrcAddr = macGetShortAddr();
  mac_pib.flags.bits.TransmittingSyncRep=1;
  macTxData();
}
#endif

static void macParseSyncRsp(){
UINT32 CurrentTime,tm,ts;
BYTE *ptr;

mac_pib.flags.bits.WaitingForSyncRep=0;
ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;
if(*ptr==LRWPAN_MACCMD_SYNC_RSP)
ptr++;
mac_sync_time.TM1_1=((UINT32)(*ptr))<<16;
ptr++;
mac_sync_time.TM1_1+=((UINT32)(*ptr))<<8;
ptr++;
mac_sync_time.TM1_1+=(UINT32)(*ptr);
ptr++;
mac_sync_time.TM1=((UINT32)(*ptr))<<16;
ptr++;
mac_sync_time.TM1+=((UINT32)(*ptr))<<8;
ptr++;
mac_sync_time.TM1+=*ptr;

ptr++;
mac_sync_time.TM2_1=((UINT32)(*ptr))<<16;
ptr++;
mac_sync_time.TM2_1+=((UINT32)(*ptr))<<8;
ptr++;
mac_sync_time.TM2_1+=(UINT32)(*ptr);
ptr++;
mac_sync_time.TM2=((UINT32)(*ptr))<<16;
ptr++;
mac_sync_time.TM2+=((UINT32)(*ptr))<<8;
ptr++;
mac_sync_time.TM2+=*ptr;

if(mac_sync_time.TM2_1>mac_sync_time.TM1_1){
	mac_sync_time.TM2_1-=1;
	mac_sync_time.TM2+=0x186A;
	}

if(mac_sync_time.TS2_1>mac_sync_time.TS1_1){
	mac_sync_time.TS2_1-=1;
	mac_sync_time.TS2+=0x186A;
	}

if(mac_sync_time.TM1_1>mac_sync_time.TS1_1){
  mac_sync_time.Offset_1=mac_sync_time.TM1_1-mac_sync_time.TS1_1;
  mac_pib.macCurrentSlotTimes+=mac_sync_time.Offset_1;
	}
if(mac_sync_time.TS1_1>mac_sync_time.TM1_1){
  mac_sync_time.Offset_1=mac_sync_time.TS1_1-mac_sync_time.TM1_1;
  mac_pib.macCurrentSlotTimes-=mac_sync_time.Offset_1;
  }

tm=mac_sync_time.TM1+mac_sync_time.TM2;
ts=mac_sync_time.TS1+mac_sync_time.TS2;
if(tm>ts){
  mac_sync_time.Offset=(tm-ts)/2;
  CurrentTime=halGetMACTimer();
  CurrentTime+=mac_sync_time.Offset;
  if(CurrentTime>0x186A) {
  	CurrentTime-=0x186A;
	mac_pib.macCurrentSlotTimes+=1;
	mac_pib.macCurrentSlot+=1;
	}
}
if(tm<ts){
  mac_sync_time.Offset=(ts-tm)/2;
  CurrentTime=halGetMACTimer();
  if(mac_sync_time.Offset>CurrentTime) {
  	CurrentTime+=0x186A;
	mac_pib.macCurrentSlotTimes-=1;
	mac_pib.macCurrentSlot-=1;
	}
  CurrentTime-=mac_sync_time.Offset;
}

INT_GLOBAL_ENABLE(INT_OFF);
//CC2530(T2OFx>T2MOVFx)
T2MOVF0=(CurrentTime) &0xff;
T2MOVF1=(CurrentTime>>8) &0xff;
T2MOVF2=(CurrentTime>>16) &0x0f;
INT_GLOBAL_ENABLE(INT_ON);
/*
	DEBUG_STRING(DBG_INFO,"\nthe offset is:");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.Offset_1);
	DEBUG_STRING(DBG_INFO,"     ");	
	if(ts>tm)
	DEBUG_STRING(DBG_INFO,"-");	
	DEBUG_UINT32(DBG_INFO, mac_sync_time.Offset);
	DEBUG_STRING(DBG_INFO,"\n");

	DEBUG_STRING(DBG_INFO,"\nTS1 is:");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TS1_1);
	DEBUG_STRING(DBG_INFO,"     ");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TS1);
	DEBUG_STRING(DBG_INFO,"\n");

	DEBUG_STRING(DBG_INFO,"\nTM1 is:");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TM1_1);
	DEBUG_STRING(DBG_INFO,"     ");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TM1);
	DEBUG_STRING(DBG_INFO,"\n");

	DEBUG_STRING(DBG_INFO,"\nTM2 is:");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TM2_1);
	DEBUG_STRING(DBG_INFO,"     ");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TM2);
	DEBUG_STRING(DBG_INFO,"\n");

	DEBUG_STRING(DBG_INFO,"\nTS2 is:");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TS2_1);
	DEBUG_STRING(DBG_INFO,"     ");
	DEBUG_UINT32(DBG_INFO, mac_sync_time.TS2);
	DEBUG_STRING(DBG_INFO,"\n");

	DEBUG_STRING(DBG_INFO,"\nT1&T2 is:");
	DEBUG_UINT32(DBG_INFO, t1);
	DEBUG_STRING(DBG_INFO,"     ");
	DEBUG_UINT32(DBG_INFO, t2);
	DEBUG_STRING(DBG_INFO,"\n");
	*/
	mac_sync_time.Offset=0;
	mac_sync_time.TM1=0;
	mac_sync_time.TM2=0;
	mac_sync_time.TS1=0;
	mac_sync_time.TS2=0;
	
	mac_sync_time.Offset_1=0;
	mac_sync_time.TM1_1=0;
	mac_sync_time.TM2_1=0;
	mac_sync_time.TS1_1=0;
	mac_sync_time.TS2_1=0;
}

static void macFormatGTSRequest(void){
  phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm = a_mac_service.args.GTSRequest.requestLength & 0x0F;
  *phy_pib.currentTxFrm  |= a_mac_service.args.GTSRequest.requestDirection<<4;
  *phy_pib.currentTxFrm  |= a_mac_service.args.GTSRequest.requestCharacter <<5;
  phy_pib.currentTxFrm--;
  *phy_pib.currentTxFrm =LRWPAN_MACCMD_GTS_REQ;
  phy_pib.currentTxFlen = 2;

  a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_MAC|LRWPAN_FCF_ACKREQ_MASK;
  a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_SADDR;         a_mac_tx_data.DestAddr.saddr = mac_pib.bcnSADDR;
  a_mac_tx_data.DestPANID = mac_pib.bcnPANID;
  a_mac_tx_data.SrcPANID = mac_pib.bcnPANID;
  a_mac_tx_data.DestAddr.saddr = mac_pib.bcnSADDR;
  a_mac_tx_data.SrcAddr = macGetShortAddr();
  //mac_pib.flags.bits.TransmittingGTSReq=1;
 }

#ifdef LRWPAN_COORDINATOR
static void macParseGTSRequest(void){
 int i,j;
 BYTE *ptr;
 BYTE length,currentSlot;
 BOOL direction,character;
 ptr = a_mac_rx_data.orgpkt->data + a_mac_rx_data.pload_offset;
if(*ptr==LRWPAN_MACCMD_GTS_REQ)
	ptr++;
length=(*ptr) & 0x0F;
direction=(*ptr) & (1<<4);
character=(*ptr) & (1<<5);

if(character){//添加GTS	
	if(mac_pib.finalSlot-length>=9){
		mac_pib.finalSlot-=length;
		currentSlot=mac_pib.finalSlot+1;
		gtsAllocate[mac_pib.GTSDescriptCount].address1=a_mac_rx_data.SrcAddr.saddr;
		gtsAllocate[mac_pib.GTSDescriptCount].GTSInformation=currentSlot;
		gtsAllocate[mac_pib.GTSDescriptCount].GTSInformation+=(length<<4);
		for(i=0;i<length;i++){
			if(direction) gtsAllocate[mac_pib.GTSDescriptCount].gtsDirection|=(1<<(currentSlot-8));
			else gtsAllocate[mac_pib.GTSDescriptCount].gtsDirection&=(0>>(currentSlot-8));
			currentSlot++;
		}	
		mac_pib.GTSDescriptCount++;
	}
	if(mac_pib.finalSlot==9) mac_pib.flags.bits.macGTSPermit=0;
}else{//解除GTS

	for(i=0;i<mac_pib.GTSDescriptCount;i++){
	if(gtsAllocate[i].address1!=a_mac_rx_data.SrcAddr.saddr) continue;
	length=(gtsAllocate[i].GTSInformation)>>4 &0x0F;
	currentSlot=gtsAllocate[i].GTSInformation &0x0F;
	for(j=i;j<mac_pib.GTSDescriptCount-1;j++){
		gtsAllocate[j].GTSInformation =gtsAllocate[j+1].GTSInformation + length;
		gtsAllocate[j].address1=gtsAllocate[j+1].address1;
		gtsAllocate[j].gtsDirection=(gtsAllocate[j+1].gtsDirection)<<length;
		}
	gtsAllocate[mac_pib.GTSDescriptCount-1].address1=0;
	gtsAllocate[mac_pib.GTSDescriptCount-1].GTSInformation=0;
	mac_pib.finalSlot +=length;
	mac_pib.GTSDescriptCount-=1;
	break;
	}
	}
mac_pib.GTSDirection=0;
for(i=0;i<mac_pib.GTSDescriptCount;i++){
	mac_pib.GTSDirection|=gtsAllocate[i].gtsDirection;
	}
}
#endif

