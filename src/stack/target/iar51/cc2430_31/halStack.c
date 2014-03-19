#include "hal.h"
#include "halStack.h"
#include "ieee_lrwpan_defs.h"
#include "memalloc.h"
#include "phy.h"
#include "mac.h"
#include "neighbor.h"

extern BYTE zz;
BYTE usrReciflag;
extern BYTE PCdata[4];
extern ACK_PENDING_TABLE AckPendingTable[MaxAckWindow];
RADIO_FLAGS local_radio_flags;
static BYTE PreviousSlotTimes;
static BOOL halInterruptFlag;
extern BYTE count3;
extern BYTE count3flag;
BYTE tick_10;//mini

//static BYTE CurrentSlotTimes;

/********************************************
add by weimin for count1 20071024
********************************************/
  UINT16 count1;
  UINT16  count2;
//extern UINT16  count3;
//extern UINT16  count4;
//extern UINT16  count5;
//extern UINT16  count6;	

#ifdef  LRWPAN_ASYNC_INTIO

static volatile BYTE serio_rxBuff[LRWPAN_ASYNC_RX_BUFSIZE];//Size is 32
//static volatile BYTE serio_rxHead, serio_rxTail;
BYTE serio_rxHead, serio_rxTail;
#endif


void halInitRF(void)
{
//  MDMCTRL0L = 0x10;//自动发送确认帧
}

//halInit contains both processor specific initialization
/*函数功能:主时钟源，串口，波特率,MACTimer的初始化*/
void halInit(void){
  local_radio_flags.val = 0;
  PreviousSlotTimes = 0;
  halInterruptFlag = 0;
  count1 = 0;
  //SET_MAIN_CLOCK_SOURCE(RC);
  SET_MAIN_CLOCK_SOURCE(CRYSTAL);
  halInitUart();
  halInitRF();
  halSetBaud(LRWPAN_DEFAULT_BAUDRATE);
  halInitMACTimer();
}



//initialize UART to be used by
/*函数功能:初始化串口UART0,包括管脚配置，和开中断*/
void halInitUart(void) {
   IO_PER_LOC_UART0_AT_PORT0_PIN2345();
   //U0UCR = 0x06;
   UTX0IF = 1;
#ifdef LRWPAN_ASYNC_INTIO
  serio_rxHead = 0;
  serio_rxTail = 0;
  INT_ENABLE_URX0(INT_ON);
#endif

}

#ifdef  LRWPAN_ASYNC_INTIO

//get a character from serial port, uses interrupt driven IO
/*函数功能:从串口UART0获取数据,采用中断方式*/
char halGetch(void){
    char x;
    do {
      x = serio_rxHead;
    }  while(serio_rxTail == x);
    serio_rxTail++;
   if (serio_rxTail == LRWPAN_ASYNC_RX_BUFSIZE) serio_rxTail = 0;
   return (serio_rxBuff[serio_rxTail]);
}
/*函数功能:判断缓存器是否有可读数据，TRUE 为 非空，可以取数据，FALSE 为空，不能取数据*/
BOOL halGetchRdy(void){
   char x;
   x = serio_rxHead;
   return(serio_rxTail != x);
 }

#else

//get a character from serial port
/*函数功能:从串口UART0获取数据,采用查询方式*/
char halGetch(void){
   char c;
   while (!URX0IF);
   c = U0DBUF;
   URX0IF = FALSE;
   return c;
}
/*函数功能:通过标志位判断是否有可读数据，TRUE 为 有可读数据，FALSE 为 无可读数据*/
BOOL halGetchRdy(void){
  if (URX0IF) return (1);
  else return(0);
}

#endif

/*函数功能:数据复制，目的地址dst,源地址src,长度len*/
void halUtilMemCopy(BYTE *dst, BYTE *src, BYTE len) {
  while (len) {
    *dst = *src;
    dst++;src++;
    len--;
  }
}

// assuming 16us period, have 1/16us = 62500 tics per seocnd
//6250 MACTICKs = 1 SLOWTICKS,generate an interrupt per slowtick
//#define T2CMPVAL (62500/SLOWTICKS_PER_SECOND)


//use timer2, will set it up for one tick per symbol
//assuming 2.4GHZ and a 32 MHZ clock.
// this is a 20 bit counter, will overflow in ~16 seconds
//should be long enough for MAC timeout cases
/*函数功能:设置MAC Timer(Timer 2)寄存器，T2每16us 中断一次，T2溢出计数每 100ms 中断一次,二者中断均开启*/
void halInitMACTimer(void) {

  T2CTRL = 0x00; //ensure timer is idle  //CC2530(T2CNF>T2CTRL)

/*CC2530
  T2CAPHPH = 0x02;  // setting for 16 u-second periods
  T2CAPLPL = 0x00;  // (0x0200) / 32 = 16 u-seconds
  //set the interrupt compare to its maximum value


  T2PEROF0 = 0xFF & (T2CMPVAL);
  T2PEROF1 = (BYTE) (T2CMPVAL>>8);
  //enable overflow count compare interrupt
  T2PEROF2 = ((BYTE) (T2CMPVAL>>16)) | 0x20;
*/

T2MSEL = 0x32;//select timer period and overflow compare 1

T2M1 = 0x02;
T2M0 = 0x00;// (0x0200) / 32 = 16 u-seconds

T2MOVF0 = 0xFF & (T2CMPVAL);
T2MOVF1 = (BYTE) (T2CMPVAL>>8);
T2MOVF2 = ((BYTE) (T2CMPVAL>>16)) ;

T2IRQM = 0x10;//enable overflow count compare 1  interrupt
T2IRQF = 0x00;//clear overflow count compare 1  interrupt flag

T2MSEL = 0x00;//select timer and overflow count value
//CC2530



  //turn on timer
  //configure timer
  T2CTRL = 0x01; //start timer   //CC2530(T2CNF>T2CTRL)

  INT_SETFLAG_T2(INT_CLR); //clear processor interrupt flag

  INT_ENABLE_T2(INT_ON);   //enable T2 processor interrupt


}

