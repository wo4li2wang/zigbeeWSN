/*
*2007/09/29 WXL 2.0
*/

//-----------------------------------------------------------------------------
// Copyright (C), 2007, CQUPT.													
// Do not use commercially without author's permission
//
// File name:	MIB.C														
// Author:	Wei Min															
// Version:		0.1 																	
// Date:		2007-9-29 														
// Description:	ZigBee MIB. This is a test unit.			
//
// Refer to Standard for Architecture and Digital Data Communication for
//	 ZigBee-based Industrial Control Systems
//-------------------------------------------------------------------



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
#include "evboard.h"
#include "mib.h"
#include "string.h"
#include "user.h"
#ifdef SECURITY_GLOBALS
#include "security.h"
#endif
DevDes   gdev_des;// ZigBee设备描述对象
/*********************************************
此部分内容需要和李老师（应用层）沟通后添加
********************************************/
void DevDes_Init(void) {
	gdev_des.obj_id = MIB_BASE_OBJID_DEVDES;
	gdev_des.res[0] = ZigBee_RESERVED_VALUE;
	gdev_des.res[1] = ZigBee_RESERVED_VALUE;
	memcpy(gdev_des.dev_id, ZigBee_DEFAULT_DEVID1, 32);         // 设备标识
	memcpy(gdev_des.pd_tag, ZigBee_DEFAULT_PDTAG, 32);                          // 设备标识
	gdev_des.act_ShortAddress = mac_pib.macShortAddress;
        gdev_des.act_ExtendedAddress = mac_pib.macExtendedAddress;
	gdev_des.dev_type = ZigBee_DEV_TYPE ;
	gdev_des.status = 1;
	gdev_des.dev_ver = ZigBee_DEV_VER;
	gdev_des.ann_interval = ZjgBee_DEFAULT_ANN_INTERVAL;
	gdev_des.ann_ver = 0;
	gdev_des.dev_r_state = 0;
	gdev_des.dev_r_num = 1;
	gdev_des.lan_r_port = 0;
	gdev_des.max_r_num = 2;
	gdev_des.dup_tag_detected = 0;
	gdev_des.zigbeeID = 0x01;//is it need there? i think it is just for test version by weimin
}

void MIBInit(void) {
	DevDes_Init();
#ifdef SECURITY_GLOBALS
	KeyMan_Init();
	AuthenObj_Init();
       MAC_SECURITY_PIB_Init();
	SecMeasure_Init();
	SecPtclMan_Init();
	AcctrlObj_Init();

#endif
	
}
void MIBRead()
{
UINT16 obj_id;
obj_id=(UINT16)(a_aps_rx_data.usrPload[5]<<8)+a_aps_rx_data.usrPload[6];
switch(obj_id)
	{
	case MIB_BASE_OBJID_MIBHDR:
	//return (MIBHdr_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_DEVDES:
	//return (DevDes_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_CLKSYNC:
	//return (ClkSync_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_MAXRSPTIME:
	//return (MaxRsp_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_COMSCHEDULE:
	//return (ComSch_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_DEVAPP:
	//return (DevApp_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_FBAPPHDR:
	//return (FBAppHdr_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_DOMAINHDR:
	//return (DomainAppHdr_Read(sub_idx, msg_id, src_app, src_port, src_ip));
	case MIB_BASE_OBJID_LINKOBJHDR:
	//return (LinkObjHdr_Read(sub_idx, msg_id, src_app, src_port, src_ip));
        case ZigBee_PIB:
         ZigBeePIB_read();
        break;
#ifdef SECURITY_GLOBALS
		case NSMIB_BASE_OBJID_KEY_MAN:
	KeyMan_Read();
         break;
		case NSMIB_BASE_OBJID_SEC_MEASURE:
	SecMeasure_Read();
         break;
		case NSMIB_BASE_OBJID_PTCL_MAN:
 	SecPtclMan_Read();
                   break;
		case NSMIB_BASE_OBJID_AUTHEN:
	AuthenObj_Read();
         break;
		case NSMIB_BASE_OBJID_MACPIB:
	MAC_SECURITY_PIB_READ();
         break;
#endif	
default:
	 	break;
}

}




