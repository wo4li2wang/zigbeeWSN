#include "compiler.h"
#include "lrwpan_config.h"         //user configurations
#include "lrwpan_common_types.h"   //types common acrosss most files
#include "ieee_lrwpan_defs.h"
#include "memalloc.h"
#include "neighbor.h"
#include "hal.h"
#include "halStack.h"
#include "phy.h"
#include "mac.h"
#include "nwk.h"
#include "aps.h"
#include "neighbor.h"
#ifdef SECURITY_GLOBALS
#include "security.h"
#endif

NWK_RXSTATE_ENUM nwkRxState;
NWK_NETWORK_DESCRIPTOR NetworkDescriptorList[16];
NWK_SERVICE a_nwk_service;
NWK_STATE_ENUM nwkState;
NWK_RX_DATA a_nwk_rx_data;
NWK_TX_DATA a_nwk_tx_data;
BYTE nwkDSN;
NWK_PIB nwk_pib;

extern BYTE EnergyDetectList[MAC_SCAN_SIZE];
extern SCAN_PAN_INFO PANDescriptorList[MAC_SCAN_SIZE];



/*
//locals
#ifndef LRWPAN_COORDINATOR		//如果不是协调器就执行下面的代码
static UINT32 nwk_utility_timer;   //utility timer
static UINT8 nwk_retries;       //utility retry counter
#endif
*/


/*函数功能:网络层相关参数初始化*/
void nwkInit(void){	
  nwkDSN = 0;		
  nwk_pib.flags.val = 0;
  nwkState = NWK_STATE_IDLE;
  nwkRxState= NWK_RXSTATE_IDLE;
#ifdef LRWPAN_FFD
  nwk_pib.rxTail = 0;	//对全功能设备的接收堆的初始化
  nwk_pib.rxHead = 0;
#endif
  nwkInitNetworkDescriptorList();
}

void nwkInitNetworkDescriptorList(void)
{
  BYTE i;
  nwk_pib.nwkNetworkCount = 0;
  for (i=0;i<16;i++) {
    NetworkDescriptorList[i].LinkQuality = 0;
  }
}

BOOL nwkCheckLaddrNull(LADDR *ptr)
{
  BYTE i;
  for (i=0;i<8;i++) {
    if (ptr->bytes[i]!=0)
      break;
  }
  if (i==8)
    return TRUE;
  else
    return FALSE;
}

/*函数功能:网络层状态机函数*/
void nwkFSM(void){

  macFSM();//调用mac状态机
  nwkRxFSM();//调用接收状态机

  switch (nwkState) {
  case NWK_STATE_IDLE://see if we have packets to forward and can grab the TX buffer
#ifdef LRWPAN_FFD
    if (!nwkRxBuffEmpty() && phyTxUnLocked()) { //grab the lock and forward the packet
      phyGrabTxLock();
      nwkCopyFwdPkt();//transmit it
      nwkTxData(TRUE); //use TRUE as this is a forwarded packet
    }
#endif
    break;
  case NWK_STATE_COMMAND_START:

    switch(a_nwk_service.cmd) {
    case LRWPAN_SVC_NWK_GENERIC_TX:
      nwkTxData(FALSE);	//不是转发，而是自己产生的一个网络报文
      nwkState = NWK_STATE_IDLE;
      break;
    case LRWPAN_SVC_NWK_GTS_REQ:
      a_mac_service.cmd=LRWPAN_SVC_MAC_GTS_REQ;
      a_mac_service.args.GTSRequest.requestCharacter=a_nwk_service.args.GTSRequest.requestCharacter;
      a_mac_service.args.GTSRequest.requestDirection=a_nwk_service.args.GTSRequest.requestDirection;
      a_mac_service.args.GTSRequest.requestLength=a_nwk_service.args.GTSRequest.requestLength;
      nwkState = NWK_STATE_IDLE;
      macDoService();
      break;
    case LRWPAN_SVC_NWK_DISC_NETWORK:
      nwkDiscoveryNetwork();
      nwkState = NWK_STATE_IDLE;
      break;
#ifdef LRWPAN_COORDINATOR
    case LRWPAN_SVC_NWK_FORM_NETWORK:
      //只有协调器才有这样的功能，否则返回"LRWPAN_STATUS_INVALID_REQ"，break
      if (!mac_pib.flags.bits.macPanCoordinator) {
        a_nwk_service.status = LRWPAN_STATUS_INVALID_REQUEST;
        //DEBUG_STRING(DBG_ERR, "Network formed error, it is not a PAN Coordinator!\n");
        nwkState = NWK_STATE_IDLE;
        break;
      }
      a_nwk_service.status = LRWPAN_STATUS_SUCCESS;
      //DEBUG_STRING(DBG_INFO, "Network formed, start to form network!\n");
      nwkFormNetwork();
      nwkState = NWK_STATE_IDLE;
      break;
#else
    case LRWPAN_SVC_NWK_JOIN_NETWORK:
      if (mac_pib.flags.bits.macIsAssociated) {
        a_nwk_service.status = LRWPAN_STATUS_INVALID_REQUEST;
        //DEBUG_STRING(DBG_ERR, "Network joined error, it has been associated!\n");
        nwkState = NWK_STATE_IDLE;
        break;
      }
      //DEBUG_STRING(DBG_INFO,"Network joined, start to join the network!\n");
      nwkJoinNetwork();
      nwkState = NWK_STATE_IDLE;	
      break;
		
#endif
    case LRWPAN_SVC_NWK_LEAVE_NETWORK:
      if (!mac_pib.flags.bits.macIsAssociated) {
        a_nwk_service.status = LRWPAN_STATUS_INVALID_REQUEST;
        //DEBUG_STRING(DBG_ERR, "Network leaved error, it is not associated!\n");
        nwkState = NWK_STATE_IDLE;
        break;
      }
      //DEBUG_STRING(DBG_INFO, "Network leaved, start to leave the network!\n");
      nwkLeaveNetwork();
      nwkState = NWK_STATE_IDLE;
      break;
    default: break;
    }
  default:  break;
  }
}

