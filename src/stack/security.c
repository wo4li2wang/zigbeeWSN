/*
*2007/09/29 WXL 2.0
*/

//-----------------------------------------------------------------------------														
// Description:	ZigBee security. This is a test unit.			
//
// Refer to Standard for Architecture and Digital Data Communication for
//	 ZigBee-based Industrial Control Systems
//-------------------------------------------------------------------



#include "compiler.h"
#include "string.h"
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
#include"security.h"
#endif


#ifdef SECURITY_GLOBALS
#define NSMIB_BASE_OBJID_MIBHDR         (10)
#define NSMIB_BASE_OBJID_KEY_MAN        (8001)
#define NSMIB_BASE_OBJID_SEC_MEASURE    (8002)
#define NSMIB_BASE_OBJID_PTCL_MAN       (8003)
#define NSMIB_BASE_OBJID_AUTHEN         (8004)
#define NSMIB_BASE_OBJID_ACCTRL_OBJ     (8500)
#define MIB_NUM_ACCTRL_OBJ              (5)

#define NS_SEC_ID_LEN                   (32)
#define NS_SEC_KEY_LEN                  (32)
#define NS_SEC_KEY_TBL_LEN              (64)

/**********************
for security parameter ,by weimin
*******************************/

KeyMan	        gns_key_man;
AuthenObj	gns_authen_obj;
MAC_SECURITY_PIB	gns_mac_securitypib;
SecMeasure 	gns_sec_measure;
SecPtclMan    gns_sec_ptcl_man;
ACCtrlObj     gns_acctrl_obj[MIB_NUM_ACCTRL_OBJ];
/*********************************
********************************/

void KeyMan_Init(void) {
	gns_key_man.obj_id = NSMIB_BASE_OBJID_KEY_MAN;
	gns_key_man.revision = 0;
        memcpy(gns_key_man.initial_key,"12345678abcdefgh",8);
	//gns_key_man.initial_key = "12345678abcdefgh";
	memset(gns_key_man.transfer_key, 0xCD, 16);
	memset(gns_key_man.communicat_key, 0xCD, 16);
	gns_key_man.key_off = 0;
	gns_key_man.key_life = 0;
	gns_key_man.key_len =16;
	gns_key_man.current_key_type = Initialkey;
	gns_key_man.attack_time = 0;
	gns_key_man.attack_type = 0;
}

void AuthenObj_Init(void) {
	gns_authen_obj.obj_id = NSMIB_BASE_OBJID_AUTHEN;
	gns_authen_obj.revision = 0;
	memcpy(gns_authen_obj.sec_id, NS_DEFAULT_SEC_ID, NS_SEC_ID_LEN);
	gns_authen_obj.stamp.sec = 0;
	gns_authen_obj.stamp.us = 0;
	gns_authen_obj.state = 0;
}

void GetSecID(BYTE secid[]) {
	memcpy(secid, gns_authen_obj.sec_id, 32);
}


void MAC_SECURITY_PIB_Init(void){
	gns_mac_securitypib.macACLEntryDescriptorSet.ACLExtendedAddress=mac_pib.macExtendedAddress;
	gns_mac_securitypib.macACLEntryDescriptorSet.ACLShortAddress=mac_pib.macShortAddress;
	gns_mac_securitypib.macACLEntryDescriptorSet.ACLPANId =mac_pib.macPANID;
	gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecurityMateriallength=21;
	gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecurityMaterial=NULL;
	gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecuritySuit=0x00;
	gns_mac_securitypib.macACLEntryDescriporSetSize=0x00;
	gns_mac_securitypib.macDefaultSecurity=FALSE;
	gns_mac_securitypib.macDefautSecurityMaterial=NULL;
	gns_mac_securitypib.macDefautSecurityMaterialLength=0x10;
	gns_mac_securitypib.macDefautSecuritySuite=0x00;
	gns_mac_securitypib.macSecurityMode=0x00;
}


void SecMeasure_Init(void) {
	gns_sec_measure.obj_id = NSMIB_BASE_OBJID_SEC_MEASURE;
	gns_sec_measure.revision = 0;
	gns_sec_measure.sec_mode = 0x0F;
	gns_sec_measure.authen_mode = 0x10;//maybe rewrite later
	gns_sec_measure.encypt_mode = 0x02;
}