/*********
读ZIGPIB
********/
void ZigBeePIB_read(void)
{
 BYTE pload_rsp[50];
 UINT8 Lay_ID;           //读哪一层的PIB
 UINT8 AIB_attribute_ID;//PIB对应的标识
 LADDR_UNION dstADDR;//目的地址（长地址和短地址）
 BYTE dstEP;        //目的端点
 BYTE cluster;      //簇
 BYTE srcEP;      //源EP
 BYTE plen;       //长度
 BYTE tsn;      //频率
 BYTE reqack;   //是否请求响应
 UINT8 error;
 Lay_ID=a_aps_rx_data.usrPload[7];
 AIB_attribute_ID=a_aps_rx_data.usrPload[8];
 h2n16(macGetShortAddr(),pload_rsp);
 pload_rsp[2] = EVS_READ_POS;
 memcpy(pload_rsp+3,a_aps_rx_data.usrPload+3,32);
 pload_rsp[7] = Lay_ID;
 pload_rsp[8] = AIB_attribute_ID;
 switch(Lay_ID){
         case PHY_AIB_ID:
        switch(AIB_attribute_ID){
                case 0x00:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = phy_pib.phyCurrentChannel;
                         pload_rsp[11] = 0;//CRC校验
                         plen=12;
                         break;
                case 0x01:
                         pload_rsp[9] = 4;//读取的属性的长度
                         pload_rsp[10] =phy_pib.phyChannelsSupported&0xff;
                         pload_rsp[11] =phy_pib.phyChannelsSupported>>8&0xff;
                         pload_rsp[12] =phy_pib.phyChannelsSupported>>16&0xff;
                         pload_rsp[13] =phy_pib.phyChannelsSupported>>24&0xff;
                         pload_rsp[14] = 0;//CRC校验
                         plen=15;
                         break;
                 case 0x02:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = phy_pib.phyTransmitPower;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                 case 0x03:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = phy_pib.phyCCAMode;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                 case All_Attribute_ID:
                         pload_rsp[9] = phy_pib.phyCurrentChannel;
                         pload_rsp[10] =phy_pib.phyChannelsSupported&0xff;
                         pload_rsp[11] =phy_pib.phyChannelsSupported>>8&0xff;
                         pload_rsp[12] =phy_pib.phyChannelsSupported>>16&0xff;
                         pload_rsp[13] =phy_pib.phyChannelsSupported>>24&0xff;
                         pload_rsp[14] = phy_pib.phyTransmitPower;
                         pload_rsp[15] = phy_pib.phyCCAMode;
                         pload_rsp[16] = 0;
                         plen=17;
                         break;
		 default:
		 	   error=01;//attribute_ID wrong
                        goto NES;

        }
      case MAC_AIB_ID:
        switch(AIB_attribute_ID){
		 case 0x4A:        //该标识代表读MAC的扩展地址
                         pload_rsp[9] = 8;//读取的属性的长度
                         memcpy(pload_rsp+10,mac_pib.macCoordExtendedAddress.bytes,64);
                         pload_rsp[18] = 0;
                         plen=19;
                         break;

		case 0x4B:
                         pload_rsp[9] = 2;
                         pload_rsp[10] = mac_pib.macCoordShortAddress&0xff;
                         pload_rsp[11] = mac_pib.macCoordShortAddress>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;
	        case 0x4C:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = mac_pib.macDSN;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
	        case 0x50:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = mac_pib.macPANID;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case 0x53:
                         pload_rsp[9] = 2;
                         pload_rsp[10] = mac_pib.macShortAddress&0xff;
                         pload_rsp[11] = mac_pib.macShortAddress>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;
                case 0x63:
                         pload_rsp[9] = 8;//读取的属性的长度
                         memcpy(pload_rsp+10,mac_pib.macExtendedAddress.bytes,64);
                         pload_rsp[18] = 0;
                         plen=19;
                         break;
	        case 0x68:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = mac_pib.currentAckRetries;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
	        case 0x69:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = mac_pib.rxTail;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
	        case 0x6a:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = mac_pib.rxHead;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;

                case All_Attribute_ID:
                         memcpy(pload_rsp+9,mac_pib.macCoordExtendedAddress.bytes,64);
                         pload_rsp[17] = mac_pib.macCoordShortAddress&&0xff;
                         pload_rsp[18] = mac_pib.macCoordShortAddress>>8;
                         pload_rsp[19] = mac_pib.macDSN;
                         pload_rsp[20] = mac_pib.macPANID;
                         pload_rsp[21] = mac_pib.macShortAddress&0xff;
                         pload_rsp[22] = mac_pib.macShortAddress>>8;
                         memcpy(pload_rsp+23,mac_pib.macExtendedAddress.bytes,64);
                         pload_rsp[31] = mac_pib.currentAckRetries;
                         pload_rsp[32] = mac_pib.rxTail;
                         pload_rsp[33] = mac_pib.rxHead;
                         plen=34;
                         break;

		default:
			error=01;//attribute_ID wrong
                       goto NES;
     }
				
     case NWK_AIB_ID:
        switch(AIB_attribute_ID){
                case 0x90:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = nwk_pib.nwkDSN;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case 0x91:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = nwk_pib.nwkNetworkCount;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                #ifdef LRWPAN_FFD
                case 0x92:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = nwk_pib.rxTail;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case 0x93:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] =nwk_pib.rxHead;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                #endif
                case All_Attribute_ID:
                         pload_rsp[9] = nwk_pib.nwkDSN;
                         pload_rsp[10] = nwk_pib.nwkNetworkCount;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
               	default:
			error=01;//attribute_ID wrong
                       goto NES;
        }
     case APS_AIB_ID:
        switch(AIB_attribute_ID){
          	case 0xa0://activeEPs:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = aps_pib.activeEPs;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case 0xa1:
                         pload_rsp[9] = 4;
                         pload_rsp[10] =aps_pib.tx_start_time&0xff;
                         pload_rsp[11] =aps_pib.tx_start_time>>8&0xff;
                         pload_rsp[12] =aps_pib.tx_start_time>>16&0xff;
                         pload_rsp[13] =aps_pib.tx_start_time>>24&0xff;
                         pload_rsp[14] = 0;
                         plen=15;
                         break;
                case 0xa2:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = aps_pib.apsTSN;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;

                case 0xa3:
                         pload_rsp[9] = 4;
                         pload_rsp[10] =aps_pib.apscAckWaitDuration&0xff;
                         pload_rsp[11] =aps_pib.apscAckWaitDuration>>8&0xff;
                         pload_rsp[12] =aps_pib.apscAckWaitDuration>>16&0xff;
                         pload_rsp[13] =aps_pib.apscAckWaitDuration>>24&0xff;
                         pload_rsp[14] = 0;
                         plen=15;
                         break;
                case 0xa4:
                         pload_rsp[9] = 2;
                         pload_rsp[10] = aps_pib.apsAckWaitMultiplier&0xff;
                         pload_rsp[11] = aps_pib.apsAckWaitMultiplier>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;
                case 0xa5:
                         pload_rsp[9] = 2;
                         pload_rsp[10] = aps_pib.apsAckWaitMultiplierCntr&0xff;
                         pload_rsp[11] = aps_pib.apsAckWaitMultiplierCntr>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;
                case 0xa6:
                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = aps_pib.apscMaxFrameRetries;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case 0xa7:

                         pload_rsp[9] = 1;//读取的属性的长度
                         pload_rsp[10] = aps_pib.currentAckRetries;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case All_Attribute_ID:
                         pload_rsp[9] = aps_pib.activeEPs;
                         pload_rsp[10] =aps_pib.tx_start_time&0xff;
                         pload_rsp[11] =aps_pib.tx_start_time>>8&0xff;
                         pload_rsp[12] =aps_pib.tx_start_time>>16&0xff;
                         pload_rsp[13] =aps_pib.tx_start_time>>24&0xff;
                         pload_rsp[14] = aps_pib.apsTSN;
                         pload_rsp[15] =aps_pib.apscAckWaitDuration&0xff;
                         pload_rsp[16] =aps_pib.apscAckWaitDuration>>8&0xff;
                         pload_rsp[17] =aps_pib.apscAckWaitDuration>>16&0xff;
                         pload_rsp[18] =aps_pib.apscAckWaitDuration>>24&0xff;
                         pload_rsp[19] = aps_pib.apsAckWaitMultiplier&0xff;
                         pload_rsp[20] = aps_pib.apsAckWaitMultiplier>>8;
                         pload_rsp[21] = aps_pib.apsAckWaitMultiplierCntr&0xff;
                         pload_rsp[22] = aps_pib.apsAckWaitMultiplierCntr>>8;
                         pload_rsp[23] = aps_pib.apscMaxFrameRetries;
                         pload_rsp[24] = aps_pib.currentAckRetries;
                         pload_rsp[25] = 0;
                         plen=26;
                         break;
       }
     case 0xFF:
        break;
     default:
	 error=02;//lay_ID wrong
        goto NES;

     }