//Add the NWK header, then send it to MAC
//if fwdFlag is true, then packet is being forwarded, so nwk header
//is already in place, and assume that currentTxFrm and currentTxPLen
//are correct as well, and that the radius byte has been decremented.
/*函数功能:网络层发送数据，若设备未加入网络或广播半径为0，出错，交于MAC处理*/
void nwkTxData(BOOL fwdFlag) {
  //if we are not associated, don't bother sending NWK packet
  if (!mac_pib.flags.bits.macIsAssociated) {
    //call a dummy service that just returns an error code
    //have to do it this way since the caller is expecting a mac service call
    a_mac_service.args.error.status = LRWPAN_STATUS_MAC_NOT_ASSOCIATED;
    a_mac_service.cmd = LRWPAN_SVC_MAC_ERROR;
    goto nwkTxData_sendit;
  }
  if (a_nwk_tx_data.radius == 0) {//广播半径为0，出错
    //DEBUG_STRING(DBG_ERR,"Nwk Radius is zero, discarding packet.\n");
    //can no longer forward this packet.
    a_mac_service.args.error.status =  LRWPAN_STATUS_NWK_RADIUS_EXCEEDED;
    a_mac_service.cmd = LRWPAN_SVC_MAC_ERROR;
    goto nwkTxData_sendit;
  }
  if (fwdFlag)
    goto nwkTxData_addmac;//fwdFlag为1，表示要转发该报文
  //sequence number
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = nwkDSN;
  nwkDSN++;
  //radius, decrement before sending, receiver will get a value that is one less than this node.
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (--a_nwk_tx_data.radius);
  //src address
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (a_nwk_tx_data.srcSADDR >> 8);
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (a_nwk_tx_data.srcSADDR);
  //dst address
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (a_nwk_tx_data.dstSADDR >> 8);
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = (BYTE) (a_nwk_tx_data.dstSADDR);

  //frame control
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = a_nwk_tx_data.fcfmsb;
  --phy_pib.currentTxFrm;
  *phy_pib.currentTxFrm = a_nwk_tx_data.fcflsb;
  //network header is fixed size
  phy_pib.currentTxFlen +=  8;

nwkTxData_addmac:
  //为了方便，我们这里修改了一点，以后深入了解
  /*
  //fill in the MAC fields. For now, we don't support inter-PAN,so the PANID has to be our mac PANID	
  //不支持跨网段传送，目的和源网段ID都是自己的网段ID
  a_mac_tx_data.DestPANID = mac_pib.macPANID;
  a_mac_tx_data.SrcPANID = mac_pib.macPANID;
  if (a_nwk_tx_data.dstSADDR == LRWPAN_SADDR_USE_LADDR ){//如果发送的目的地址为0xFFFE
    //long address is specified from above.  We assume they know where they are going no routing necessary
    //a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_LADDR;
    //把fcfmsb字节置为11001100，目的地址和源地址均为长地址
    //copy in the long address
    //fill it in.以后更改
    if (macGetShortAddr()==LRWPAN_SADDR_USE_LADDR)
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_LADDR;
    else
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_SADDR;
    halUtilMemCopy(&a_mac_tx_data.DestAddr.laddr.bytes[0], a_nwk_tx_data.dstLADDR, 8);
  } else {//如果发送的目的地址不为0xFFFE
    //lets do some routing需要路由
#ifdef LRWPAN_RFD	//如果是精简功能设备则执行以下的部分
    //RFD's are easy. Always send to parent, our SRC address is always long
    //so that parent can confirm that the RFD is still in their neighbor table
    //will use the parent short address
    //fcfmsb字节置为11001000，目的地址为短地址，源地址为长地址
    //a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
    //fill it in.以后更改
    if (macGetShortAddr()==LRWPAN_SADDR_USE_LADDR)
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
    else
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_SADDR;
    //16位目的地址是协调器的短地址
    a_mac_tx_data.DestAddr.saddr = mac_pib.macCoordShortAddress;
#else		//如果是全功能设备
    {
      SADDR newDstSADDR;
      //this is router. need to determine the new dstSADDR
      newDstSADDR = a_nwk_tx_data.dstSADDR; //default
      DEBUG_STRING(DBG_INFO,"Routing pkt to: ");
      DEBUG_UINT16(DBG_INFO,newDstSADDR);
      if (a_nwk_tx_data.dstSADDR != LRWPAN_BCAST_SADDR) {
        //not broadcast address
        newDstSADDR = ntFindNewDst(a_nwk_tx_data.dstSADDR);
        DEBUG_STRING(DBG_INFO," through: ");
        DEBUG_UINT16(DBG_INFO,newDstSADDR);
        if (newDstSADDR == LRWPAN_BCAST_SADDR) {
          DEBUG_STRING(DBG_INFO,", UNROUTABLE, error!\n ");
          //error indicator. An unroutable packet from here.
          a_mac_service.args.error.status = LRWPAN_STATUS_NWK_PACKET_UNROUTABLE;
          a_mac_service.cmd = LRWPAN_SVC_MAC_ERROR;
          goto nwkTxData_sendit;
        }
        DEBUG_STRING(DBG_INFO,"\n");
      }
      //fill it in.以后更改
      if (macGetShortAddr()==LRWPAN_SADDR_USE_LADDR)
        a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
      else
        a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_SADDR;
      a_mac_tx_data.DestAddr.saddr = newDstSADDR;//目的地址
    }
#endif
  }
  //for data frames, we want a MAC level ACK, unless it is a broadcast.
  //除了广播以外的其他的数据帧我们都希望能得到一个mac层的确认帧
  if ( ((LRWPAN_GET_DST_ADDR(a_mac_tx_data.fcfmsb)) == LRWPAN_ADDRMODE_SADDR) &&
      a_mac_tx_data.DestAddr.saddr == LRWPAN_BCAST_SADDR) {
        //no MAC ACK
        a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_DATA|LRWPAN_FCF_INTRAPAN_MASK;
      }else {
        a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_DATA|LRWPAN_FCF_INTRAPAN_MASK |LRWPAN_FCF_ACKREQ_MASK;
      }
  //send it.
  a_mac_service.cmd = LRWPAN_SVC_MAC_GENERIC_TX;
  */
  ////////////////////////////////////////////////////////////////////////////////////以下为新加的
  a_mac_tx_data.DestPANID = mac_pib.macPANID;
  a_mac_tx_data.SrcPANID = mac_pib.macPANID;
  if (a_nwk_tx_data.dstSADDR == LRWPAN_SADDR_USE_LADDR ){
    if (macGetShortAddr()==LRWPAN_SADDR_USE_LADDR) {
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_LADDR;
    } else {
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_LADDR|LRWPAN_FCF_SRCMODE_SADDR;
    }
    halUtilMemCopy(&a_mac_tx_data.DestAddr.laddr.bytes[0], a_nwk_tx_data.dstLADDR, 8);
  } else {
    if (macGetShortAddr()==LRWPAN_SADDR_USE_LADDR)
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_LADDR;
    else
      a_mac_tx_data.fcfmsb = LRWPAN_FCF_DSTMODE_SADDR|LRWPAN_FCF_SRCMODE_SADDR;
   // a_mac_tx_data.DestAddr = mac_pib.macParentShortAddress;//delete by weimin,for the mac_destaddress should equel to nwk_tx_desaddress
 //   a_mac_tx_data.DestAddr.laddr = a_nwk_tx_data.dstLADDR;
 a_mac_tx_data.DestAddr.saddr=a_nwk_tx_data.dstSADDR; //add by weimin
/*//mini
 #ifdef LRWPAN_ROUTER
 a_mac_tx_data.DestAddr.saddr=a_nwk_tx_data.dstSADDR;
 #else
  a_mac_tx_data.DestAddr.saddr=0x0001;
 #endif
//mini*/

  }
  a_mac_tx_data.SrcAddr = macGetShortAddr();
  if ( ((LRWPAN_GET_DST_ADDR(a_mac_tx_data.fcfmsb)) == LRWPAN_ADDRMODE_SADDR) &&
      a_mac_tx_data.DestAddr.saddr == LRWPAN_BCAST_SADDR) {
        a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_DATA|LRWPAN_FCF_INTRAPAN_MASK;
      }else {
        a_mac_tx_data.fcflsb = LRWPAN_FRAME_TYPE_DATA|LRWPAN_FCF_INTRAPAN_MASK |LRWPAN_FCF_ACKREQ_MASK;
      }
  a_mac_service.cmd = LRWPAN_SVC_MAC_GENERIC_TX;
  ////////////////////////////////////////////////////////////////////////////////////以上为新加的
nwkTxData_sendit:
  macDoService();//调用mac层的服务
}