/*函数功能:获取MAC Timer(Timer 2)溢出计数的值，n macticks 或 n*16us*/
UINT32 halGetMACTimer(void)//n*16us=n mac ticks
{
  UINT32 t;
  BOOL gie_status;

  SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
  if (halInterruptFlag == 0)
    PreviousSlotTimes = mac_pib.macCurrentSlotTimes;
  //CC2530(T2OFx>T2MOVFx)
  t = 0x0FF & T2MOVF0;
  t += (((UINT16)T2MOVF1)<<8);
  t += (((UINT32) T2MOVF2& 0x0F)<<16);
  RESTORE_GLOBAL_INTERRUPT(gie_status);
  return (t);
}

UINT32 halMACTimerNowDelta(UINT32 previoustime)
{
  UINT32 nowtime;
  BOOL gie_status;

  SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
    //CC2530(T2OFx>T2MOVFx)
  nowtime = 0x0FF & T2MOVF0;
  nowtime += (((UINT16)T2MOVF1)<<8);
  nowtime += (((UINT32)T2MOVF2 & 0x0F)<<16);
  RESTORE_GLOBAL_INTERRUPT(gie_status);

  if (PreviousSlotTimes > mac_pib.macCurrentSlotTimes) {
    return ((mac_pib.macCurrentSlotTimes+256-PreviousSlotTimes)*T2CMPVAL+nowtime-previoustime);
  } else {
    return ((mac_pib.macCurrentSlotTimes-PreviousSlotTimes)*T2CMPVAL+nowtime-previoustime);
  }

}

UINT32 halMACTimerDelta(UINT32 preslot,UINT32 pretime)
{
  BOOL gie_status;
  UINT32 curslot,curtime;

  SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
  //CC2530(T2OFx>T2MOVFx)
  curtime = 0x0FF & T2MOVF0;
  curtime += (((UINT16)T2MOVF1)<<8);
  curtime += (((UINT32)T2MOVF2 & 0x0F)<<16);
  curslot = mac_pib.macCurrentSlotTimes;
  RESTORE_GLOBAL_INTERRUPT(gie_status);

  if (preslot == curslot) {
    return (curtime-pretime);
  } else {
    return ((curslot-preslot)*T2CMPVAL+curtime-pretime);
  }
}


//only works as long as SYMBOLS_PER_MAC_TICK is not less than 1
/*函数功能:convert MacTicks to us,1 MacTicks(Symbol)=16us*/
UINT32 halMacTicksToUs(UINT32 ticks){

   UINT32 rval;

   rval =  (ticks/SYMBOLS_PER_MAC_TICK())* (1000000/LRWPAN_SYMBOLS_PER_SECOND);
   return(rval);
}

//assumes that Timer2 has been initialized and is running
/*函数功能:产生随机数，方法不标准*/
UINT8 halGetRandomByte(void) {
  return(T2MOVF0);//CC2530(T20F0>T2MOVF0)
}

//write a character to serial port
// Uses UART initialized by halInitUart
/*函数功能:通过UART0发送一字节，先判断发送完标志，再发送*/
void halPutch(char c){
  //while (!UTX0IF);
  //UTX0IF = 0;
  //U0DBUF = c;
}

void halRawPut(char c){
  while (!UTX0IF);
  UTX0IF = 0;
  U0DBUF = c;
}





//set the radio frequency
/*函数功能:配置 Radio 工作频率和信道*/
LRWPAN_STATUS_ENUM halSetRadioIEEEFrequency(PHY_FREQ_ENUM frequency, BYTE channel)
{
   UINT16 afreq;

   if (frequency != PHY_FREQ_2405M) return(LRWPAN_STATUS_PHY_FAILED);
   if ((channel < 11) || (channel > 26)) return(LRWPAN_STATUS_PHY_FAILED);
   afreq = 11 + 5*(channel - 11);//See page181   //CC2530(357>11)
  //FSCTRLL = (BYTE) afreq;//CC2530
  //FSCTRLH = ((FSCTRLH & ~0x03) | (BYTE)((afreq >> 8) & 0x03));//CC2530
  FREQCTRL = afreq;   //CC2530
   return(LRWPAN_STATUS_SUCCESS);
}

//this assumes 2.4GHz frequency
/*函数功能:根据频段和信道，在寄存器中存频率值*/
LRWPAN_STATUS_ENUM halSetChannel(BYTE channel){
 return(halSetRadioIEEEFrequency(PHY_FREQ_2405M, channel));
}



//this uses the IEEE address stored in program memory.
//ensure that the flash programmer is configured to retain
//the IEEE address


/*为了调试加入的函数*/

#ifdef LRWPAN_RFD
void halGetProcessorIEEEAddress(BYTE *buf) {
  buf[7] = 0;
  buf[6] = 0;
  buf[5] = 0;
  buf[4] = 0;
  buf[3] = 0;
  buf[2] = 0;
  buf[1] = 1;
  buf[0] = 0;
}
#endif
#ifdef LRWPAN_COORDINATOR
void halGetProcessorIEEEAddress(BYTE *buf) {
  buf[7] = 0;
  buf[6] = 0;
  buf[5] = 0;
  buf[4] = 0;
  buf[3] = 0;
  buf[2] = 0;
  buf[1] = 0;
  buf[0] = 0;
}
#endif

#ifdef LRWPAN_ROUTER
void halGetProcessorIEEEAddress(BYTE *buf) {
  buf[7] = 1;
  buf[6] = 1;
  buf[5] = 1;
  buf[4] = 1;
  buf[3] = 1;
  buf[2] = 1;
  buf[1] = 1;
  buf[0] = 2;
}
#endif


/*函数功能:从FLASH存储器中获取 IEEE 地址*/
/*
#if defined(IAR8051)
void halGetProcessorIEEEAddress(BYTE *buf) {
#if (CC2430_FLASH_SIZE == 128)
  unsigned char bank;
  bank = MEMCTR;
  //switch to bank 3
  MEMCTR |= 0x30;
#endif
  //note that the flash programmer stores these in BIG ENDIAN order for some reason!!!
  buf[7] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+0);
  buf[6] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+1);
  buf[5] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+2);
  buf[4] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+3);
  buf[3] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+4);
  buf[2] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+5);
  buf[1] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+6);
  buf[0] = *(ROMCHAR *)(IEEE_ADDRESS_ARRAY+7);
 #if (CC2430_FLASH_SIZE == 128)
  //resore old bank settings
  MEMCTR = bank;
#endif
}

#endif


#if defined(HI_TECH_C)
//cannot figure out how to get the Hi-Tech C51 compiler to support
//banking, so can't access the IEEE address stored in memory

//use this if you don't want to use the IEEE Address stored in program memory
//by the flash memory
void halGetProcessorIEEEAddress(BYTE *buf) {
	buf[0] = aExtendedAddress_B0;
	buf[1] = aExtendedAddress_B1;
	buf[2] = aExtendedAddress_B2;
	buf[3] = aExtendedAddress_B3;
	buf[4] = aExtendedAddress_B4;
	buf[5] = aExtendedAddress_B5;
	buf[6] = aExtendedAddress_B6;
	buf[7] = aExtendedAddress_B7;
}
#endif
*/