void SecPtclMan_Init(void) {
	gns_sec_ptcl_man.obj_id = NSMIB_BASE_OBJID_PTCL_MAN;
	gns_sec_ptcl_man.revision = 0;
	gns_sec_ptcl_man.time_limit = 0;
	gns_sec_ptcl_man.run_mode = 0;
	gns_sec_ptcl_man.link_mode = 0;
	gns_sec_ptcl_man.authen_mode = 0;
	gns_sec_ptcl_man.Ptcl_type = 0;
}
void AcctrlObj_Init(void) {
	UINT8 idx;
	
	for(idx = 0; idx < MIB_NUM_ACCTRL_OBJ; ++idx) {
		gns_acctrl_obj[idx].obj_id = NSMIB_BASE_OBJID_ACCTRL_OBJ + idx;
		gns_acctrl_obj[idx].local_app = 0;
		gns_acctrl_obj[idx].local_obj = 0;
		gns_acctrl_obj[idx].rmt_app = 0;
		gns_acctrl_obj[idx].rmt_obj = 0;
		gns_acctrl_obj[idx].srv_op = 0;
		gns_acctrl_obj[idx].srv_role = 0;
		gns_acctrl_obj[idx].rmt_ip = 0;
	//	gns_acctrl_obj[idx].send_time_offset = 0;
		gns_acctrl_obj[idx].right = 0;
		gns_acctrl_obj[idx].group = 0;
		gns_acctrl_obj[idx].psw = 0;
	}
}


void Authen_Reply(){
LADDR_UNION dstADDR;
BYTE pload_rsp[50];
pload_rsp[0]=a_aps_rx_data.usrPload[0];
pload_rsp[1]=a_aps_rx_data.usrPload[1];
dstADDR.saddr=a_aps_rx_data.srcSADDR;
pload_rsp[2]=NSID_AUTHEN_POS;
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR,1, 0,2,pload_rsp, 3, a_aps_rx_data.tsn, 0);
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}
void Authen_POS_rsp()
{
  gns_authen_obj.state=1;
}
void Authen_NEG_rsp()
{
}
void KeyMan_Read()
{
BYTE pload_rsp[50];
UINT8 idx,plen;
 LADDR_UNION dstADDR;//目的地址（长地址和短地址）
 BYTE dstEP;        //目的端点
 BYTE cluster;      //簇
 BYTE srcEP;      //源EP
 BYTE tsn;      //频率
 BYTE reqack;   //是否请求响应
idx=a_aps_rx_data.usrPload[7];
h2n16(macGetShortAddr(),pload_rsp);
        pload_rsp[2] = EVS_READ_POS;
        pload_rsp[3] = 0x00;
        pload_rsp[4]=0x01;
        pload_rsp[5]=0x1F;
        pload_rsp[6]=0x41;
	pload_rsp[7]=0;
	pload_rsp[8]=0;//保留
switch (idx) {
	      case 0:
	pload_rsp[9]=0x10;//长度16 个字节
        h2n16(gns_key_man.obj_id,pload_rsp+10);
        h2n16(gns_key_man.revision,pload_rsp+12);
	//	memcpy(pload_rsp + 14, gns_key_man.initial_key, 16);
	//	memcpy(pload_rsp + 30, gns_key_man.transfer_key, 16);
	//	memcpy(pload_rsp + 46, gns_key_man.communicat_key, 16);
	h2n16(gns_key_man.key_off,pload_rsp + 14);
	h2n16(gns_key_man.key_life,pload_rsp + 16);
	h2n16(gns_key_man.key_len,pload_rsp + 18);
        pload_rsp [19]=gns_key_man.current_key_type;
        h2n16(gns_key_man.attack_time,pload_rsp+20);
        h2n16(gns_key_man.attack_type,pload_rsp+22);
        pload_rsp[24]=0x00;
	plen=25;
		break;
	 case 1:
           pload_rsp[9]=0x02;//长度
	   h2n16(gns_key_man.obj_id, pload_rsp+10);
           pload_rsp[12]=0x00;
           plen=13;
	      break;	
       case 2:
           pload_rsp[9]=0x02;//长度
	   h2n16(gns_key_man.revision, pload_rsp+10);
           pload_rsp[12]=0x00;
           plen=13;
	      break;
	case 3:
		pload_rsp[9]=16;//长度
		memcpy(pload_rsp + 10, gns_key_man.initial_key, 16);
		pload_rsp[26]=0;
		plen=27;
	       break;
	case 4:
		pload_rsp[9]=16;//长度
		memcpy(pload_rsp + 10, gns_key_man.transfer_key, 16);
		pload_rsp[26]=0;
		plen=27;
	       break;
	case 5:
		pload_rsp[9]=16;//长度
		memcpy(pload_rsp + 10, gns_key_man.communicat_key, 16);
		pload_rsp[26]=0;
		plen=27;
	       break;
	 case 6:
           pload_rsp[9]=0x02;//长度
	   h2n16(gns_key_man.key_off, pload_rsp+10);
           pload_rsp[12]=0x00;
           plen=13;
	      break;
	case 7:
	   pload_rsp[9]=0x02;//长度
           h2n16(gns_key_man.key_life,pload_rsp+10);
           pload_rsp[12]=0x00;
           plen=13;
	      break;	
	case 8:
          pload_rsp[9]=0x02;//长度
          h2n16(gns_key_man.key_len,pload_rsp+10);
          pload_rsp[12]=0x00;
          plen=13;
		break;
	case 9:
          pload_rsp[9]=0x01;//长度
          pload_rsp [10]=gns_key_man.current_key_type;
          pload_rsp[11]=0x00;
          plen=12;
		break;
	case 10:
	  pload_rsp[9]=0x02;//长度
          h2n16(gns_key_man.attack_time,pload_rsp+10);
          pload_rsp[12]=0x00;
          plen=13;
	case 11:
          pload_rsp[9]=0x02;//长度
          h2n16(gns_key_man.attack_type,pload_rsp+10);	
          pload_rsp[12]=0x00;
          plen=13;
           break;
	default:
	pload_rsp[2] = EVS_READ_NEG;
        pload_rsp[9]=0;
        break;
	}
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();

}