/*函数功能:网络层接收状态机函数，若nwkRxState为NWK_RXSTATE_IDLE，直接返回；
            若nwkRxState为NWK_RXSTATE_START，根据帧类型，进行处理，将命令帧丢弃，
            将数据帧交给上层或进行转发；若nwkRxState为NWK_RXSTATE_APS_HANDOFF，传递相关信息；
            若nwkRxState为NWK_RXSTATE_DOROUTE，若广播半径为0或转发缓存区没有空间，丢弃数据帧，否则进行转发*/
static void nwkRxFSM(void) {
  BYTE *ptr;	
nwkRxFSM_start://接收状态的起始位
  switch(nwkRxState) {
  case NWK_RXSTATE_IDLE:
    break;
  case NWK_RXSTATE_START://we have a packet, lets check it out.
    ptr = a_nwk_rx_data.orgpkt.data + a_nwk_rx_data.nwkOffset;
    if (NWK_IS_CMD(*ptr)) {//currently don't handle CMD packets. Discard.网络层命令帧和路由有关
      //DEBUG_STRING(DBG_INFO,"NWK: Received NWK CMD packet, discarding.\n");
      //MAC resource already free; need to free the MEM resource
      MemFree(a_nwk_rx_data.orgpkt.data);
      nwkRxState = NWK_RXSTATE_IDLE;
      break;
    }
    //this is a data packet. do more parsing.
    nwkParseHdr(ptr);	//解析出网络层的头部
    //see if this is for us.
    if ((a_nwk_rx_data.dstSADDR == LRWPAN_BCAST_SADDR) ||//如果目的地址是广播地址
        (a_nwk_rx_data.dstSADDR == LRWPAN_SADDR_USE_LADDR) ||//或者目的地址是长地址
          (a_nwk_rx_data.dstSADDR == macGetShortAddr())) {//或者目的地址(短地址)匹配
            //hand this off to the APS layer
            nwkRxState = NWK_RXSTATE_APS_HANDOFF;
          } else {//have to route this packet
            nwkRxState = NWK_RXSTATE_DOROUTE;
          }
    goto nwkRxFSM_start;//跳转到接收状态的起始位
  case NWK_RXSTATE_APS_HANDOFF:
    if (apsRxBusy()) break;    //apsRX is still busy
    //handoff the current packet
    apsRxHandoff();
    //we are finished with this packet.
    //we don't need to do anything to free this resource other
    // than to change state
    nwkRxState = NWK_RXSTATE_IDLE;
    break;
  case NWK_RXSTATE_DOROUTE:
#ifdef LRWPAN_RFD//如果是精简功能设备，没有路由功能
    //RFD somehow got a data packet not intended for it.
    //should never happen, but put code here anyway to discard it.
    //DEBUG_STRING(DBG_INFO,"NWK: RFD received spurious datapacket, discarding.\n");
    MemFree(a_nwk_rx_data.orgpkt.data);
    nwkRxState = NWK_RXSTATE_IDLE;
#else
    //first, check the radius, if zero, then discard.
    if (!(*(ptr+6))) {
      //DEBUG_STRING(DBG_INFO,"NWK: Data packet is out of hops for dest: ");
      //DEBUG_UINT16(DBG_INFO,a_nwk_rx_data.dstSADDR);
      //DEBUG_STRING(DBG_INFO,", discarding...\n");
      MemFree(a_nwk_rx_data.orgpkt.data);
      nwkRxState = NWK_RXSTATE_IDLE;
      break;
    }
    //DEBUG_STRING(DBG_INFO,"NWK: Routing NWK Packet to: ");
    //DEBUG_UINT16(DBG_INFO,a_nwk_rx_data.dstSADDR);
    //DEBUG_STRING(DBG_INFO,"\n");
    //this packet requires routing, not destined for us.
    if (nwkRxBuffFull()) {//如果接收缓冲区满
      //no more room. discard this packet
      //DEBUG_STRING(DBG_INFO,"NWK: FWD buffer full, discarding pkt.\n");
      //DEBUG_STRING(DBG_INFO,"NWK state: ");
      //DEBUG_UINT8(DBG_INFO,nwkState);
      //DEBUG_STRING(DBG_INFO,"MAC state: ");
      //DEBUG_UINT8(DBG_INFO,macState);
      //DEBUG_STRING(DBG_INFO,"\n");
      MemFree(a_nwk_rx_data.orgpkt.data);
      nwkRxState = NWK_RXSTATE_IDLE;
    } else {//ok, add this pkt to the buffer
      nwk_pib.rxHead++;
      if (nwk_pib.rxHead == NWK_RXBUFF_SIZE) nwk_pib.rxHead = 0;
      //save it.
      nwk_pib.rxBuff[nwk_pib.rxHead].data = a_nwk_rx_data.orgpkt.data;
      nwk_pib.rxBuff[nwk_pib.rxHead].nwkOffset = a_nwk_rx_data.nwkOffset;
      nwkRxState = NWK_RXSTATE_IDLE;
      //this packet will be retransmitted by nwkFSM
    }
#endif
    break;
  default:
    break;
  }
}



//Callback from MAC Layer
//Returns TRUE if nwk is still busy with last RX packet.
/*函数功能:判断网络层接收状态，空闲状态返回FALSE，非空闲状态返回TRUE*/
BOOL nwkRxBusy(void){
	return(nwkRxState != NWK_RXSTATE_IDLE);	
}

//Callback from MAC Layer从mac层进行回调
//Hands off parsed packet from MAC layer, frees MAC for parsing
//next packet.该报文从mac层传送到网络层解析之，并释放mac层以处理下条报文
/*函数功能:网络层从MAC层接收数据，主要是一些信息的传递*/
void nwkRxHandoff(void){
	a_nwk_rx_data.orgpkt.data = a_mac_rx_data.orgpkt->data;	//把该报文从mac层传送到网络层
	a_nwk_rx_data.orgpkt.rssi = a_mac_rx_data.orgpkt->rssi;//信号强度指示字节
	a_nwk_rx_data.nwkOffset = a_mac_rx_data.pload_offset;	//偏移量
	nwkRxState = NWK_RXSTATE_START;	//网络接收机状态为开始
}