/*函数功能:从FLASH存储器中获取 IEEE 地址，并写到寄存器IEEE_ADDR*/
void halSetRadioIEEEAddress(void) {
  BYTE buf[8];
  halGetProcessorIEEEAddress(buf);
  EXT_ADDR0 = buf[0];
  EXT_ADDR1 = buf[1];
  EXT_ADDR2 = buf[2];
  EXT_ADDR3 = buf[3];
  EXT_ADDR4 = buf[4];
  EXT_ADDR5 = buf[5];
  EXT_ADDR6 = buf[6];
  EXT_ADDR7 = buf[7];
}
//CC2530(IEEE_ADDR>EXT_ADDR)

/*函数功能:将 PANID 写到寄存器PANID*/
void halSetRadioPANID(UINT16 panid){
  PAN_ID0 = (BYTE) (panid);
  PAN_ID1 = (BYTE) (panid>>8);
 }
//CC2530(PANIDL>PAN_ID0;PANIDL>PAN_ID1)

/*函数功能:将 短地址 写到寄存器 SHORTADDR*/
void halSetRadioShortAddr(SADDR saddr){
  SHORT_ADDR0 = (BYTE) (saddr);
  SHORT_ADDR1 = (BYTE) (saddr>>8);
}
//CC2530(SHORTADDRL>SHORT_ADDR0;SHORTADDRL>SHORT_ADDR1)



//return value of non-zero indicates failure
//Turn on Radio, initialize for RF mode
//assumes that auto-ack is enabled
//this function based on sppInit example from ChipCon
//also sets the IEEE address
//if listen_flag is true, then radio is configured for
//listen only (no auto-ack, no address recognition)

/*
Eventually, change this so that auto-ack can be configured as
on or off. When Coordinator is trying to start a network,
auto-ack, addr decoding will be off as Coordinator will be doing an energy
scan and detecting PAN collisions, and thus will not be doing
any acking of packets. After the network is started, then
will enable auto-acking.
Routers and End devices will always auto-ack

i
*/


/*函数功能:初始化RF,设置 RF 工作频率，根据radio_flags设置相关参数,中断设置*/
LRWPAN_STATUS_ENUM halInitRadio(PHY_FREQ_ENUM frequency, BYTE channel, RADIO_FLAGS radio_flags)
{
  LRWPAN_STATUS_ENUM status;


  // Setting the frequency
  status = halSetRadioIEEEFrequency(frequency, channel);
  if (status != LRWPAN_STATUS_SUCCESS) return(status);

    //RFPWR = 0x04;//turning on power to analog part of radio and waiting 100 us for voltage regulator.//CC2530
  //while((RFPWR & 0x10)){}//waiting until all analog modules in the radio are set in power up//CC2530

  if (radio_flags.bits.listen_mode) {//radio_flags.listen_mode=1;
    //corresponds to promiscuous modes
    //radio accepts all packets, the HUSSY!
    FRMFILT0 &= ~ADR_DECODE;    //no address decode
    FRMCTRL0 &= ~AUTO_ACK;       //no auto ack
  } else {
    FRMFILT0 |= ADR_DECODE;// Turning on Address Decoding
    //FRMCTRL0 |= AUTO_ACK;//enable auto_ack
    FRMCTRL0 &= ~AUTO_ACK;       //no auto ack
  }
  //CC2530(MDMCTRL0H>FRMFILT0;MDMCTRL0L>FRMCTRL0)

	
  local_radio_flags = radio_flags;  //save this for later

  FRMCTRL0 |= AUTO_CRC;// Setting for AUTO CRC  //CC2530(MDMCTRL0L>FRMCTRL0)


  //pan
  if (radio_flags.bits.pan_coordinator) {
    FRMFILT0 |= PAN_COORDINATOR;  //accepts frames with only source addressing modes
  } else {
    FRMFILT0 &= ~PAN_COORDINATOR;  //rejects frames with only source addressing modes
  }
  //CC2530(MDMCTRL0H>FRMFILT0)


//CC2530
/*
  // Turning on AUTO_TX2RX,ACK packets are received
  FSMTC1 = ((FSMTC1 & (~AUTO_TX2RX_OFF & ~RX2RX_TIME_OFF))  | ACCEPT_ACKPKT);

  // Turning off abortRxOnSrxon.Packet reception is not aborted when SRXON is issued
  FSMTC1 &= ~0x20;
*/
//CC2530

  //now configure the RX, TX systems.
  // Setting the number of bytes to assert the FIFOP flag
  FIFOPCTRL = 127;  //set to max value as the FIFOP flag goes high when complete packet received    //CC2530(IOCFG0>FIFOPCTRL)

  // Flushing both Tx and Rx FiFo. The flush-Rx is issued twice to reset the SFD.
  // Calibrating the radio and turning on Rx to evaluate the CCA.
  SRXON;		//Enable and calibrate frequency synthesizer for RX
  SFLUSHTX;		//Flush TXFIFO buffer
  SFLUSHRX;		//Flush RXFIFO buffer and reset demodulator
  SFLUSHRX;		//The flush-Rx is issued twice to reset the SFD
  //STXCALN;		//Enable and calibrate frequency synthesizer for TX  //CC2530
  ISSTART;		//The ISSTART instruction starts the CSP program execution from first instruction written to instruction memory.

//this controls the frame pending bit in ACK frames.
//because of auto-ack, we will not have time to determine if data
//is actually pending or not.
#if defined(LWRPAN_RFD)
  SACK;  //Send acknowledge frame with pending field cleared
  		//RFDs never have data pending for FFDs
#else
  SACKPEND;  //Send acknowledge frame with pending field set.   routers
#endif

  halSetRadioIEEEAddress();

   //Radio can interrupt when
   //RX configuration
   //clear flags/mask in radio
   RFIRQF0 = 0;
   RFIRQF1 = 0;
   //CC2530(RFIF>RFIRQF0,RFIRQF1)

   RFIRQM0 = 0;   //all interrupts are masked.
   RFIRQM1 = 0;
   //CC2530(RFIM>RFIRQM0,RFIRQM1)


   //enable RF general interrupt on processor
   INT_SETFLAG_RF(INT_CLR);
   INT_ENABLE_RF(INT_ON);

   //enable RF TX/RX FIFO(RFERR) interrupt on processor
   INT_SETFLAG_RFERR(INT_CLR);
   INT_ENABLE_RFERR(INT_ON);

   //do not use DMA at this point
   //enable the RX receive interrupt here.
   RFIRQM0 |= IRQ_FIFOP;    //CC2530(RFIM>RFIRQM0)

   return(LRWPAN_STATUS_SUCCESS);
}