void SecMeasure_Read()
{
BYTE pload_rsp[50];
UINT8 idx,plen;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP;        //目的端点
BYTE cluster;      //簇
BYTE srcEP;      //源EP
BYTE tsn;      //频率
BYTE reqack;   //是否请求响应
h2n16(macGetShortAddr(),pload_rsp);
       pload_rsp[2] = EVS_READ_POS;
       pload_rsp[3] = 0x00;
       pload_rsp[4]=0x01;
       pload_rsp[5]=0x1F;
       pload_rsp[6]=0x42;
	pload_rsp[8]=0;//保留
       idx=a_aps_rx_data.usrPload[7];
	pload_rsp[7]=idx;
	switch (idx) {
		case 0:
			pload_rsp[9]=0x07;
			h2n16(gns_sec_measure.obj_id, pload_rsp+10);
			h2n16(gns_sec_measure.revision, pload_rsp + 12);
			pload_rsp[14] = gns_sec_measure.sec_mode;
			pload_rsp[15] = gns_sec_measure.authen_mode;
			pload_rsp[16] = gns_sec_measure.encypt_mode;
			pload_rsp[17]=0;
			plen=18;
		break;
		case 2:
			pload_rsp[9]=0x02;
			h2n16(gns_sec_measure.obj_id, pload_rsp+10);
			pload_rsp[12]=0;
			plen=13;
			break;
		case 3:
			pload_rsp[9]=0x02;
			pload_rsp[10] = gns_sec_measure.sec_mode;
			pload_rsp[11]=0;
			plen=12;
			break;
		case 4:
			pload_rsp[9]=0x01;
			pload_rsp[10] = gns_sec_measure.authen_mode;
			pload_rsp[11]=0;
			plen=12;
			break;
		case 5:
			pload_rsp[9]=0x01;
			pload_rsp[10] = gns_sec_measure.encypt_mode;
			pload_rsp[11]=0;
			plen=12;
			break;
		default:
			pload_rsp[2] = EVS_READ_NEG;
        		pload_rsp[9]=0;
        		break;
	}
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}