/*函数功能:对网络层接收数据包的头部进行解析，包括目的地址和源地址*/
void nwkParseHdr(BYTE *ptr) {
	//ptr is pointing at nwk header. Get the SRC/DST nodes.
	ptr= ptr+2;
	//get Dst SADDR
	a_nwk_rx_data.dstSADDR = *ptr;
	ptr++;
	a_nwk_rx_data.dstSADDR += (((UINT16)*ptr) << 8);
	ptr++;

	//get Src SADDR
	a_nwk_rx_data.srcSADDR = *ptr;
	ptr++;
	a_nwk_rx_data.srcSADDR += (((UINT16)*ptr) << 8);
	ptr++;
}

#ifdef LRWPAN_FFD

//copies packet to forward from heap space to TXbuffer space
/*函数功能:拷贝转发数据，从内存到发送缓存器，包括网络层帧报头和网络层的有效载荷，
			同时将目的地址和广播半径写到a_nwk_tx_data结构体*/
void nwkCopyFwdPkt(void){
	BYTE *srcptr, len;
	NWK_FWD_PKT *pkt;

	phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
	//get next PKT
	pkt = nwkGetRxPacket();//应该检查是否为NULL，其实在非空的情况下调用的
	srcptr = pkt->data;  //points at original packet in heapspace
	//compute bytes to copy.计算要复制的字节数
	//nwkoffset is the offset of the nwkheader in the original packet
	len = *(srcptr) - pkt->nwkOffset - PACKET_FOOTER_SIZE + 1;
	//point this one byte past the end of the packet
	srcptr = srcptr		//把该指针移到报文的尾部
		+ *(srcptr) //length of original packet, not including this byte
		+ 1         //add one for first byte which contains packet length
		- PACKET_FOOTER_SIZE; //subtract footer bytes, don't want to copy these.
	//save length
	phy_pib.currentTxFlen = len;//当前发送报文的长度
	//copy from heap space to TXBuffer space
	do {
		srcptr--; phy_pib.currentTxFrm--;
		*phy_pib.currentTxFrm = *srcptr;
		len--;
	}while(len);
	nwkFreeRxPacket(TRUE);  //free this packet
	//some final steps
	//get the dstSADDR, needed for routing.
	a_nwk_tx_data.dstSADDR = *(phy_pib.currentTxFrm+2);
	a_nwk_tx_data.dstSADDR += (((UINT16)*(phy_pib.currentTxFrm+3)) << 8);
	//decrement the radius before sending it on.
	*(phy_pib.currentTxFrm+6)= *(phy_pib.currentTxFrm+6)- 1;
	a_nwk_tx_data.radius = *(phy_pib.currentTxFrm+6);

	
	//leave the SADDR unchanged as we want to know where this originated from!
#if 0
	//replace the SADDR with our SADDR
	*(phy_pib.currentTxFrm+4) = (BYTE) macGetShortAddr();
	*(phy_pib.currentTxFrm+5) = (BYTE) (macGetShortAddr() >>8);
#endif

}

/*函数功能:判断网络层接收缓存器是否为满，缓存器满返回true,缓存器没满返回false*/
static BOOL nwkRxBuffFull(void){
	BYTE tmp;
	//if next write would go to where Tail is, then buffer is full
	tmp = nwk_pib.rxHead+1;
	if (tmp == NWK_RXBUFF_SIZE) tmp = 0;
	return(tmp == nwk_pib.rxTail);
}

/*函数功能:判断网络层接收缓存器是否为空，缓存器空返回true,缓存器非空返回false*/
static BOOL nwkRxBuffEmpty(void){
	return(nwk_pib.rxTail == nwk_pib.rxHead);
}

//this does NOT remove the packet from the buffer
/*函数功能:从网络层接收缓存器获取一个指向转发数据包的指针，若缓存器为空，返回NULL*/
static NWK_FWD_PKT *nwkGetRxPacket(void) {
	BYTE tmp;
	if (nwk_pib.rxTail == nwk_pib.rxHead) return(NULL);
	tmp = nwk_pib.rxTail+1;
	if (tmp == NWK_RXBUFF_SIZE) tmp = 0;
	return(&nwk_pib.rxBuff[tmp]);
}

//frees the first packet in the buffer.释放接收缓冲区
/*函数功能:释放网络层接收缓存器第一个指针指向的转发数据包内存*/
static void nwkFreeRxPacket(BOOL freemem) {
	nwk_pib.rxTail++;
	if (nwk_pib.rxTail == NWK_RXBUFF_SIZE) nwk_pib.rxTail = 0;
	if (freemem) MemFree(nwk_pib.rxBuff[nwk_pib.rxTail].data);
}

#endif

/*函数功能:发现网络功能，首先执行主动扫描或被动扫描，完成后更新邻居表，
	将扫描返回的信息组合成网络描述符*/