#define PIN_CCA   CCA    //CCA is defined in hal.h

//regardless of what happens here, we will try TXONCCA after this returns.
/*函数功能:CSMA_CA中，退避算法*/
void  doIEEE_backoff(void) {
     BYTE be, nb, tmp, rannum;
    UINT32  delay, start_tick;

   be = aMinBE;
   nb = 0;
  do {
      if (be) {
        //do random delay
        tmp = be;
        //compute new delay
        delay = 1;
        while (tmp) {
          delay = delay << 1;  //delay = 2**be;
           tmp--;
         }
        rannum =  halGetRandomByte() & (delay-1); //rannum will be between 0 and delay-1
        delay = 0;
        while (rannum) {
            delay  += SYMBOLS_TO_MACTICKS(aUnitBackoffPeriod);
            rannum--;
         }//delay = aUnitBackoff * rannum
        //now do backoff
       start_tick = halGetMACTimer();
        while (halMACTimerNowDelta(start_tick) < delay);
      }
      //check CCA
      if (PIN_CCA)  break;
      nb++;
      be++;
      if (be > aMaxBE) be =aMaxBE;
   }while (nb <= macMaxCSMABackoffs);
  return;
}

//transmit packet
//hdrlen - header lenth
//hdr - pointer to header data
//plen - payload length
//pload - pointer to payload
/*函数功能:在芯片底层发送数据，实现过程不清楚*/
LRWPAN_STATUS_ENUM halSendPacket(BYTE flen, BYTE *frm)
{
  BYTE len;
  //BYTE *frm;
  LRWPAN_STATUS_ENUM res;

  if( mac_pib.flags.bits.TransmittingSyncRep==1){
    mac_sync_time.TM2=halGetMACTimer();
    mac_sync_time.TM2_1=mac_pib.macCurrentSlotTimes;
    phy_pib.currentTxFrm = &tmpTxBuff[LRWPAN_MAX_FRAME_SIZE];
    //DEBUG_STRING(DBG_INFO,"adding TM2!");
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM2);
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM2>>8);
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM2>>16);
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm =mac_sync_time.TM2_1;
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm = (BYTE)(mac_sync_time.TM2_1>>8);
    phy_pib.currentTxFrm--;
    *phy_pib.currentTxFrm =(BYTE)(mac_sync_time.TM2_1>>16);
    flen=flen+6;
    mac_pib.flags.bits.TransmittingSyncRep=0;
  }

  if(mac_pib.flags.bits.TransmittingSyncReq==1){
    mac_sync_time.TS1=halGetMACTimer();
    mac_sync_time.TS1_1=mac_pib.macCurrentSlotTimes;
    mac_pib.flags.bits.TransmittingSyncReq=0;
  }

  //conPrintROMString("hal send packet ,DSN: ");
  //conPrintUINT8(mac_pib.macDSN);
  //conPrintROMString("\n");

  //frm = ptr;
  //ptr--;
  //*ptr = flen+1;
  len = flen + PACKET_FOOTER_SIZE;//PACKET_FOOTER_SIZE is 2

  //DEBUG_STRING(DBG_INFO, "TX PKT Size: ");
  //DEBUG_UINT8(DBG_INFO,len);
  //DEBUG_STRING(DBG_INFO,"\n");
  if (len > 127) {    //packet size is too large!
    return(LRWPAN_STATUS_PHY_TX_PKT_TOO_BIG);
  }

  // Clearing RF interrupt flags and enabling RF interrupts.
  if((FSMSTAT0&0x3f) == 6 && RXFIFOCNT > 0)     //CC2530(FSMSTATE>(FSMSTAT0&0x3f))
  {
    ISFLUSHRX;
    ISFLUSHRX;
  }

  //清中断标志
  RFIRQF1 &= ~IRQ_TXDONE;      //Clear the RF TXDONE flag，发送完成中断标志           //CC2530(RFIF>RFIRQF1)
  INT_SETFLAG_RF(INT_CLR);  //Clear processor interrupt flag，RF总中断标志

  //write packet length
  RFD = len;
  /*if(mac_pib.flags.bits.TransmittingGTSReq==1)
  	{
  	while (flen) {
		DEBUG_UINT8(DBG_INFO, *frm);
  		DEBUG_STRING(DBG_INFO,"\n");
		frm++; flen--;
		}
  	}*/
  //write the packet. Use 'flen' as the last two bytes are added automatically
  //At some point, change this to use DMA
  while (flen) {RFD = *frm; frm++; flen--;}//以后可用DMA实现

  // If the RSSI value is not valid, enable receiver
  if(RSSI == 0x80)      //CC2530(RSSIL>RSSI)
  {
  	//The ISRXON instruction immediately enables and calibrates frequency synthesizer for RX.
    ISRXON;
    //waiting 320u-sec to make the RSSI value become valid.
    halWait(1);
  }

  doIEEE_backoff();
  //The ISTXONCCA instruction immediately enables TX after calibration if CCA indicates a clear channel.
  ISTXONCCA;       //TODO: replace this with IEEE Backoff

  if((FSMSTAT0&0x3f) > 30)  //is TX active?       //CC2530(FSMSTATE>(FSMSTAT0&0x3f))
  {
    // Asserting the status flag and enabling ACK reception if expected.
    AckPendingTable[mac_pib.macDSNIndex].TxStartTime = halGetMACTimer();
    AckPendingTable[mac_pib.macDSNIndex].currentAckRetries--;
    phy_pib.txStartTime = AckPendingTable[mac_pib.macDSNIndex].TxStartTime ;
    //phyTxStartCallBack(ptr);
    res = LRWPAN_STATUS_SUCCESS;
    RFIRQM1 |= IRQ_TXDONE;             //enable IRQ_TXDONE interrupt      //CC2530(RFIM>RFIRQM1)
    //DEBUG_STRING(DBG_TX,"RF work well, Tx send Packet success!\n");
    //DEBUG_CHAR(DBG_TX, DBG_CHAR_TXSTART);
  }
  else
  {
    ISFLUSHTX;               //empty buffer
    res = LRWPAN_STATUS_PHY_CHANNEL_BUSY;
    RFIRQM1 &= ~IRQ_TXDONE;     //mask interrupt      //CC2530(RFIM>RFIRQM1)
   // DEBUG_STRING(DBG_TX,"RF work bad, Tx send Packet failure!\n");
   //DEBUG_CHAR(DBG_TX, DBG_CHAR_TXBUSY);
  }
  return(res);
}


