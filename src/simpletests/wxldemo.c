#include "wx_lrwpan.h"



                              /*****定义变量******/
#define RX_PING_TIMEOUT     5    //5秒超时
//this is assumed to be the long address of our coordinator, in little endian order
//协调器的长地址，在小字节序顺序
LADDR_UNION dstADDR;              //测试协调器的长地址
BYTE usrTxData[85];
BYTE usrRxData[85];
BYTE usrRxFlag;
BYTE usrTxFlag;
BYTE zz;
BYTE FactDATA[80];               //存储设备的数据
BYTE count3;
BYTE count3flag;
UINT32 Sync_time;


                            /*****定义外部变量******/
extern NWK_NETWORK_DESCRIPTOR NetworkDescriptorList[16];
extern BYTE WenMaxRetry;
extern AuthenObj gns_authen_obj;
extern DevDes gdev_des;
extern UINT16 count1;
extern UINT16 count2;
extern BYTE usrReciflag;


                              /*****主函数******/
void main (void){
#ifdef LRWPAN_RFD
  BYTE i;
#endif

  zz = 0;
  count3 = 0;
  count3flag = 0;


  halInit();
  evbInit();
  aplInit();                                //初始化协议栈
  ENABLE_GLOBAL_INTERRUPT();                //允许中断
              /*****先形成一个网络，然后每隔一段时间发送一次信标帧-----FFD*****/
#ifdef LRWPAN_COORDINATOR
START_FORM_NETWORK:            //形成网络
  while(apsBusy())
    apsFSM();
  a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;
  a_nwk_service.cmd = LRWPAN_SVC_NWK_FORM_NETWORK;
  a_nwk_service.args.form_network.ScanChannels = LRWPAN_DEFAULT_CHANNEL_MASK;
  a_nwk_service.args.form_network.ScanDuration = LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION;
  a_nwk_service.args.form_network.BeaconOrder = 15;
  a_nwk_service.args.form_network.SuperframeOrder = 15;
  a_nwk_service.args.form_network.PANID = LRWPAN_DEFAULT_PANID;
  apsDoService();
  if (a_aps_service.status == LRWPAN_STATUS_SUCCESS) {
    //conPrintROMString("Network is formed , waiting for RX!\n");
    //Print6(7,3,"Network formed!",1);
  } else {
    //conPrintROMString("Network is not formed, repeat to form network!\n");
    //Print6(7,3,"Network unformed!",1);
    goto START_FORM_NETWORK;
  }
  EVB_LED2_ON();
  while(1) {
    apsFSM();

    /*                                      //Testsendtouart---FFD;
    for(i=0;i<15;i++)
 {  halRawPut(TESTUART[i]);
 }*/

 /*                                     //Test send Readrequest to RFD-----FFD
    if (mac_pib.flags.bits.macIsAssociated){
    if(count1>=10){
    usrTxtData[0]=macGetShortAddr()>>8;
    usrTxtData[1]=macGetShortAddr()&0xff;
    usrTxtData[2] = EVS_READ;
    usrTxtData[3] = 0;
    usrTxtData[4] = 1;
    usrTxtData[5] = 0x1f;
    usrTxtData[6] = 0x42;
    usrTxtData[7] = 2;
    usrTxtData[8] = 0;
    dstADDR.saddr = 0x059A;                //方便调试，地址写死
     if (mac_pib.flags.bits.macIsAssociated) {

       aplSendMSG (APS_DSTMODE_SHORT,&dstADDR,1,0,2,&usrTxtData[0],9,apsGenTSN(),FALSE); //No APS ack requested

     } count1=0;}}


*/



  if (usrRxFlag)                      //(*)如果从残功能节点RFD接受数据,那么送到gateway
    {
      LED_FLASH();
      usrSendDataToGateway();
      usrRxFlag = 0;
    }



     if ((usrReciflag==0x01)&&(count3 == 0x01))
	 	{
        usrRecedataFromGateway();
         zz=0;
         count3 = 0;
         count3flag = 0;
         usrReciflag = 0;
              }

	
  }




                              /******发现并加入网络------RFD******/

#else

START_DISCOVERY_NETWORK:
  while(apsBusy())
    apsFSM();
  a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;
  a_nwk_service.cmd = LRWPAN_SVC_NWK_DISC_NETWORK;
  a_nwk_service.args.discovery_network.request.ScanChannels = LRWPAN_DEFAULT_CHANNEL_MASK;
  a_nwk_service.args.discovery_network.request.ScanDuration = LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION;
  apsDoService();
  if (a_nwk_service.status == LRWPAN_STATUS_SUCCESS) {      //显示发现网络的结果
  } else {
    //DEBUG_STRING(DBG_INFO,"Network Discovery failed, have no beacon!\n");
    goto START_DISCOVERY_NETWORK;
  }
                                                            //加入网络
  do {
    while(apsBusy())
    apsFSM();
    a_aps_service.cmd = LRWPAN_SVC_APS_NWK_PASSTHRU;
    a_nwk_service.cmd = LRWPAN_SVC_NWK_JOIN_NETWORK;
    a_nwk_service.args.join_network.ScanChannels = LRWPAN_DEFAULT_CHANNEL_MASK;
    a_nwk_service.args.join_network.ScanDuration = LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION;
    a_nwk_service.args.join_network.PANID = LRWPAN_DEFAULT_PANID;
    a_nwk_service.args.join_network.options.bits.RejoinNetwork = 0;
#ifdef LRWPAN_RFD
    a_nwk_service.args.join_network.options.bits.JoinAsRouter = 0;
#endif
#ifdef LRWPAN_FFD
    a_nwk_service.args.join_network.options.bits.JoinAsRouter = 1;
#endif
    a_nwk_service.args.join_network.options.bits.PowerSource = 0;
    a_nwk_service.args.join_network.options.bits.RxOnWhenIdle = 1;
    a_nwk_service.args.join_network.options.bits.MACSecurity = 0;
    apsDoService();
    if (a_aps_service.status == LRWPAN_STATUS_SUCCESS) {
      EVB_LED2_ON();
      break;
    }
  } while(1);


/*
 while(1){
    halWait(200);
LED_FLASH();
    }
  */

  while(1){
     apsFSM();
#ifdef LRWPAN_RFD
     if (mac_pib.flags.bits.macIsAssociated)
     {
   // if(count1>=10) {
         usrTxData[0] = macGetShortAddr()>>8;
         usrTxData[1] = macGetShortAddr()&0xff;
         halWait(200);
	controlU();//	 检测电压并闪灯
for(i=0;i<80;i++)
FactDATA[i]=i;
         WIRELESSDB_Device(FactDATA,80);
         LED_FLASH();
 halWait(200);
		// halSleep(65000);


     // }
    }
#endif
   }//(*) 发送的数据在这里
#endif
  /*
#ifdef LRWPAN_RFD
mac_pib.flags.bits.GotBeaconResponse = 0;
  while(1) {
    apsFSM();
   if(mac_pib.flags.bits.GotBeaconResponse == 1)
    break;
     }
  DEBUG_STRING(DBG_INFO, "sync req!!\n");
  a_mac_service.cmd=LRWPAN_SVC_MAC_SYNC_REQ;
  macState =MAC_STATE_COMMAND_START;
  mac_pib.flags.bits.WaitingForSyncRep=1;
  macFSM();
  while(1){
  macFSM();
  if(mac_pib.flags.bits.WaitingForSyncRep==0){
	DEBUG_STRING(DBG_INFO, "sync success!!\n");
        break;
      }
  }

  DEBUG_STRING(DBG_INFO, "sync req!!\n");
  a_mac_service.cmd=LRWPAN_SVC_MAC_SYNC_REQ;
  macState =MAC_STATE_COMMAND_START;
  mac_pib.flags.bits.WaitingForSyncRep=1;
  macFSM();
  while(1){
  macFSM();
  if(mac_pib.flags.bits.WaitingForSyncRep==0){
	DEBUG_STRING(DBG_INFO, "sync success!!\n");
        break;
      }
  }

#endif

#ifdef LRWPAN_RFD
DEBUG_STRING(DBG_INFO, "\ngts req!!!");
aplGTSRequest(2,0,1);
L1:
mac_pib.flags.bits.GotBeaconResponse = 0;
  while(1) {
    apsFSM();
   if(mac_pib.flags.bits.GotBeaconResponse == 1)
    i++;
   if(i==10) break;
   goto L1;
     }
  aplGTSRequest(2,0,0);
  while(1){
  apsFSM();
}
#endif
*/
}


            //########## Callbacks ##########
