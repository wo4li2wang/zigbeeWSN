/*
*2007/09/29 WXL 2.0
*/

//-----------------------------------------------------------------------------													
// Description:	ZigBee User Layer. This is a test unit.			
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
#include "user.h"
#ifdef SECURITY_GLOBALS
#include "security.h"
#endif


extern DevDes gdev_des;
extern BYTE usrRxFlag;
extern BYTE usrTxData[85];
extern LADDR_UNION dstADDR;
BYTE ledstatus;
BYTE PCdata[4];

void servicefunction(void){
  LADDR_UNION dstADDR;
dstADDR.saddr=(UINT16)(a_aps_rx_data.usrPload[0]<<8)+a_aps_rx_data.usrPload[1];
if(dstADDR.saddr==0x0000)
{
switch(*(a_aps_rx_data.usrPload+2)) {
    case EVS_READ:
      Read_rsp();
      break;
    case EVS_WRITE:
      Write_rsp();
      break;
    case NSID_AUTHEN:                               // this service is send from node to gateway
      Authen_Reply();                              //add by weimin for gateway send the authen message to the PC
      break;
    default:
      break;
}}
else
{
switch(*(a_aps_rx_data.usrPload+2)) {
    case NSID_AUTHEN_POS:
      Authen_POS_rsp();
      break;
    case NSID_AUTHEN_NEG:
      Authen_NEG_rsp();
      break;
    default:
      break;
    	}
}
}





void Read_POS_rsp()
{
UINT8 len;
len=a_aps_rx_data.usrPload[9]+11;
memcpy(usrRxData,a_aps_rx_data.usrPload,len);
//mini//usrSendDataToGateway();
}


void Read_NES_rsp()
{
}
void EVS_DISTRIBUTE_rsp()
{
  BYTE i,len,*ptr;
  len = aplGetRxMsgLen();
  ptr = aplGetRxMsgData();
  for (i=0;i<len;i++) {
    usrRxData[i] = *ptr;
    ptr++;}
   usrRxFlag = 1;
	
}


//读响应
void Read_rsp()
{

UINT8 dst_app;
dst_app=(UINT16)(a_aps_rx_data.usrPload[3]<<8)+a_aps_rx_data.usrPload[4];
switch(dst_app) {
	case ZigBee_MIB_APP_ID:
	MIBRead();
		break;
	default:
	 	break;
	}
}



void Read_POS(BYTE dstMode,LADDR_UNION *dstADDR,BYTE dstEP,BYTE cluster,BYTE srcEP,BYTE* pload,
                    BYTE plen,BYTE tsn,BYTE reqack)
{
aplSendMSG(dstMode, dstADDR, dstEP, cluster, srcEP, pload, plen, tsn, reqack)
}


void Read_NES(BYTE dstMode,LADDR_UNION *dstADDR,BYTE dstEP,BYTE cluster,BYTE srcEP,BYTE* pload,
                    BYTE plen,BYTE tsn,BYTE reqack)
{
aplSendMSG(dstMode, dstADDR, dstEP, cluster, srcEP, pload, plen, tsn, reqack);
}





//写响应
void Write_rsp()
{
UINT8 Lay_num;           //读哪一层的PIB
UINT8 AIB_attribute_num;//PIB对应的标识
 a_aps_rx_data.usrPload++;
 Lay_num=*a_aps_rx_data.usrPload;
 a_aps_rx_data.usrPload++;
 AIB_attribute_num=*a_aps_rx_data.usrPload;
 a_aps_rx_data.usrPload++;
 switch(Lay_num){
      case MAC_AIB_ID:
        switch(AIB_attribute_num){

		        case 0x4A:        //该标识代表读MAC的扩展地址
                          memcpy(mac_pib.macCoordExtendedAddress.bytes,a_aps_rx_data.usrPload,64);
                                 break;
			case 0x4B:
                          mac_pib.macCoordShortAddress=(UINT16)*(a_aps_rx_data.usrPload+1);
                          mac_pib.macCoordShortAddress=(mac_pib.macCoordShortAddress<<8)&(*a_aps_rx_data.usrPload);
			        break;
		        case 0x4C:
                                mac_pib.macDSN=*a_aps_rx_data.usrPload;
			        break;
			case 0x50:
				mac_pib.macPANID=*a_aps_rx_data.usrPload;
			        break;
		      default:
			        break;
       }
				
     case NWK_AIB_ID:
       // switch(AIB_attribute_num)
          break;
     case APS_AIB_ID:
       // switch(AIB_attribute_num)
        break;
     case 0xFF:
        break;
     default:
        break;
     }
}

void  WIRELESSDB_Device(BYTE *devicedata,BYTE length)
           {
           usrTxData[2]=length;
	    usrTxData[3]=0x55;
           memcpy(usrTxData+4,devicedata,length);
           dstADDR.saddr = 0x0000;
           apsState = APS_STATE_IDLE;
           aplSendMSG (APS_DSTMODE_SHORT,&dstADDR,2,0,1,&usrTxData[0],length+4,apsGenTSN(),FALSE);
          }

#ifdef LRWPAN_COORDINATOR
void usrSendDataToGateway(void)
{
  UINT8 i;
  for(i=0;i<usrRxData[2]+4;i++)
  halRawPut(usrRxData[i]);
  halRawPut(0xff);
  halRawPut(0xff);
}

void  usrRecedataFromGateway(void)
{
	  usrTxData[0]=PCdata[0];
	  usrTxData[1]=PCdata[1];
	  dstADDR.saddr = 0xffff;                //方便调试，地址写死
           aplSendMSG (APS_DSTMODE_SHORT,&dstADDR,1,0,2,&usrTxData[0],2,apsGenTSN(),FALSE); //No APS ack requested

}


#endif



void LED_FLASH(void)
{
  ledstatus++;
      if (ledstatus&0x01)
        {
        EVB_LED1_OFF();
        }
      else
         {
        EVB_LED1_ON();
          }
}