/*函数功能:在ACK中超时后，重发数据，在芯片底层发送数据，实现过程不清楚*/
void halRepeatSendPacket(BYTE DsnIndex)
{


  BYTE flen,len;
  BYTE *frm;
  //LRWPAN_STATUS_ENUM res;


  //conPrintROMString("hal repeat send packet ,DSN: ");
  //conPrintUINT8(AckPendingTable[DsnIndex].DSN);
  //conPrintROMString("\n");

  flen = *AckPendingTable[DsnIndex].Ptr-1;
  frm = AckPendingTable[DsnIndex].Ptr+1;

  len = flen + PACKET_FOOTER_SIZE;//PACKET_FOOTER_SIZE is 2

  //DEBUG_STRING(DBG_INFO, "TX PKT Size: ");
  //DEBUG_UINT8(DBG_INFO,len);
  //DEBUG_STRING(DBG_INFO,"\n");

  if (len > 127) { //packet size is too large!
    return;
  }

  // Clearing RF interrupt flags and enabling RF interrupts.
  if((FSMSTAT0&0x3f) == 6 && RXFIFOCNT > 0)      //CC2530(FSMSTATE>(FSMSTAT0&0x3f))
  {
    ISFLUSHRX;
    ISFLUSHRX;
  }

  RFIRQF1 &= ~IRQ_TXDONE;      //Clear the RF TXDONE flag，发送完成中断标志      //CC2530(RFIF>RFIRQF1)
  INT_SETFLAG_RF(INT_CLR);  //Clear processor interrupt flag，RF总中断标志

  //write packet length
  RFD = len;
  //write the packet. Use 'flen' as the last two bytes are added automatically
  //At some point, change this to use DMA
  while (flen) {RFD = *frm; frm++; flen--;}//以后可用DMA实现

  // If the RSSI value is not valid, enable receiver
  if(RSSI == 0x80)       //CC2530(RSSIL>RSSI)
  {
  	//The ISRXON instruction immediately enables and calibrates frequency synthesizer for RX.
    ISRXON;
    //waiting 320u-sec to make the RSSI value become valid.
    halWait(1);
  }

  doIEEE_backoff();
  //The ISTXONCCA instruction immediately enables TX after calibration if CCA indicates a clear channel.
  ISTXONCCA;       //TODO: replace this with IEEE Backoff

  if((FSMSTAT0&0x3f) > 30)  //is TX active?      //CC2530(FSMSTATE>(FSMSTAT0&0x3f))
  {
    // Asserting the status flag and enabling ACK reception if expected.
    //phyTxStartCallBack(ptr);
    AckPendingTable[DsnIndex].TxStartTime = halGetMACTimer();
    AckPendingTable[DsnIndex].currentAckRetries--;
    phy_pib.txStartTime = AckPendingTable[DsnIndex].TxStartTime ;
    //DEBUG_STRING(DBG_TX,"RF work well, Tx repeat send Packet success!\n");
    RFIRQM1 |= IRQ_TXDONE;//enable IRQ_TXDONE interrupt      //CC2530(RFIM>RFIRQM1)
  }
  else
  {
    ISFLUSHTX;               //empty buffer
    RFIRQM1 &= ~IRQ_TXDONE;     //mask interrupt      //CC2530(RFIM>RFIRQM1)
    //DEBUG_STRING(DBG_TX,"RF work badl, Tx send Packet failure!\n");
  }

}

/*函数功能：本设备接收到请求确认帧的帧时，发送确认帧, 目前是自动回复确认帧。确认帧可以自己定义，用于同步，向SP100。11a发胀*/
/*
void halSendAckFrame(BYTE DSN)
{
  BYTE fcflsb,fcfmsb;

  conPrintROMString("hal send ack frame, DSN: ");
  conPrintUINT8(DSN);
  conPrintROMString("\n");

  fcflsb = LRWPAN_FRAME_TYPE_ACK;//如果本设备有未处理数据要发送，可以置未处理域为1
  fcfmsb = 0x00;

  // Clearing RF interrupt flags and enabling RF interrupts.
  if(FSMSTATE == 6 && RXFIFOCNT > 0)
  {
    ISFLUSHRX;
    ISFLUSHRX;
  }

  RFIF &= ~IRQ_TXDONE;      //Clear the RF TXDONE flag，发送完成中断标志
  INT_SETFLAG_RF(INT_CLR);  //Clear processor interrupt flag，RF总中断标志

  //write packet length
  RFD = LRWPAN_ACKFRAME_LENGTH;
  RFD = fcflsb;
  RFD = fcfmsb;


  // If the RSSI value is not valid, enable receiver
  if(RSSIL == 0x80)
  {
  	//The ISRXON instruction immediately enables and calibrates frequency synthesizer for RX.
    ISRXON;
    //waiting 320u-sec to make the RSSI value become valid.
    halWait(1);
  }

  doIEEE_backoff();
  //The ISTXONCCA instruction immediately enables TX after calibration if CCA indicates a clear channel.
  ISTXONCCA;       //TODO: replace this with IEEE Backoff

  if(FSMSTATE > 30)  //is TX active
  {
    conPrintROMString("hal send ack frame successly!\n");
    //RFIM |= IRQ_TXDONE;
    RFIM &= ~IRQ_TXDONE;     //mask interrupt

  }
  else
  {
    ISFLUSHTX;               //empty buffer

    RFIM &= ~IRQ_TXDONE;     //mask interrupt

  }

  RFIF &= ~IRQ_TXDONE;      //Clear the RF TXDONE flag，发送完成中断标志
  INT_SETFLAG_RF(INT_CLR);  //Clear processor interrupt flag，RF总中断标志

}

*/