//callback for anytime the Zero Endpoint RX handles a command
//user can use the APS functions to access the arguments
//and take additional action is desired.
//the callback occurs after the ZEP has already taken
//its action.
LRWPAN_STATUS_ENUM  usrZepRxCallback(void){

  return LRWPAN_STATUS_SUCCESS;
}

//callback from APS when packet is received
//user must do something with data as it is freed
//within the stack upon return.
LRWPAN_STATUS_ENUM  usrRxPacketCallback(void) {
  /*
  //just print out this data
  if(LcdPage == 1)
  {
    ClearScreen();
  }
#ifdef LRWPAN_COORDINATOR
  Print6(0,9,"--ZIGBEE_COORD-- ",1);
#endif
#ifdef LRWPAN_ROUTER
  Print6(0,5,"--ZIGBEE_ROUTER-- ",1);
#endif
#ifdef LRWPAN_RFD
  Print6(0,15,"--ZIGBEE_RFD--  ",1);
#endif
        conPrintROMString("User Data Packet Received: \n");
	conPrintROMString("SrcSADDR: ");
	conPrintUINT16(aplGetRxSrcSADDR());
        Print6(LcdPage,3,"SrcSADDR: ",1);
        LcdPrintUINT16(aplGetRxSrcSADDR(),LcdPage++,63);
	conPrintROMString(", DstEp: ");
	conPrintUINT8(aplGetRxDstEp());
	conPrintROMString(", Cluster: ");
	conPrintUINT8(aplGetRxCluster());
	conPrintROMString(", MsgLen: ");
	len = aplGetRxMsgLen();
	conPrintUINT8(len);
	conPrintROMString(",RSSI: ");
	conPrintUINT8(aplGetRxRSSI());
        Print6(LcdPage,3,"RSSI: ",1);
        LcdPrintUINT8(aplGetRxRSSI(),LcdPage++,39);
	conPCRLF();
	conPrintROMString("PingCnt: ");
	ptr = aplGetRxMsgData();
	ping_cnt = *ptr;
	ptr++;
	ping_cnt += ((UINT16)*ptr)<<8;
	conPrintUINT16(ping_cnt);
        Print6(LcdPage,3,"PingCnt: ",1);
        LcdPrintUINT16(ping_cnt,LcdPage++,57);
        LcdPage++;
        if(LcdPage >= 8)
        {
          LcdPage = 1;
        }
	conPrintROMString(", RxTimeouts: ");
	conPrintUINT16(numTimeouts);
	rxFlag = 1;//signal that we got a packet
	//use this source address as the next destination address
	dstADDR.saddr = aplGetRxSrcSADDR();
	conPCRLF();
	return LRWPAN_STATUS_SUCCESS;
  */
  BYTE i,len,*ptr;
  len = aplGetRxMsgLen();
  ptr = aplGetRxMsgData();
  for (i=0;i<len;i++) {
    usrRxData[i] = *ptr;
    ptr++;
  }
  usrRxFlag = 1;                                   //signal that we got a packet
  return LRWPAN_STATUS_SUCCESS;
}