void AuthenObj_Read()
{
BYTE pload_rsp[58];
UINT8 idx,plen;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP;        //目的端点
BYTE cluster;      //簇
BYTE srcEP;      //源EP
BYTE tsn;      //频率
BYTE reqack;   //是否请求响应
h2n16(macGetShortAddr(),pload_rsp);
       pload_rsp[2] = EVS_READ_POS;
       pload_rsp[3] = 0x00;
       pload_rsp[4]=0x01;
       pload_rsp[5]=0x1F;
       pload_rsp[6]=0x44;
	pload_rsp[8]=0;//保留
	idx=a_aps_rx_data.usrPload[7];
	pload_rsp[7]=idx;


	switch (idx) {
		case 0:
			pload_rsp[9]=45;
			h2n16(gns_authen_obj.obj_id, pload_rsp+10);
			h2n16(gns_authen_obj.revision, pload_rsp + 11);
			memcpy(pload_rsp + 13, gns_authen_obj.sec_id, 32);
			h2n32(gns_authen_obj.stamp.sec, pload_rsp + 45);
			h2n32(gns_authen_obj.stamp.us, pload_rsp + 49);
			pload_rsp[53] = gns_authen_obj.state;
			pload_rsp[54]=0;
			plen=55;
			break;
		case 2:
			pload_rsp[9]=2;
			h2n16(gns_authen_obj.revision, pload_rsp+10);
			pload_rsp[12]=0;
			plen=13;
			break;
		case 3:
			pload_rsp[9]=16;
			memcpy(pload_rsp+10, gns_authen_obj.sec_id, 16);
			pload_rsp[26]=0;
			plen=27;
			break;
		case 4:
			pload_rsp[9]=16;
			memcpy(pload_rsp+10, gns_authen_obj.sec_id+16, 16);
			pload_rsp[26]=0;
			plen=27;
			break;
		case 5:
			pload_rsp[9]=8;
			h2n32(gns_authen_obj.stamp.sec, pload_rsp+10);
			h2n32(gns_authen_obj.stamp.us, pload_rsp+13);
			pload_rsp[17]=0;
			plen=18;
			break;
		case 6:
			pload_rsp[9]=1;
			pload_rsp[10] = gns_authen_obj.state;
			pload_rsp[11]=0;
			plen=12;
			break;
		default:
			pload_rsp[2] = EVS_READ_NEG;
        		pload_rsp[9]=0;
        		break;
	}
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_POS(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();	
}


void MAC_SECURITY_PIB_READ()
{
BYTE pload_rsp[58];
UINT8 idx,plen,i;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP;        //目的端点
BYTE cluster;      //簇
BYTE srcEP;      //源EP
BYTE tsn;      //频率
BYTE reqack;   //是否请求响应
h2n16(macGetShortAddr(),pload_rsp);
       pload_rsp[2] = EVS_READ_POS;
       pload_rsp[3] = 0x00;
       pload_rsp[4]=0x01;
       pload_rsp[5]=0x1F;
       pload_rsp[6]=0x45;
	pload_rsp[8]=0;//保留
	idx=a_aps_rx_data.usrPload[7];
	pload_rsp[7]=idx;


switch (idx) {
	case 0:
		pload_rsp[9]=19;
		for(i=0;i<8;i++)
			{
		pload_rsp[10+i]=gns_mac_securitypib.macACLEntryDescriptorSet.ACLExtendedAddress.bytes[i];
				}
		h2n16(gns_mac_securitypib.macACLEntryDescriptorSet.ACLShortAddress, pload_rsp+18);
		h2n16(gns_mac_securitypib.macACLEntryDescriptorSet.ACLPANId, pload_rsp+20);
       	pload_rsp[22]=gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecurityMateriallength;
		pload_rsp[23]=gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecuritySuit;
		pload_rsp[24]=gns_mac_securitypib.macACLEntryDescriporSetSize;
		pload_rsp[25]=gns_mac_securitypib.macDefaultSecurity;	
		pload_rsp[26]=gns_mac_securitypib.macDefautSecurityMaterialLength;
		pload_rsp[27]=gns_mac_securitypib.macDefautSecuritySuite;
		pload_rsp[28]=gns_mac_securitypib.macSecurityMode;
		pload_rsp[29]=0;
		plen=30;
		break;
	case 1:
		pload_rsp[9]=8;
		for(i=0;i<8;i++)
	{
		pload_rsp[10+i]=gns_mac_securitypib.macACLEntryDescriptorSet.ACLExtendedAddress.bytes[i];
	}
		pload_rsp[18]=0;
		plen=19;
		break;
	case 2:
		pload_rsp[9]=2;
		h2n16(gns_mac_securitypib.macACLEntryDescriptorSet.ACLShortAddress, pload_rsp+10);
		pload_rsp[12]=0;
		plen=13;
		break;
	case 3:
		pload_rsp[9]=2;
		h2n16(gns_mac_securitypib.macACLEntryDescriptorSet.ACLPANId, pload_rsp+20);
 		pload_rsp[12]=0;
		plen=13;
		break;
	case 4:
		pload_rsp[9]=1;
   		pload_rsp[10]=gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecurityMateriallength;
		pload_rsp[11]=0;
		plen=12;
		break;
	case 5:
		pload_rsp[9]=1;
   		pload_rsp[10]=gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecuritySuit;
		pload_rsp[11]=0;
		plen=12;
		break;
	case 6:
		pload_rsp[9]=1;
   		pload_rsp[10]=gns_mac_securitypib.macACLEntryDescriporSetSize;
		pload_rsp[11]=0;
		plen=12;
		break;
	case 7:
		pload_rsp[9]=1;
   		pload_rsp[10]=gns_mac_securitypib.macDefaultSecurity;
		pload_rsp[11]=0;
		plen=12;
		break;	
	case 8:
		pload_rsp[9]=1;
   		pload_rsp[10]=gns_mac_securitypib.macDefautSecurityMaterialLength;
		pload_rsp[11]=0;
		plen=12;
		break;	
	case 9:
		pload_rsp[9]=1;
   		pload_rsp[10]=gns_mac_securitypib.macDefautSecuritySuite;
		pload_rsp[11]=0;
		plen=12;
		break;	
	case 10:
		pload_rsp[9]=1;
   		pload_rsp[10]=gns_mac_securitypib.macSecurityMode;
		pload_rsp[11]=0;
		plen=12;
		break;
	default:
		pload_rsp[2] = EVS_READ_NEG;
        	pload_rsp[9]=0;
   		break;	}
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_POS(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();
	
}

void SecPtclMan_Read() {
	BYTE pload_rsp[58];
	UINT8 idx,plen;
	LADDR_UNION dstADDR;//目的地址（长地址和短地址）
	BYTE dstEP;        //目的端点
	BYTE cluster;      //簇
	BYTE srcEP;      //源EP
	BYTE tsn;      //频率
	BYTE reqack;   //是否请求响应
	h2n16(macGetShortAddr(),pload_rsp);
       pload_rsp[2] = EVS_READ_POS;
       pload_rsp[3] = 0x00;
       pload_rsp[4]=0x01;
       pload_rsp[5]=0x1F;
       pload_rsp[6]=0x43;
	pload_rsp[8]=0;//保留
	idx=a_aps_rx_data.usrPload[7];
	pload_rsp[7]=idx;
	 switch (idx) {
		case 0:
			pload_rsp[9]=10;
			h2n16(gns_sec_ptcl_man.obj_id, pload_rsp+10);
			h2n16(gns_sec_ptcl_man.revision, pload_rsp + 12);
			h2n16(gns_sec_ptcl_man.time_limit, pload_rsp +14);
			pload_rsp[16] = gns_sec_ptcl_man.run_mode;
			pload_rsp[17] = gns_sec_ptcl_man.link_mode;
			pload_rsp[18] = gns_sec_ptcl_man.authen_mode;
			pload_rsp[19] = gns_sec_ptcl_man.Ptcl_type;
			pload_rsp[20] =0;
			plen=21;
			break;
		case 2:
			pload_rsp[9]=2;
			h2n16(gns_sec_ptcl_man.revision, pload_rsp+10);
			pload_rsp[12]=0;
			plen=13;
			break;
		case 3:
			pload_rsp[9]=2;
			h2n16(gns_sec_ptcl_man.time_limit, pload_rsp+10);
			pload_rsp[12]=0;
			plen=13;
			break;
		case 4:
			pload_rsp[9]=1;
			pload_rsp[10] = gns_sec_ptcl_man.run_mode;
			pload_rsp[11]=0;
			plen=12;
			break;
		case 5:
			pload_rsp[9]=1;
			pload_rsp[10] = gns_sec_ptcl_man.link_mode;
			pload_rsp[11]=0;
			plen=12;
			break;
		case 6:
			pload_rsp[9]=1;
			pload_rsp[10] = gns_sec_ptcl_man.authen_mode;
			pload_rsp[11]=0;
			plen=12;
			break;
		case 7:
			pload_rsp[9]=1;
			pload_rsp[10] = gns_sec_ptcl_man.Ptcl_type;
			pload_rsp[11]=0;
			plen=12;
			break;
		default:
			pload_rsp[2] = EVS_READ_NEG;
        		pload_rsp[9]=0;
        		break;
	}

dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_POS(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}


void	KeyMan_WRITE(BYTE *load)
{	
BYTE pload_rsp[11];
UINT8 plen;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP,cluster,srcEP,tsn,reqack;
h2n16(macGetShortAddr(),pload_rsp);
pload_rsp[2] = EVS_WRITE_POS;
pload_rsp[3] = 0x00;
pload_rsp[4]=0x01;
pload_rsp[5]=0x1F;
pload_rsp[6]=0x41;
pload_rsp[7]=load[7];
pload_rsp[8]=0;//保留
pload_rsp[9]=0;//正负响应都不带负载
pload_rsp[10]=0;//check byte
plen=11;
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
switch (pload_rsp[7]) {
 case 3:	
  	memcpy(gns_key_man.initial_key,load + 10, 16);
	break;
case 4:
	memcpy(gns_key_man.transfer_key, pload_rsp + 10, 16);
	break;
case 5:
	memcpy(gns_key_man.communicat_key, pload_rsp + 10, 16);
       break;
 case 6:
    	n2h16(&gns_key_man.key_off, pload_rsp+10);
      break;
case 7:
	n2h16(&gns_key_man.key_life,pload_rsp+10);
      break;
case 8:
       n2h16(&gns_key_man.key_len,pload_rsp+10);
	break;
default:
	pload_rsp[2] = EVS_WRITE_NEG;
       break;
	}

apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}

void	SecMeasure_WRITE(BYTE *load)
{
BYTE pload_rsp[11];
UINT8 plen;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP,cluster,srcEP,tsn,reqack;
h2n16(macGetShortAddr(),pload_rsp);
pload_rsp[2] = EVS_WRITE_POS;
pload_rsp[3] = 0x00;
pload_rsp[4]=0x01;
pload_rsp[5]=0x1F;
pload_rsp[6]=0x42;
pload_rsp[7]=load[7];
pload_rsp[8]=0;//保留
pload_rsp[9]=0;//正负响应都不带负载
pload_rsp[10]=0;//check byte
plen=11;
	switch (load[7]) {
		case 2:
			n2h16(&gns_sec_measure.obj_id, load+10);
			break;
		case 3:
			gns_sec_measure.sec_mode=load[10];
			break;
		case 4:
			gns_sec_measure.authen_mode=load[10];
			break;
		case 5:
			gns_sec_measure.encypt_mode=load[10];;
			break;
		default:
			pload_rsp[2] = EVS_READ_NEG;
        		break;
	}
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}


void SecPtclMan_WRITE(BYTE *load)
{
BYTE pload_rsp[11];
UINT8 plen;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP,cluster,srcEP,tsn,reqack;
h2n16(macGetShortAddr(),pload_rsp);
pload_rsp[2] = EVS_WRITE_POS;
pload_rsp[3] = 0x00;
pload_rsp[4]=0x01;
pload_rsp[5]=0x1F;
pload_rsp[6]=0x43;
pload_rsp[7]=load[7];
pload_rsp[8]=0;//保留
pload_rsp[9]=0;//正负响应都不带负载
pload_rsp[10]=0;//check byte
	 switch (load[7]) {
		case 3:
			n2h16(&gns_sec_ptcl_man.time_limit, load+10);
			plen=13;
			break;
		case 4:
			gns_sec_ptcl_man.run_mode=load[10];
			break;
		case 5:
			gns_sec_ptcl_man.link_mode=load[10];
			break;
		case 6:
			gns_sec_ptcl_man.authen_mode=load[10];
			break;
		case 7:
			 gns_sec_ptcl_man.Ptcl_type=load[10];
			break;
		default:
			pload_rsp[2] = EVS_WRITE_NEG;
        		break;
	}

dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
apsRxState=APS_RXSTATE_IDLE;
Read_POS(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();

}
void	AuthenObj_WRITE(BYTE *load)
{
BYTE pload_rsp[11];
UINT8 plen;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP,cluster,srcEP,tsn,reqack;
h2n16(macGetShortAddr(),pload_rsp);
pload_rsp[2] = EVS_WRITE_POS;
pload_rsp[3] = 0x00;
pload_rsp[4]=0x01;
pload_rsp[5]=0x1F;
pload_rsp[6]=0x44;
pload_rsp[7]=load[7];
pload_rsp[8]=0;//保留
pload_rsp[9]=0;//正负响应都不带负载
pload_rsp[10]=0;//check byte
plen=11;
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack = 0;
switch (load[7]) {
		case 2:
			n2h16(&gns_authen_obj.revision, load+10);
			break;
		case 3:
			memcpy(gns_authen_obj.sec_id, load+10, 16);
			break;
		case 4:
			memcpy(gns_authen_obj.sec_id+16, load+10, 16);
			break;
		case 5:
			n2h32(&gns_authen_obj.stamp.sec, load+10);
			n2h32(&gns_authen_obj.stamp.us, load+13);
			break;
		case 6:
			gns_authen_obj.state=load[10];
			break;
		default:
			pload_rsp[2] = EVS_WRITE_NEG;
        		break;
	}
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();
}
void MAC_SECURITY_PIB_WRITE(BYTE *load)
{
BYTE pload_rsp[11];
UINT8 plen,i;
LADDR_UNION dstADDR;//目的地址（长地址和短地址）
BYTE dstEP,cluster,srcEP,tsn,reqack;
h2n16(macGetShortAddr(),pload_rsp);
pload_rsp[2] = EVS_WRITE_POS;
pload_rsp[3] = 0x00;
pload_rsp[4]=0x01;
pload_rsp[5]=0x1F;
pload_rsp[6]=0x45;
pload_rsp[7]=load[7];
pload_rsp[8]=0;//保留
pload_rsp[9]=0;//正负响应都不带负载
pload_rsp[10]=0;//check byte
plen=11;
dstADDR.saddr = a_aps_rx_data.srcSADDR;
dstEP = a_aps_rx_data.srcEP;
srcEP = a_aps_rx_data.dstEP;
cluster = a_aps_rx_data.cluster;
tsn = a_aps_rx_data.tsn;
reqack=0;
switch (load[7]) {
	case 1:
		for(i=0;i<8;i++)
	{
		gns_mac_securitypib.macACLEntryDescriptorSet.ACLExtendedAddress.bytes[i]=load[i+10];
	}
		break;
	case 2:
		load[10]=gns_mac_securitypib.macACLEntryDescriptorSet.ACLShortAddress;
		break;
	case 3:
		n2h16(&gns_mac_securitypib.macACLEntryDescriptorSet.ACLPANId, load+20);
		break;
	case 4:
   		gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecurityMateriallength=load[10];
		break;
	case 5:
		gns_mac_securitypib.macACLEntryDescriptorSet.ACLSecuritySuit=load[10];
		break;
	case 6:
   		gns_mac_securitypib.macACLEntryDescriporSetSize=load[10];
		break;
	case 7:
		gns_mac_securitypib.macDefaultSecurity=load[10];
		break;	
	case 8:
		gns_mac_securitypib.macDefautSecurityMaterialLength=load[10];
		break;	
	case 9:
   		gns_mac_securitypib.macDefautSecuritySuite=load[10];
		break;	
	case 10:
   		gns_mac_securitypib.macSecurityMode=load[10];
		break;
	default:
		pload_rsp[2] = EVS_READ_NEG;
   		break;	}
apsRxState=APS_RXSTATE_IDLE;
aplSendMSG(APS_DSTMODE_SHORT, &dstADDR, dstEP, cluster,srcEP,pload_rsp, plen, tsn, reqack);
apsState=APS_STATE_IDLE;
apsSetTxIdle();

}



#endif