#ifdef  LRWPAN_ASYNC_INTIO

#if defined(IAR8051)
#pragma vector=URX0_VECTOR
#endif
#if defined(HI_TECH_C)
ROM_VECTOR(URX0_VECTOR,urx0_service_IRQ);
#endif

/*中断函数功能:URX0中断服务，从U0DBUF中读取数据，存在serio_rxBuff[]*/
/*See Page52,Interrupt 2, USART0 RX complete*/
INTERRUPT_FUNC urx0_service_IRQ(void){
   BYTE y;
   HAL_ENTER_INTERRUPT();
   y = U0DBUF;
   PCdata[zz]=y;
   usrReciflag = 1;
   count3flag = 1;
   zz++;

   HAL_EXIT_INTERRUPT();
}

#endif



//This timer interrupt is the periodic interrupt for
//evboard functions

#ifdef LRWPAN_ENABLE_SLOW_TIMER

#if defined(IAR8051)
#pragma vector=T2_VECTOR
#endif
#if defined(HI_TECH_C)
ROM_VECTOR(T2_VECTOR,t2_service_IRQ);
#endif
/*中断函数功能:T2中断服务，首先关总中断，清T2IF标志，确定下次溢出计数比较中断时间*/
/*             更新开关状态的共用体，开总中断*/
/*See Page52,Interrupt 10, Timer 2 (overflow count compare interrupt)*/
INTERRUPT_FUNC t2_service_IRQ(void){

  HAL_ENTER_INTERRUPT();
  INT_GLOBAL_ENABLE(INT_OFF);
  INT_SETFLAG_T2(INT_CLR); //clear processor interrupt flag
  T2IRQF=0x00;//clear overflow count compare 1  interrupt flag  //CC2530
//CC2530(T2OFx>T2MOVFx)
  T2MOVF0=0x00;
  T2MOVF1=0x00;
  T2MOVF2=0x00;
//mini
  tick_10++;
  mac_pib.macCurrentSlotTimes++;
  if(tick_10==10)
  	{tick_10=0;
//mini
//mini//mac_pib.macCurrentSlotTimes++;
  count1++;
  count2++;//add by weimin for authen;
  if(count3flag == 0x01)count3 = 0x01;
  	}//mini
/*
  if (mac_pib.flags.bits.macIsAssociated) {
    mac_pib.macCurrentSlot++;
    if(mac_pib.macCurrentSlot>=16) {
#ifdef LRWPAN_COORDINATOR
  if (phyTxUnLocked()) {
  	phyGrabTxLock();
       macSendBeacon();
    }
#endif
      mac_pib.macCurrentSlot = 0;
    }
  } else
    mac_pib.macCurrentSlot = 0;
*/
  T2CTRL = 0x01;  //CC2530(T2CNF>T2CTRL)
  INT_GLOBAL_ENABLE(INT_ON);
  HAL_EXIT_INTERRUPT();
}
#endif




//interrupt for RF error
//this interrupt is same priority as FIFOP interrupt,
//but is polled first, so will occur first.

#if defined(IAR8051)
#pragma vector=RFERR_VECTOR
#endif
#if defined(HI_TECH_C)
ROM_VECTOR(RFERR_VECTOR,rf_error_IRQ);
#endif
/*中断函数功能:RFERR中断服务，首先关总中断，判断是RF TX FIFO underflow 还是*/
/*             RX FIFO overflow 中断，并进行相应处理，开总中断*/
/*See Page52,Interrupt 0, RF TX FIFO underflow and RX FIFO overflow */
INTERRUPT_FUNC rf_error_IRQ(void)
{
  HAL_ENTER_INTERRUPT();
   INT_GLOBAL_ENABLE(INT_OFF);
   // If Rx overflow occurs, the Rx FiFo is reset.
   // The Rx DMA is reset and reception is started over.
   if((FSMSTAT0&0x3f) == 17)        //CC2530(FSMSTATE>(FSMSTAT0&0x3f))
   {
      //DEBUG_CHAR( DBG_ITRACE,DBG_CHAR_TXBUSY);
      STOP_RADIO();//Disable RX/TX and frequency synthesizer
      ISFLUSHRX;//Flush RXFIFO buffer and reset demodulator
      ISFLUSHRX;
      ISRXON;//Enable and calibrate frequency synthesizer for RX
   }
   else if((FSMSTAT0&0x3f) == 56)      //CC2530(FSMSTATE>(FSMSTAT0&0x3f))
   {
      //DEBUG_CHAR( DBG_ITRACE,DBG_CHAR_RXOFLOW);
      ISFLUSHTX;//Flush TXFIFO buffer
   }

   INT_SETFLAG_RFERR(INT_CLR);
   INT_GLOBAL_ENABLE(INT_ON);
   HAL_EXIT_INTERRUPT();
}