dstADDR.saddr=(UINT16)(a_aps_rx_data.usrPload[0]<<8)+a_aps_rx_data.usrPload[1];

if(dstADDR.saddr==0x0000)
{
  memcpy(usrRxData,pload_rsp,plen);
#ifdef LRWPAN_COORDINATOR
usrSendDataToGateway();
#endif
}
else{
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_POS(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
goto Read_end;
NES:
pload_rsp[0] = macGetShortAddr()>>8;
pload_rsp[1] = macGetShortAddr()&0xff;
pload_rsp[2] = EVS_READ_NEG;
pload_rsp[9]= error;
pload_rsp[10]=0;
plen=11;
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_NES(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
Read_end:
apsState=APS_STATE_IDLE;
apsSetTxIdle();}
}

ZigBeePIB_write(){
 BYTE pload_rsp[50];
 UINT8 Lay_ID;
 UINT8 AIB_attribute_ID;//PIB对应的标识
 LADDR_UNION dstADDR;//目的地址（长地址和短地址）
 BYTE dstEP;        //目的端点
 BYTE cluster;      //簇
 BYTE srcEP;      //源EP
 BYTE plen;
 BYTE tsn;      //频率
 BYTE reqack;
 UINT8 error;
 Lay_ID=a_aps_rx_data.usrPload[7];
 AIB_attribute_ID=a_aps_rx_data.usrPload[8];//usrPload[9]中是写入的长度
 h2n16(macGetShortAddr(),pload_rsp);
 pload_rsp[2] = EVS_WRITE_POS;
 memcpy(pload_rsp+3,a_aps_rx_data.usrPload+3,32);
 pload_rsp[7] = Lay_ID;
 pload_rsp[8] = AIB_attribute_ID;
 switch(Lay_ID){
         case PHY_AIB_ID:
        switch(AIB_attribute_ID){
                case 0x00:
                         pload_rsp[9] = 1;//写入的属性的长度
                         phy_pib.phyCurrentChannel=a_aps_rx_data.usrPload[10];
                         break;
                case 0x01:
                         pload_rsp[9] = 4;//写入的属性的长度
                         h2n32(phy_pib.phyChannelsSupported,a_aps_rx_data.usrPload+10);
                         break;
                 case 0x02:
                         pload_rsp[9] = 1;
                         phy_pib.phyTransmitPower=a_aps_rx_data.usrPload[10];
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                 case 0x03:
                         pload_rsp[9] = 1;
                         phy_pib.phyCCAMode=a_aps_rx_data.usrPload[10];
                         break;
		 default:
		 	   error=01;//attribute_ID wrong
                        goto NES;

        }
      case MAC_AIB_ID:
        switch(AIB_attribute_ID){
		 case 0x4A:        //该标识代表写MAC的扩展地址
                         pload_rsp[9] = 8;
                         memcpy(mac_pib.macCoordExtendedAddress.bytes,a_aps_rx_data.usrPload+10,64);
                         break;

		case 0x4B:
                         pload_rsp[9] = 2;
                         n2h16(&mac_pib.macCoordShortAddress,a_aps_rx_data.usrPload+10);
                         break;
	        case 0x4C:
                         pload_rsp[9] = 1;
                         mac_pib.macDSN=a_aps_rx_data.usrPload[10];
                         break;
	        case 0x50:
                         pload_rsp[9] = 1;
                         mac_pib.macPANID=a_aps_rx_data.usrPload[10];
                         break;
                case 0x53:
                         pload_rsp[9] = 2;
                         n2h16(&mac_pib.macShortAddress,a_aps_rx_data.usrPload+10);
                         break;
                case 0x63:
                         pload_rsp[9] = 8;//
                         memcpy(mac_pib.macExtendedAddress.bytes,a_aps_rx_data.usrPload+10,64);
                         break;
	        case 0x68:
                         pload_rsp[9] = 1;
                         mac_pib.currentAckRetries=a_aps_rx_data.usrPload[10];
                         break;
	        case 0x69:
                         pload_rsp[9] = 1;
                         mac_pib.rxTail=a_aps_rx_data.usrPload[10];
                         break;
	        case 0x6a:
                         pload_rsp[9] = 1;
                         mac_pib.rxHead=a_aps_rx_data.usrPload[10];
                         break;

		default:
			error=01;//attribute_ID wrong
                       goto NES;
     }
				
     case NWK_AIB_ID:
        switch(AIB_attribute_ID){
                case 0x90:
                         pload_rsp[9] = 1;
                         nwk_pib.nwkDSN=a_aps_rx_data.usrPload[10];
                         break;
                case 0x91:
                         pload_rsp[9] = 1;
                         nwk_pib.nwkNetworkCount=a_aps_rx_data.usrPload[10];
                         break;
                #ifdef LRWPAN_FFD
                case 0x92:
                         pload_rsp[9] = 1;
                         nwk_pib.rxTail=a_aps_rx_data.usrPload[10];
                         break;
                case 0x93:
                         pload_rsp[9] = 1;
                         nwk_pib.rxHead=a_aps_rx_data.usrPload[10];
                         break;
                #endif
               	default:
			error=01;//attribute_ID wrong
                       goto NES;
        }
     case APS_AIB_ID:
        switch(AIB_attribute_ID){
          	case 0xa0://activeEPs:
                         pload_rsp[9] = 1;//
                         aps_pib.activeEPs=a_aps_rx_data.usrPload[10];
                         break;
                case 0xa1:
                         pload_rsp[9] = 4;
                         h2n32(aps_pib.tx_start_time,a_aps_rx_data.usrPload+10);
                         break;
                case 0xa2:
                         pload_rsp[9] = 1;
                         aps_pib.apsTSN=a_aps_rx_data.usrPload[10];
                         break;

                case 0xa3:
                         pload_rsp[9] = 4;
                         h2n32(aps_pib.apscAckWaitDuration,a_aps_rx_data.usrPload+10);
                         break;
                case 0xa4:
                         pload_rsp[9] = 2;
                         n2h16(&aps_pib.apsAckWaitMultiplier,a_aps_rx_data.usrPload+10);
                         break;
                case 0xa5:
                         pload_rsp[9] = 2;
                         n2h16(&aps_pib.apsAckWaitMultiplierCntr,a_aps_rx_data.usrPload+10);
                         break;
                case 0xa6:
                         pload_rsp[9] = 1;
                         aps_pib.apscMaxFrameRetries=a_aps_rx_data.usrPload[10];
                         break;
                case 0xa7:

                         pload_rsp[9] = 1;
                         aps_pib.currentAckRetries=a_aps_rx_data.usrPload[10];
                         break;

       }
     case 0xFF:
        break;
     default:
	 error=02;//lay_ID wrong
        goto NES;
     }
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
pload_rsp[10]=0;
plen=11;
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
goto Read_end;
NES:
pload_rsp[0] = macGetShortAddr()>>8;
pload_rsp[1] = macGetShortAddr()&0xff;
pload_rsp[2] = EVS_READ_NEG;
pload_rsp[9]= error;
pload_rsp[10]=0;
plen=11;
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
Read_end:
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}
void h2n16(UINT16 src, UINT8 dst[]) {
	dst[0] = (UINT8)(src >> 8);
	dst[1] = (UINT8)(src);
}


 void h2n32(UINT32 src, UINT8 dst[]) {
	dst[0] = (UINT8)(src >> 24);
	dst[1] = (UINT8)(src >> 16);
	dst[2] = (UINT8)(src >> 8);
	dst[3] = (UINT8)(src);
}
void n2h16(UINT16* dst,UINT8 src[] ) {
	*dst = (UINT16)src[0] << 8 | (UINT8)src[1];
}

/*------------------------------------------------------------------------------*
 *- Function name: n2h32                                                       -*
 *- Parameter: src, source byte array                                          -*
 *-            dst, destination variable pointer                               -*
 *- Return value: void                                                         -*
 *- Brief: Tran a net byte sequence array to 32-bit variable of host byte      -*
 *-        sequence                                                            -*
 *------------------------------------------------------------------------------*/
void n2h32(UINT32* dst,UINT8 src[]) {
	*dst = (UINT32)src[0] << 24 | (UINT32)src[1] << 16 | (UINT32)src[2] << 8 | (UINT32)src[3];
}


/*
void DEVDES_Read(){
    BYTE pload_rsp[50];
    LADDR_UNION dstADDR;//目的地址（长地址和短地址）
    BYTE dstEP;        //目的端点
    BYTE cluster;      //簇
    BYTE srcEP;      //源EP
    BYTE plen;       //长度
    BYTE tsn;      //频率
    BYTE reqack;   //是否请求响应
    UINT8 Desc_Class;
    UINT8 Desc_ID;
    BYTE error;
 Desc_Class=a_aps_rx_data.usrPload[7];
 Desc_ID=a_aps_rx_data.usrPload[8];
 h2n16(macGetShortAddr(),pload_rsp);
 pload_rsp[2] = EVS_READ_POS;
 memcpy(pload_rsp+3,a_aps_rx_data.usrPload+3,32);//类号，MIB类号
 pload_rsp[7] = Desc_Class;
 pload_rsp[8] = Desc_ID;
 switch(Desc_Class){
         case NODE_DESC_CLASS:
          switch(Desc_ID){
                case localtypeID:
                         pload_rsp[9] = 1;
                         pload_rsp[10]= nodedescriptor.localtype;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case flags_valID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodedescriptor.flags.val;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case APS_flagsID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodedescriptor.flags.bits.APS_flags;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case FrequencybandID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodedescriptor.flags.bits.Frequencyband;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case MACcapabilityflagsID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodedescriptor.MACcapabilityflags;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case ManufacturercodeID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodedescriptor.Manufacturercode&0xff;
                         pload_rsp[11] = nodedescriptor.Manufacturercode>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;
                case MaximumbuffersizeID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodedescriptor.Maximumbuffersize;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case MaximumtransfersizeID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] =nodedescriptor.Maximumtransfersize&0xff;
                         pload_rsp[11] =nodedescriptor.Maximumtransfersize>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;
                default:
			error=03;//Desc_ID wrong
                         goto NES;

          }
  case NODE_POWER_DESC_CLASS:
          switch(Desc_ID){
                case mode_valID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodepowerdescriptor.mode.val;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case CurrentpowermodeID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodepowerdescriptor.mode.bits.Currentpowermode;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case AvailablepowersourcesID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodepowerdescriptor.mode.bits.Availablepowersources;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case source_valID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodepowerdescriptor.source.val;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case CurrentpowersourceID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] =(nodepowerdescriptor.source.bits.Currentpowersource);
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case CurrentpowersourcelevelID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = nodepowerdescriptor.source.bits.Currentpowersourcelevel;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                default:
			error=03;//Desc_ID wrong
                         goto NES;

          }


         case SIMPLE_CLASS:
          switch(Desc_ID){
                case endpointID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] =simpledescriptor.endpoint;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case ApplicationprofileidentifierID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = simpledescriptor.Applicationprofileidentifier&0xff;
                         pload_rsp[11] =  simpledescriptor.Applicationprofileidentifier>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;

                case ApplicationdeviceidentifierID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] = simpledescriptor.Applicationdeviceidentifier&0xff;
                         pload_rsp[11] =  simpledescriptor.Applicationdeviceidentifier>>8;
                         pload_rsp[12] = 0;
                         plen=13;
                         break;
                case S_flags_valID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] =simpledescriptor.flags.val;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;

                case ApplicationdeviceversionID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] =simpledescriptor.flags.bits.Applicationdeviceversion;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                case ApplicationflagsID:
                         pload_rsp[9] = 1;
                         pload_rsp[10] =simpledescriptor.flags.bits.Applicationflags;
                         pload_rsp[11] = 0;
                         plen=12;
                         break;
                 default:
			error=03;//Desc_ID wrong
                        goto NES;

          }
     case 0xFF:
        break;
     default:
	 error=04;//Desc_Class wrong
        goto NES;
 }
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_POS(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
goto Read_end;
NES:
pload_rsp[0] = macGetShortAddr()>>8;
pload_rsp[1] = macGetShortAddr()&0xff;
pload_rsp[2] = EVS_READ_NEG;
pload_rsp[9]= error;
pload_rsp[10]=0;
plen=11;
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_NES(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
Read_end:
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}
*/

/***********
写设备
***********/
/*
void DEVDES_Write(){
    BYTE pload_rsp[50];
    LADDR_UNION dstADDR;//目的地址（长地址和短地址）
    BYTE dstEP;        //目的端点
    BYTE cluster;      //簇
    BYTE srcEP;      //源EP
    BYTE plen;       //长度
    BYTE tsn;      //频率
    BYTE reqack;   //是否请求响应
    UINT8 Desc_Class;
    UINT8 Desc_ID;
    BYTE error;
 Desc_Class=a_aps_rx_data.usrPload[7];
 Desc_ID=a_aps_rx_data.usrPload[8];// usrPload[9]是要写入的属性长度
 h2n16(macGetShortAddr(),pload_rsp);
 pload_rsp[2] = EVS_READ_POS;
 memcpy(pload_rsp+3,a_aps_rx_data.usrPload+3,32);//类号，MIB类号
 pload_rsp[7] = Desc_Class;
 pload_rsp[8] = Desc_ID;
 switch(Desc_Class){
         case NODE_DESC_CLASS:
          switch(Desc_ID){
                case localtypeID:
                         pload_rsp[9] = 1;
                         nodedescriptor.localtype=a_aps_rx_data.usrPload[10];
                         break;
                case flags_valID:
                         pload_rsp[9] = 1;
                         nodedescriptor.flags.val=a_aps_rx_data.usrPload[10];
                         break;
                case APS_flagsID:
                         pload_rsp[9] = 1;
                         nodedescriptor.flags.bits.APS_flags=a_aps_rx_data.usrPload[10];
                         break;
                case FrequencybandID:
                         pload_rsp[9] = 1;
                         nodedescriptor.flags.bits.Frequencyband=a_aps_rx_data.usrPload[10];
                         break;
                case MACcapabilityflagsID:
                         pload_rsp[9] = 1;
                         nodedescriptor.MACcapabilityflags=a_aps_rx_data.usrPload[10];
                         break;
                case ManufacturercodeID:
                         pload_rsp[9] = 1;
                         n2h16(&nodedescriptor.Manufacturercode,a_aps_rx_data.usrPload+10);
                         break;
                case MaximumbuffersizeID:
                         pload_rsp[9] = 1;
                         nodedescriptor.Maximumbuffersize=a_aps_rx_data.usrPload[10];
                         break;
                case MaximumtransfersizeID:
                         pload_rsp[9] = 1;
                         n2h16(&nodedescriptor.Maximumtransfersize,a_aps_rx_data.usrPload+10);
                         break;
                default:
			error=03;//Desc_ID wrong
                         goto NES;

          }
  case NODE_POWER_DESC_CLASS:
          switch(Desc_ID){
                case mode_valID:
                         pload_rsp[9] = 1;
                        nodepowerdescriptor.mode.val=a_aps_rx_data.usrPload[10];
                         break;
                case CurrentpowermodeID:
                         pload_rsp[9] = 1;
                         nodepowerdescriptor.mode.bits.Currentpowermode=a_aps_rx_data.usrPload[10];
                         break;
                case AvailablepowersourcesID:
                         pload_rsp[9] = 1;
                         nodepowerdescriptor.mode.bits.Availablepowersources=a_aps_rx_data.usrPload[10];
                         break;
                case source_valID:
                         pload_rsp[9] = 1;
                         nodepowerdescriptor.source.val=a_aps_rx_data.usrPload[10];
                         break;
                case CurrentpowersourceID:
                         pload_rsp[9] = 1;
                         nodepowerdescriptor.source.bits.Currentpowersource=a_aps_rx_data.usrPload[10];
                         break;
                case CurrentpowersourcelevelID:
                         pload_rsp[9] = 1;
                         nodepowerdescriptor.source.bits.Currentpowersourcelevel=a_aps_rx_data.usrPload[10];
                         break;
                default:
			error=03;//Desc_ID wrong
                         goto NES;

          }


         case SIMPLE_CLASS:
          switch(Desc_ID){
                case endpointID:
                         pload_rsp[9] = 1;
                         simpledescriptor.endpoint=a_aps_rx_data.usrPload[10];
                         break;
                case ApplicationprofileidentifierID:
                         pload_rsp[9] = 1;
                         n2h16(&simpledescriptor.Applicationprofileidentifier,a_aps_rx_data.usrPload+10);
                         break;

                case ApplicationdeviceidentifierID:
                         pload_rsp[9] = 1;
                         n2h16(&simpledescriptor.Applicationdeviceidentifier,a_aps_rx_data.usrPload+10);
                         break;
                case S_flags_valID:
                         pload_rsp[9] = 1;
                         simpledescriptor.flags.val=a_aps_rx_data.usrPload[10];
                         break;

                case ApplicationdeviceversionID:
                         pload_rsp[9] = 1;
                         simpledescriptor.flags.bits.Applicationdeviceversion=a_aps_rx_data.usrPload[10];
                         break;
                case ApplicationflagsID:
                         pload_rsp[9] = 1;
                         simpledescriptor.flags.bits.Applicationflags=a_aps_rx_data.usrPload[10];
                         break;
                 default:
			error=03;//Desc_ID wrong
                        goto NES;

          }
     case 0xFF:
        break;
     default:
	 error=04;//Desc_Class wrong
        goto NES;
 }
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
pload_rsp[10]=0;
plen=11;
apsRxState=APS_RXSTATE_IDLE;
Read_POS(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
goto Read_end;
NES:
pload_rsp[2] = EVS_READ_NEG;
pload_rsp[9]= error;
pload_rsp[10]=0;
plen=11;
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_NES(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
Read_end:
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}
*/