void nwkDiscoveryNetwork(void)
{
  BYTE i,j,k;
  NAYBORENTRY *nt_ptr;

//#ifdef LRWPAN_FFD
  //主动扫描
  a_mac_service.cmd = LRWPAN_SVC_MAC_SCAN_REQ;
  a_mac_service.args.scan.request.ScanType = ACTIVE;
  a_mac_service.args.scan.request.ScanChannels = a_nwk_service.args.discovery_network.request.ScanChannels;
  a_mac_service.args.scan.request.ScanDuration = a_nwk_service.args.discovery_network.request.ScanDuration;
  macDoService();	
//#endif
  /*
#ifdef LRWPAN_RFD
  //被动扫描
  a_mac_service.cmd = LRWPAN_SVC_MAC_SCAN_REQ;
  a_mac_service.args.scan.request.ScanType = PASSIVE;
  a_mac_service.args.scan.request.ScanChannels = a_nwk_service.args.discovery_network.request.ScanChannels;
  a_mac_service.args.scan.request.ScanDuration = a_nwk_service.args.discovery_network.request.ScanDuration;
  macDoService();	
#endif
  */
  if (a_mac_service.status==LRWPAN_STATUS_MAC_NO_BEACON) {
    //DEBUG_STRING(DBG_ERR, "Discovery network failed, active/passive scan is unsuccessful.\n");
  } else if (a_mac_service.status==LRWPAN_STATUS_SUCCESS) {
    nwkInitNetworkDescriptorList();
    mac_pib.macScanNodeCount = a_mac_service.args.scan.confirm.ResultListSize;
    k = 0;
    for (i=0;i<mac_pib.macScanNodeCount;i++) {
      //执行主动/被动扫描以后，更新该设备的邻居表，并且将返回的扫描信息组合成网络描述符
      //在树型网络中，邻居设备应该是父设备或子设备，在网络连接中进行更新
      //在Mesh网络中，邻居设备应该是PoS内的任何一个可以通信的设备，在发现网络和网络连接等情况下更新邻居表
      nt_ptr = &mac_nbr_tbl[0];
      for (j=0;j<NTENTRIES;j++,nt_ptr++) {

        if (!nt_ptr->options.bits.used) {
          nt_ptr->options.bits.used = 1;
          nt_ptr->saddr = PANDescriptorList[i].ShortAddress;
          nt_ptr->laddr[0] = PANDescriptorList[i].ExtendedAddress.bytes[0];
          nt_ptr->laddr[1] = PANDescriptorList[i].ExtendedAddress.bytes[1];
          nt_ptr->laddr[2] = PANDescriptorList[i].ExtendedAddress.bytes[2];
          nt_ptr->laddr[3] = PANDescriptorList[i].ExtendedAddress.bytes[3];
          nt_ptr->laddr[4] = PANDescriptorList[i].ExtendedAddress.bytes[4];
          nt_ptr->laddr[5] = PANDescriptorList[i].ExtendedAddress.bytes[5];
          nt_ptr->laddr[6] = PANDescriptorList[i].ExtendedAddress.bytes[6];
          nt_ptr->laddr[7] = PANDescriptorList[i].ExtendedAddress.bytes[7];
          nt_ptr->capinfo = 0x00;
          //LRWPAN_SET_CAPINFO_ALTPAN(nt_ptr->capinfo);
          LRWPAN_SET_CAPINFO_DEVTYPE(nt_ptr->capinfo);
          //LRWPAN_SET_CAPINFO_PWRSRC(nt_ptr->capinfo);
          //LRWPAN_SET_CAPINFO_RONIDLE(nt_ptr->capinfo);
          if (PANDescriptorList[i].options.bits.SecurityUse)
            LRWPAN_SET_CAPINFO_SECURITY(nt_ptr->capinfo);
          //LRWPAN_SET_CAPINFO_ALLOCADDR(nt_ptr->capinfo);
          nt_ptr->PANID = PANDescriptorList[i].CoordPANId;
          if (PANDescriptorList[i].options.bits.PANCoordinator)
            nt_ptr->DeviceType = LRWPAN_DEVTYPE_COORDINATOR;
          else
            nt_ptr->DeviceType = LRWPAN_DEVTYPE_ROUTER;
          nt_ptr->Relationship = LRWPAN_DEVRELATION_SIBING;
          nt_ptr->Depth = PANDescriptorList[i].Depth;
          nt_ptr->BeaconOrder = ((BYTE)PANDescriptorList[i].SuperframeSpec)&0x0F;
          nt_ptr->LQI = PANDescriptorList[i].LinkQuality;
          nt_ptr->LogicalChannel = PANDescriptorList[i].LogicalChannel;
          if (PANDescriptorList[i].options.bits.AssociationPermit)
            nt_ptr->options.bits.PermitJoining = 1;
          break;
        }
      }

      if (j== NTENTRIES) {
        //DEBUG_STRING(DBG_ERR, "There is no room in neighbor table!\n");
      }

      if (!PANDescriptorList[i].options.bits.PANCoordinator)
        continue;

      NetworkDescriptorList[k].PANId = PANDescriptorList[i].CoordPANId;
      NetworkDescriptorList[k].CoordShortAddress = PANDescriptorList[i].ShortAddress;
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[0] = PANDescriptorList[i].ExtendedAddress.bytes[0];
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[1] = PANDescriptorList[i].ExtendedAddress.bytes[1];
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[2] = PANDescriptorList[i].ExtendedAddress.bytes[2];
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[3] = PANDescriptorList[i].ExtendedAddress.bytes[3];
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[4] = PANDescriptorList[i].ExtendedAddress.bytes[4];
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[5] = PANDescriptorList[i].ExtendedAddress.bytes[5];
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[6] = PANDescriptorList[i].ExtendedAddress.bytes[6];
      NetworkDescriptorList[k].CoordExtendedAddress.bytes[7] = PANDescriptorList[i].ExtendedAddress.bytes[7];
      NetworkDescriptorList[k].LogicalChannel = PANDescriptorList[i].LogicalChannel;
      NetworkDescriptorList[k].LinkQuality = PANDescriptorList[i].LinkQuality;
      NetworkDescriptorList[k].StackProfile = PANDescriptorList[i].StackProfile;
      NetworkDescriptorList[k].ZigBeeVersion = PANDescriptorList[i].ZigBeeVersion;
      NetworkDescriptorList[k].BeaconOrder = (BYTE)(PANDescriptorList[i].SuperframeSpec&0x000F);
      NetworkDescriptorList[k].SuperframeOrder = (BYTE)((PANDescriptorList[i].SuperframeSpec>>4)&0x000F);
      NetworkDescriptorList[k].PermitJioning = (BOOL)(PANDescriptorList[i].SuperframeSpec>>15);
      NetworkDescriptorList[k].SecurityLevel = 0x00;//目前不采用安全，待改进
      k++;
    }
    nwk_pib.nwkNetworkCount = k;
    a_nwk_service.args.discovery_network.confirm.NetworkCount = k;
  }

  a_nwk_service.status = a_mac_service.status;
}