//This interrupt used for both TX and RX
#if defined(IAR8051)
#pragma vector=RF_VECTOR
#endif
#if defined(HI_TECH_C)
ROM_VECTOR(RF_VECTOR,spp_rf_IRQ);
#endif
/*中断函数功能:RF中断服务，首先关总中断，清分中断标志和RFIF标志*/
/*             判断中断是IRQ_FIFOP还是IRQ_TXDONE中断，并进行相应处理，开总中断*/
/*See Page52,Interrupt 16, RF general interrupts(IRQ_FIFOP and IRQ_TXDONE) */
INTERRUPT_FUNC spp_rf_IRQ(void)
{
  BYTE flen;
  BYTE enabledAndActiveInterrupt1,enabledAndActiveInterrupt2;
  BYTE *ptr, *rx_frame;
  BYTE ack_bytes[5];
  BYTE crc;

  //define alternate names for readability in this function
#define  fcflsb ack_bytes[0]
#define  fcfmsb  ack_bytes[1]
#define  dstmode ack_bytes[2]
#define  srcmode ack_bytes[3]

  HAL_ENTER_INTERRUPT();
  INT_GLOBAL_ENABLE(INT_OFF);
  enabledAndActiveInterrupt1 = RFIRQF0;      //CC2530(RFIF>RFIRQF0)
  enabledAndActiveInterrupt2 = RFIRQF1;
  RFIRQF0 = 0x00; RFIRQF1=0x00;                       // Clear all radio interrupt flags         //CC2530(RFIF>RFIRQF0,RFIRQF1)
  INT_SETFLAG_RF(INT_CLR);    // Clear MCU interrupt flag,S1CON.RFIF
  enabledAndActiveInterrupt1 &= RFIRQM0;     //CC2530(RFIM>RFIRQM0)
  enabledAndActiveInterrupt2 &= RFIRQM1;

  if(enabledAndActiveInterrupt1 & IRQ_FIFOP)// complete frame has arrived
  {
    //DEBUG_CHAR( DBG_ITRACE,DBG_CHAR_RXRCV );
    //if last packet has not been processed, we just read it but ignore it.
    ptr = NULL; //temporary pointer
    flen = RFD & 0x7f;  //read the length
    if (flen == LRWPAN_ACKFRAME_LENGTH) {//this should be an ACK.
      //read the packet, do not allocate space for it
      //DEBUG_CHAR( DBG_ITRACE,DBG_CHAR_ACKPKT );
      ack_bytes[0]= flen;
      ack_bytes[1] =  RFD;  //LSB Frame Control Field
      ack_bytes[2] = RFD;   //MSB Frame Control Field
      ack_bytes[3] = RFD;   //dsn
      ack_bytes[4] = RFD;   //RSSI
      crc = RFD;
      if (crc & 0x80){      //check CRC
        // CRC ok, perform callback if this is an ACK
        macRxCallback(ack_bytes, ack_bytes[4], (crc&0x7F));
      }

    }else {
      //not an ack packet, lets do some more early rejection
      // that the CC2430 seems to not do that we want to do.
      //read the fcflsb, fcfmsb
      fcflsb = RFD;
      fcfmsb = RFD;
      if (!local_radio_flags.bits.listen_mode) {
        //only reject if not in listen mode
        srcmode = LRWPAN_GET_SRC_ADDR(fcfmsb);
        dstmode = LRWPAN_GET_DST_ADDR(fcfmsb); //get the src, dst addressing modes
        if ((srcmode == LRWPAN_ADDRMODE_NOADDR) && (dstmode == LRWPAN_ADDRMODE_NOADDR)) {
          //reject this packet, no addressing information
          goto do_rxflush;
        }
      }

      if (!macRxBuffFull()) {
        //MAC TX buffer has room,  allocate new memory space,   read the length
        rx_frame = MemAlloc(flen+1);//长度字节( 1 )  +   数据长度( flen )
        ptr = rx_frame;
      } else {
        //MAC RX buffer is full
        //DEBUG_CHAR( DBG_ITRACE,DBG_CHAR_MACFULL );
      }

      // at this point, if ptr is null, then either
      // the MAC RX buffer is full or there is  no
      // free memory for the new frame, or the packet is
      // going to be rejected because of addressing info.
      // In these cases, we need to
      // throw the RX packet away
      if (ptr == NULL) {
        //just flush the bytes
        goto do_rxflush;
      }else {
        mac_sync_time.ReceiveTime=halGetMACTimer();
        mac_sync_time.ReceiveTime_1=mac_pib.macCurrentSlotTimes;
        //save packet, including the length
        *ptr = flen; ptr++;
        //save the fcflsb, fcfmsb bytes
        *ptr = fcflsb; ptr++; flen--;
        *ptr = fcfmsb; ptr++; flen--;
        while (flen) { *ptr = RFD;  flen--; ptr++; }//get the rest of the bytes
        //do RX callback

        if (*(ptr-1) & 0x80) {//check the CRC
          //CRC good
          //change the RSSI byte from 2's complement to unsigned number
          //下面是转换RSSI值的最简单方法
          //*(ptr-2) = *(ptr-2) + 0x80;
          phyRxCallback();
          macRxCallback(rx_frame, *(ptr-2),(*(ptr-1)&0x7F));
        }else {
          // CRC bad. Free the packet
          MemFree(rx_frame);
        }
      }
    }

    //flush any remaining bytes
  do_rxflush:
      ISFLUSHRX;
      ISFLUSHRX;

    //don't know why, but the RF flags have to be cleared AFTER a read is done.
    RFIRQF0 = 0x00;RFIRQF1=0x00;      //CC2530(RFIF>RFIRQF0,RFIRQF1)
    INT_SETFLAG_RF(INT_CLR);    // Clear MCU interrupt flag
    //don't know why, but the interrupt mask has to be set again here for some reason.
    //the processor receives packets, but does not generate an interrupt
    RFIRQM0 |= IRQ_FIFOP;     //CC2530(RFIM>RFIRQM0)
  }//end receive interrupt (FIFOP)

  // Transmission of a packet is finished. Enabling reception of ACK if required.
  if(enabledAndActiveInterrupt2 & IRQ_TXDONE)
  {
    //Finished TX, do call back
    //DEBUG_CHAR( DBG_ITRACE,DBG_CHAR_TXFIN );
    phyTxEndCallBack();
    //macTxCallback();
    // Clearing the tx done interrupt enable
    RFIRQM1 &= ~IRQ_TXDONE;//关闭发送结束中断      //CC2530(RFIM>RFIRQM1)

  }
  usrIntCallback();//nop
  INT_GLOBAL_ENABLE(INT_ON);
  HAL_EXIT_INTERRUPT();

#undef  fcflsb
#undef  fcfmsb
#undef  dstmode
#undef  srcmode
}



//software delay, waits is in milliseconds
void halWait(BYTE wait){
   UINT32 largeWait;

   if(wait == 0)
   {return;}
   largeWait = ((UINT16) (wait << 7));
   largeWait += 114*wait;


   largeWait = (largeWait >> CLKSPD);
   while(largeWait--);

   return;
}


void halWait111(BYTE wait){
   UINT32 largeWait;

   if(wait == 0)
   {return;}
   largeWait = ((UINT16) (wait << 0));
   largeWait += 2*wait;


   largeWait = (largeWait >> CLKSPD);
   while(largeWait--);

   return;
}

void halWaitMs(const UINT32 msecs){
  UINT32 towait;

  towait = msecs;
  while (towait > 100) {
    halWait(100);
    towait -= 100;
  }
  halWait(towait);
}

/*函数功能:关T2中断，RF中断，RFERR中断，关所有 radio 部分的电源*/
void halShutdown(void) {
  //disable some interrupts
  #ifdef LRWPAN_ENABLE_SLOW_TIMER
  INT_ENABLE_T2(INT_OFF);//disable T2 interrupt
  #endif
  //disable RADIO interrupts
  //Radio RF interrupt
  INT_ENABLE_RF(INT_OFF);
  //enable RX RFERR interrupt on processor
  INT_ENABLE_RFERR(INT_OFF);
  //shutoff the analog power to the radio
  //RFPWR = RFPWR | (1<<3);    //RFPWR.RREG_RADIO_PD = 1;     CC2530

}

/*函数功能:开T2中断，和 radio 电源，并等待电源稳定*/
void halWarmstart(void) {
  UINT32 myticks;
   //re-enable the timer interrupt
  #ifdef LRWPAN_ENABLE_SLOW_TIMER
  INT_ENABLE_T2(INT_ON);
  #endif
  //turn on the radio again
  //RFPWR = RFPWR & ~(1<<3);    //RFPWR.RREG_RADIO_PD = 0;Power up     CC2530
  //wait for power to stabilize
  myticks = halGetMACTimer();
  while (halMACTimerNowDelta(myticks) < MSECS_TO_MACTICKS(10)) {
    //check the power up bit, max time is supposed to be 2 ms
   // if (!(RFPWR & ~(1<<4))) CC2530
		break;
  }
 }



//this is provided as an example
//will go into power mode 1, and use the sleep
//timer to wake up.
//Caution: Use of power down mode 2 caused erratic
//results after power up, I am not sure if was due
//to parts of RAM not retaining their value or what.
/*函数功能:进入POWER_MODE_1(32.768 KHz oscillator on, voltage regulator on),休眠 n ms*/
/*
void halSleep(UINT32 msecs) {
  UINT32 t;
  UINT32 delta;
  BOOL gie_status;

  SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
  delta = (32768 * msecs)/1000; //read the sleep timer
  t = 0xFF & ST0;
  t += (((UINT16)ST1)<<8);
  t += (((UINT32) ST2 & 0xFF)<<16);
  t = (t + delta)&0x00FFFFFF;  //compute the compare value
  ST2 = (t >> 16)&0xFF;  //write the new sleep timer value
  ST1 = (t >> 8)&0xFF;
  ST0 = t & 0xFF;
  //clear the sleep flag, enable the interrupt
  IRCON = IRCON & 0x7F; //clear the sleep flag IRCON.STIF = 0;
  IEN0 = IEN0 | (1<<5); //enable the interrupt  IEN0.STIE = 1;

  ENABLE_GLOBAL_INTERRUPT();  //interrupts must be enabled to wakeup!
  //SET_POWER_MODE(POWER_MODE_2);
  SET_POWER_MODE(POWER_MODE_1);  //configure the power mode and sleep
    //wake up!
  DISABLE_GLOBAL_INTERRUPT();
  IEN0 = IEN0 & ~(1<<5);  // IEN0.STIE = 0;disable sleep interrupt
  while(!XOSC_STABLE);//等待时钟源稳定
  asm("NOP");
  RESTORE_GLOBAL_INTERRUPT(gie_status);
};
*/


/*函数功能:获取AD采样结果*/
INT16 halGetAdcValue(){
   INT16 value;
   value = ((INT16)ADCH) << 8;
   value |= (INT16)ADCL;
   return value;
}


//functions used by EVboard.

//-----------------------------------------------------------------------------
// See hal.h for a description of this function.
//-----------------------------------------------------------------------------
/*函数功能:使AD采样一次，返回结果，相关参数说明如下*/
/*
Reference voltage:
#define ADC_REF_1_25_V      0x00     // Internal 1.25V reference
#define ADC_REF_P0_7        0x40     // External reference on AIN7 pin
#define ADC_REF_AVDD        0x80     // AVDD_SOC pin
#define ADC_REF_P0_6_P0_7   0xC0     // External reference on AIN6-AIN7 differential input

Resolution (decimation rate):
#define ADC_8_BIT           0x00     //  64 decimation rate(8 bits resolution)
#define ADC_10_BIT          0x10     // 128 decimation rate(10 bits resolution)
#define ADC_12_BIT          0x20     // 256 decimation rate(12 bits resolution)
#define ADC_14_BIT          0x30     // 512 decimation rate(14 bits resolution)

Input channel:
#define ADC_AIN0            0x00     // single ended P0_0,AIN0
#define ADC_AIN1            0x01     // single ended P0_1,AIN1
#define ADC_AIN2            0x02     // single ended P0_2,AIN2
#define ADC_AIN3            0x03     // single ended P0_3,AIN3
#define ADC_AIN4            0x04     // single ended P0_4,AIN4
#define ADC_AIN5            0x05     // single ended P0_5,AIN5
#define ADC_AIN6            0x06     // single ended P0_6,AIN6
#define ADC_AIN7            0x07     // single ended P0_7,AIN7
#define ADC_GND             0x0C     // Ground
#define ADC_TEMP_SENS       0x0E     // on-chip temperature sensor
#define ADC_VDD_3           0x0F     // (vdd/3)

#define ADC_AIN01			0x08	//AIN0-AIN1
#define ADC_AIN23			0x09	//AIN2-AIN3
#define ADC_AIN45			0x0A	//AIN4-AIN5
#define ADC_AIN67			0x0B	//AIN6-AIN7
#define ADC_POSITIVE_VOLTAGE	0x0D	//Positive voltage reference
*/
INT16 halAdcSampleSingle(BYTE reference, BYTE resolution, UINT8 input) {
//reference参考电压引脚resolution 分辨率input 引脚
//例如	  //1000 0000 , 000 0000 , 0000 0110
    BYTE volatile temp;
    INT16 value;

    //reading out any old conversion value
    temp = ADCH;
    temp = ADCL;


    ADC_ENABLE_CHANNEL(input);//激活引脚
    ADC_STOP();//手动ADC

    ADC_SINGLE_CONVERSION(reference | resolution | input);//参考电压.分辨率,端口号
    //1000 0110,看ADCCON3
    //=AVDD5引脚参考电压,64抽样率,ANI6,就是P0_6

    while (!ADC_SAMPLE_READY());//等着ADC进行

    ADC_DISABLE_CHANNEL(input);//关闭引脚

    value = (((INT16)ADCH) << 8);
    value |= ADCL;

    resolution >>= 3;
    return value >> (8 - resolution);
}