#ifndef LRWPAN_COORDINATOR
/*函数功能:加入网络*/
void nwkJoinNetwork(void)
{
  BYTE i,lqi;
  NAYBORENTRY *nt_ptr;

  if (a_nwk_service.args.join_network.options.bits.RejoinNetwork) {//重新加入网络
    /*网络层管理实体发送MLME_SCAN.request原语，孤点扫描。接收到响应后，在MAC层更新设置*/
    a_mac_service.cmd = LRWPAN_SVC_MAC_SCAN_REQ;
    a_mac_service.args.scan.request.ScanType = ORPHAN;
    a_mac_service.args.scan.request.ScanChannels = a_nwk_service.args.join_network.ScanChannels;
    a_mac_service.args.scan.request.ScanDuration = a_nwk_service.args.join_network.ScanDuration;
    macDoService();
    a_nwk_service.status = a_mac_service.status;
  } else {
    phyInit();
    macInit();
    /*网络层管理实体发送MLME_ASSOCIATE.request原语，相关信息从邻居表中获取。
    如果在邻居表中不存在符合条件的设备，则网络层返回状态NOT_PERMITTED
    */
    mac_pib.flags.bits.macAssociationPermit = 0;//if trying to join, do not allow association
    mac_pib.flags.bits.macIsAssociated = 0;
    nt_ptr = NULL;
    lqi = 0;
    a_mac_service.cmd = LRWPAN_SVC_MAC_ASSOC_REQ;
    //以下加入网络，是应用子层确定哪个网络，网络层确定加入哪个节点
    for (i=0;i<NTENTRIES;i++) {
      if (!mac_nbr_tbl[i].options.bits.used)
        continue;
      if (mac_nbr_tbl[i].PANID==a_nwk_service.args.join_network.PANID && mac_nbr_tbl[i].saddr!=LRWPAN_BCAST_SADDR) {
        if (mac_nbr_tbl[i].LQI > lqi) {
          lqi = mac_nbr_tbl[i].LQI;
          nt_ptr = &mac_nbr_tbl[i];
        }
      }
    }
    if (nt_ptr==NULL) {
      //DEBUG_STRING(DBG_ERR, "Network join failed, have no a right parent device!\n");
      nwkState = NWK_STATE_IDLE;
      a_nwk_service.status = LRWPAN_STATUS_NWK_JOIN_NOT_PERMITTED;
      return;
    }
    a_mac_service.args.associate.request.LogicalChannel = nt_ptr->LogicalChannel;
    a_mac_service.args.associate.request.CoordAddrMode = LRWPAN_ADDRMODE_SADDR;
    a_mac_service.args.associate.request.CoordPANID = a_nwk_service.args.join_network.PANID;
    a_mac_service.args.associate.request.CoordAddress.saddr = nt_ptr->saddr;
    a_mac_service.args.associate.request.CapabilityInformation = LRWPAN_ASSOC_CAPINFO_ALLOCADDR;//分配短地址
    if (a_nwk_service.args.join_network.options.bits.JoinAsRouter) {//作为路由器和备用协调器
      a_mac_service.args.associate.request.CapabilityInformation |= LRWPAN_ASSOC_CAPINFO_ALTPAN;
      a_mac_service.args.associate.request.CapabilityInformation |= LRWPAN_ASSOC_CAPINFO_DEVTYPE;
    }
    if (a_nwk_service.args.join_network.options.bits.PowerSource)//交流电源
      a_mac_service.args.associate.request.CapabilityInformation |= LRWPAN_ASSOC_CAPINFO_PWRSRC;
    if (a_nwk_service.args.join_network.options.bits.RxOnWhenIdle)//空闲时接收机打开
      a_mac_service.args.associate.request.CapabilityInformation |= LRWPAN_ASSOC_CAPINFO_RONIDLE;
    if (a_nwk_service.args.join_network.options.bits.MACSecurity) {//使用安全
      a_mac_service.args.associate.request.CapabilityInformation |= LRWPAN_ASSOC_CAPINFO_SECURITY;
      a_mac_service.args.associate.request.SecurityEnable = 0;
    }

    //a_mac_service.args.associate.request.CapabilityInformation = 0x01;
    a_nwk_service.status = macInitRadio(a_mac_service.args.associate.request.LogicalChannel, a_mac_service.args.associate.request.CoordPANID);
    if (a_nwk_service.status != LRWPAN_STATUS_SUCCESS) {//初始化信道失败
      //DEBUG_STRING(DBG_ERR, "Network join failed, phy radio initializtion is unsuccessful!\n");
      nwkState = NWK_STATE_IDLE;
      a_nwk_service.status = LRWPAN_STATUS_PHY_RADIO_INIT_FAILED;
      return;
    }

    macDoService();
    a_nwk_service.status = a_mac_service.status;
    if (a_nwk_service.status == LRWPAN_STATUS_SUCCESS) {
      mac_pib.flags.bits.macAssociationPermit = 1;//if trying to join, do not allow association
      mac_pib.flags.bits.macIsAssociated = 1;
      nt_ptr->Relationship = LRWPAN_DEVRELATION_PARENT;
      mac_pib.macDepth = nt_ptr->Depth + 1;
      mac_pib.macParentShortAddress = nt_ptr->saddr;
      halUtilMemCopy(&mac_pib.macParentExtendedAddress.bytes[0], &nt_ptr->laddr[0], 8);
#ifdef LRWPAN_FFD
      //初始化邻居设备数目和短地址分配
      ntInitAddressAssignment();
#endif
      //DEBUG_STRING(DBG_INFO,"Network join successed, the AssocShortAddress is ");
      //DEBUG_UINT16(DBG_INFO,a_mac_service.args.associate.confirm.AssocShortAddress);
      //DEBUG_STRING(DBG_INFO,"  \n ");
    } else {
      //DEBUG_STRING(DBG_INFO,"Network join failed, the coordinator denies this associate request!\n");
    }
    nwkState = NWK_STATE_IDLE;
    /*返回 连接请求命令状态，得到 分配的短地址。在接收到连接响应后，在MAC层更新设置*/
  }
}
#endif

void nwkLeaveNetwork(void)
{
//FFD设备断开网络有两种形式：1.将某设备（由参数DeviceAddress指定）同网络断开。2.将自身（参数DeviceAddress为NULL）同网络断开
#ifdef LRWPAN_FFD
  if (nwkCheckLaddrNull(&a_nwk_service.args.leave_network.DeviceAddress)){
    a_mac_service.args.disassoc_req.DisassociateReason = DEVICE_DISASSOC_WITH_NETWORK;
    nwkSetLaddrNull(&a_mac_service.args.disassoc_req.DeviceAddress.bytes[0]);
    a_mac_service.args.disassoc_req.SecurityEnable = FALSE;
    a_mac_service.cmd = LRWPAN_SVC_MAC_DISASSOC_REQ;
    macDoService();
    a_nwk_service.status = a_mac_service.status;

    if (a_nwk_service.status == LRWPAN_STATUS_SUCCESS) {
      //DEBUG_STRING(DBG_INFO, "Network leaved successed, I am disassociated!");
    } else {
      //DEBUG_STRING(DBG_INFO, "Network leaved failed, I am not disassociated!");
      return;
    }
    //清除路由表,邻居表，和地址表
    ntInitTable();
    mac_pib.flags.bits.macIsAssociated = 0;
    //MAC层复位
    a_mac_service.cmd = LRWPAN_SVC_MAC_RESET_REQ;
    a_mac_service.args.reset.SetDefaultPIB = TRUE;
    macDoService();
    a_nwk_service.status = a_mac_service.status;		
  } else {
    if ((NAYBORENTRY *)ntFindByLADDR( &a_nwk_service.args.leave_network.DeviceAddress) == NULL){//在邻居表中找不到该设备
      a_nwk_service.status = LRWPAN_STATUS_UNKNOWN_DEVICE;
      //DEBUG_STRING(DBG_ERR, "Force a device to leave network, the device is not existent in neighbortable!\n");
      return;
    }	
    a_mac_service.args.disassoc_req.DisassociateReason = FORCE_DEVICE_OUTOF_NETWORK;
    halUtilMemCopy(&a_mac_service.args.disassoc_req.DeviceAddress.bytes[0], &a_nwk_service.args.leave_network.DeviceAddress.bytes[0],8);
    a_mac_service.args.disassoc_req.SecurityEnable = FALSE;
    a_mac_service.cmd = LRWPAN_SVC_MAC_DISASSOC_REQ;
    ntDelNbrByLADDR(&a_nwk_service.args.leave_network.DeviceAddress);
    macDoService();
    if (a_nwk_service.status == LRWPAN_STATUS_SUCCESS) {
      //DEBUG_STRING(DBG_INFO, "Network leaved successed, this device is disassociated!");
    } else {
      //DEBUG_STRING(DBG_INFO, "Network leaved failed, this device is not disassociated!");
      return;
    }
    a_nwk_service.status = a_mac_service.status;			
  }

#endif
//RFD设备只能将自身同网络断开，其参数DeviceAddress必须为NULL
#ifdef LRWPAN_RFD
  if ( !(nwkCheckLaddrNull(&a_nwk_service.args.leave_network.DeviceAddress))){
    a_nwk_service.status = LRWPAN_STATUS_INVALID_PARAMETER;
    //DEBUG_STRING(DBG_ERR, "RFD leaved the network,the parameter of DeviceAddress is invalid!\n");
    return;
  }
  a_mac_service.args.disassoc_req.DisassociateReason = DEVICE_DISASSOC_WITH_NETWORK;
  nwkSetLaddrNull(&a_mac_service.args.disassoc_req.DeviceAddress.bytes[0]);
  a_mac_service.args.disassoc_req.SecurityEnable = FALSE;
  macDoService();
  a_nwk_service.status = a_mac_service.status;

  if (a_nwk_service.status == LRWPAN_STATUS_SUCCESS) {
    //DEBUG_STRING(DBG_INFO, "Network leaved successed, I am disassociated!");
  } else {
    //DEBUG_STRING(DBG_INFO, "Network leaved failed, I am not disassociated!");
    return;
  }
  //清楚路由表、地址表信息
  //ntInitAddressMap();//与ntDelAddressByIndex(0)功能相同
  mac_pib.flags.bits.macIsAssociated = 0;
  //MAC层复位
  a_mac_service.cmd = LRWPAN_SVC_MAC_RESET_REQ;
  a_mac_service.args.reset.SetDefaultPIB = TRUE;
  macDoService();
  a_nwk_service.status = a_mac_service.status;
#endif
}

#ifdef LRWPAN_COORDINATOR
/*函数功能:形成网络功能*/
void nwkFormNetwork(void)
{
  BYTE minenergy;
  BYTE channel,i;
  UINT16 panid;
  UINT32 optionalchannel;	

  nwk_pib.flags.bits.nwkFormed = 0;	
  mac_pib.flags.bits.macIsAssociated = 0;
  mac_pib.flags.bits.macAssociationPermit = 0;
  //能量检测扫描
  //DEBUG_STRING(DBG_INFO,"Network formed, start to energy detect.\n");
  a_mac_service.cmd = LRWPAN_SVC_MAC_SCAN_REQ;
  a_mac_service.args.scan.request.ScanType = ENERGY_DETECT;
  a_mac_service.args.scan.request.ScanChannels = a_nwk_service.args.form_network.ScanChannels;
  a_mac_service.args.scan.request.ScanDuration = a_nwk_service.args.form_network.ScanDuration;
  macDoService();
  //查看哪些信道被扫描，进一步对这些信道进行主动扫描
  optionalchannel = a_nwk_service.args.form_network.ScanChannels&(~a_mac_service.args.scan.confirm.UnscanChannels);
  /*
  //主动扫描
  DEBUG_STRING(DBG_INFO,"Network formed, start to active detect.\n");
  a_mac_service.cmd = LRWPAN_SVC_MAC_SCAN_REQ;
  a_mac_service.args.scan.request.ScanType = ACTIVE;
  a_mac_service.args.scan.request.ScanChannels = a_nwk_service.args.form_network.ScanChannels;
  a_mac_service.args.scan.request.ScanDuration = a_nwk_service.args.form_network.ScanDuration;
  macDoService();
  //查看哪些信道被扫描，准备进行选择适当的信道和PANID
  optionalchannel = optionalchannel&a_nwk_service.args.form_network.ScanChannels&(~a_mac_service.args.scan.confirm.UnscanChannels);
  */
  //选择信道
  //DEBUG_STRING(DBG_INFO,"Network formed, start to select a fine logicalchannel.\n");
  minenergy = 0xFF;

  for (i=11;i<27;i++) {
    if (optionalchannel&(((UINT32)1)<<i)) {
      if (EnergyDetectList[i-11]<minenergy) {
        minenergy = EnergyDetectList[i-11];
        channel = i;
      }
    }
    channel = 13;
  }

  if (minenergy==0xFF) {
    a_nwk_service.status = LRWPAN_STATUS_STARTUP_FAILURE;
    //DEBUG_STRING(DBG_ERR, "NWK Formation failed, have not a right logicalchannel!\n");
    nwkState = NWK_STATE_IDLE;
    return;
  }
  //确定PANID
  //DEBUG_STRING(DBG_INFO,"Network formed, start to select PANID.\n");
  if (a_nwk_service.args.form_network.PANID==0x0000) {
    panid = halGetRandomByte();
    panid = panid<<8;
    panid = panid + halGetRandomByte();
    panid = panid&0x3fff;
    a_nwk_service.args.form_network.PANID = panid;
  }  else
    panid = a_nwk_service.args.form_network.PANID;

  //DEBUG_STRING(DBG_INFO,"Network formed, the current logicalchannel : ");
  //DEBUG_UINT8(DBG_INFO,channel);
  //DEBUG_STRING(DBG_INFO," ;  the current PANID : ");
  //DEBUG_UINT16(DBG_INFO,panid);
  //DEBUG_STRING(DBG_INFO," \n");

  phyInit();
  macInit();
  ntInitTable();  //init neighbor table	

  //初始化RF
  a_nwk_service.status = macInitRadio(channel, panid);  //turns on the radio
  if (a_nwk_service.status != LRWPAN_STATUS_SUCCESS) {
    //DEBUG_STRING(DBG_ERR, "Network formed failed, initial RF failure!\n");
    return;
  }
  //设置相关参数PANID，信道，短地址
  macSetPANID(panid);
  macSetChannel(channel);
  macSetShortAddr(0);
  macSetDepth(0);
  //初始化短地址分配参数，必须在协调器的短地址设置完成后进行地址分配
#ifdef LRWPAN_FFD
  ntInitAddressAssignment();	
#endif
  //设置标志位，表示网络已经形成，自己已经连接网络，mac层允许连接
  nwk_pib.flags.bits.nwkFormed = 1;	
  mac_pib.flags.bits.macIsAssociated = 1;
  mac_pib.flags.bits.macAssociationPermit = 1;
  //初始化新的超帧超帧配置
  //DEBUG_STRING(DBG_INFO, "Network formed, start to upgrade superframe!\n");
  a_mac_service.cmd = LRWPAN_SVC_MAC_START_REQ;
  a_mac_service.args.start.PANID = panid;
  a_mac_service.args.start.LogicalChannel = channel;
  a_mac_service.args.start.BeaconOrder = a_nwk_service.args.form_network.BeaconOrder;
  a_mac_service.args.start.SuperframeOrder = a_nwk_service.args.form_network.SuperframeOrder;
  a_mac_service.args.start.options.bits.PANCoordinator = TRUE;
  a_mac_service.args.start.options.bits.BatteryLifeExtension = a_nwk_service.args.form_network.BatteryLifeExtension;
  a_mac_service.args.start.options.bits.CoordRealignment = TRUE;
  a_mac_service.args.start.options.bits.SecurityEnable = FALSE;
  macDoService();
  a_nwk_service.status = a_mac_service.status;
}
#endif
//given a router child SADDR, find the parent router SADDR
/*函数功能:根据网络中一个路由器短地址，获得其父设备的短地址*/
UINT16 nwkFindParentSADDR(SADDR childSADDR) {
	UINT8 currentDepth;	//当前深度
	SADDR currentParent;	//当前父节点
	SADDR currentRouter;	//当前路由器	
	SADDR maxSADDR;		//最大地址
	UINT8 i;


	currentDepth = 1;	//当前深度为1
	currentParent = 0;	//当前父节点为0，协调器
	do {
		for (i=0; i<LRWPAN_MAX_ROUTERS_PER_PARENT; i++) {//参见短地址的分配机制
			if (i==0) currentRouter = currentParent+1;
			else currentRouter += ntGetCskip(currentDepth);
			if (childSADDR == currentRouter) return(currentParent);//更改版
			//if (childSADDR == currentRouter) return(currentRouter);//原版，有点问题
			maxSADDR = ntGetMaxSADDR(currentRouter,currentDepth+1);
			if ((childSADDR > currentRouter) && (childSADDR <= maxSADDR))
				break; //must go further down the tree
		}
		currentDepth++;
		currentParent = currentRouter;
	}
	while (currentDepth < LRWPAN_MAX_DEPTH-1);//<4
	//if we reach here, could not find an address. Return 0 as an error
	return(0);
}

UINT16 nwkGetHopsToDest(SADDR dstSADDR){	//根据目的节点的地址获得去目的节点的跳数
				//该段最好对照着216页的图来看
	UINT16 numHops;
	SADDR currentParent, maxSADDR;
	UINT8 currentDepth;
	UINT8 i;
	SADDR currentRouter;

	numHops = 1;            //return a minimum value of 1

	currentDepth = 0;
	//first compute hops up the tree then down the tree
	if ( macGetShortAddr() == 0) goto nwkGetHopsToDest_down;  //this is the coordinator,the coordinator short address is 0x0000
	if (macGetShortAddr() == dstSADDR) return(1);  //to myself, should not happen, but return min value
	currentParent = mac_pib.macCoordShortAddress; //start with my parent address当前父节点的值为协调器的短地址
	currentDepth = mac_pib.macDepth - 1; //depth of my parent.当前深度为父节点的深度
	do {	
		if (currentParent == dstSADDR) return(numHops);  //destination is one of my parent nodes.目的节点是当前父节点，且父节点不是协调器，则返回跳数
		if (currentParent == 0) break;         //at coordinator.如果当前父节点是协调器，则退出
		//compute the max SADDR address range of parent		//计算父节点范围的最大地址

		maxSADDR = ntGetMaxSADDR(currentParent,currentDepth+1);  //depth of parent's children该算法有点问题，主要是它的对应表有点问题
		if ((dstSADDR > currentParent) &&  (dstSADDR <= maxSADDR)) {
			//this address is in this router's range, stop going up.该地址是与该路由器节点处于同一深度的节点，不用再传到上层
			break;
		}
		//go up a level向上一层
		currentDepth--;//当前深度减1
		numHops++;	//跳数加1	
		if (currentDepth == 0 )
			currentParent =0;//如果当前深度为 0，则当前父节点值为0，协调器
		else { currentParent = nwkFindParentSADDR(currentParent);	//否则，找出当前父节点的父节点的地址(也就是当前节点的爷爷的地址:)
		if (!currentParent) {//如果爷爷节点已经是协调器的话，就已经找到顶了
			//could not find, set numHops to maximum and return找不到，设置跳数为最大值，并返回
			return(LRWPAN_MAX_DEPTH<<1);
		}
		}
	}while(1);

nwkGetHopsToDest_down:	//this is the coordinator
	currentDepth++; //increment depth, as this should reflect my current children当前深度加1，增加深度，为了反应当前子节点
	//now search going down.	对于协调器要向下搜寻
	do {
		//destination is in the current parent's range
		//see if it is one of the routers or children.
		//first see if it is one of the children of current parent
		//目的节点是在当前父节点的范围，看是否是路由器或者终端设备，首先看是否是当前父节点的子节点
		numHops++;	//跳数加1
		maxSADDR = ntGetMaxSADDR(currentParent,currentDepth);	//获得最大地址
		if (dstSADDR > (maxSADDR-LRWPAN_MAX_NON_ROUTER_CHILDREN) &&
			dstSADDR <= maxSADDR) break;  //it is one of the children nodes是其中的一个终端设备
		for (i=0; i<LRWPAN_MAX_ROUTERS_PER_PARENT; i++) {	
			if (i==0) currentRouter = currentParent+1;//子路由器首先肯定是从0协调器开始，再是1，接着是2这些都是嫡系路由系统，其他的都是旁支
			else currentRouter += ntGetCskip(currentDepth);//到这步已经算出了子路由器的地址值

			if (dstSADDR == currentRouter) return(currentRouter);
			maxSADDR = ntGetMaxSADDR(currentRouter,currentDepth+1);//这是算出协调器下面的孙子节点一级的地址范围
			if ((dstSADDR > currentRouter) && (dstSADDR <= maxSADDR))//看是不是在孙节点一级
				break; //must go further down the tree
		}
		if (i == LRWPAN_MAX_ROUTERS_PER_PARENT) {
			//must be one of my non-router children, increment hops, return
			return(numHops);
		}
		currentDepth++;		//当前深度加1，由于在孙节点一级所以深度要再加1
		currentParent = currentRouter;	//当前路由器就是当前父节点

	}while(currentDepth < LRWPAN_MAX_DEPTH-1);

	if (numHops > LRWPAN_NWK_MAX_RADIUS) {
		//DEBUG_STRING(DBG_ERR,"nwkGetHopsToDest: Error in hop calculation: ");
		//DEBUG_UINT8(DBG_ERR,numHops);
		//DEBUG_STRING(DBG_ERR,"\n");
		numHops = LRWPAN_NWK_MAX_RADIUS-1;
	}
	return(numHops);
}








