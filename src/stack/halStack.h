#ifndef HALSTACK_H
#define HALSTACK_H

//these are prototypes for HAL functions called from the stack
//if a HAL function is not in here, then it is in the
//hal.h file in the target subdirectory

#define T2CMPVAL 625L

#define HAL_ENTER_INTERRUPT()  halInterruptFlag = 1
#define HAL_EXIT_INTERRUPT()   halInterruptFlag = 0


void halInit(void);  //processor, board specific initializations

void halInitUart(void);  // do everything except baud rate setting.
void halInitRF(void);//初始化射频部分

char halGetch(void);  //get a character from serial port
BOOL halGetchRdy(void);  //is a character available from the serial port?
void halPutch(char c);  //write a character to serial port
void halRawPut(char c);  //write a byte to serial port, no character interpretation
void halInitMACTimer(void); //init timer used for Radio timeouts
UINT32 halGetMACTimer(void); //return timer value

UINT32 halMACTimerNowDelta(UINT32 previoustime);
UINT32 halMACTimerDelta(UINT32 preslot,UINT32 pretime);

LRWPAN_STATUS_ENUM halInitRadio(PHY_FREQ_ENUM frequency, BYTE channel, RADIO_FLAGS radio_flags);
void halGetProcessorIEEEAddress(BYTE *buf);
void halSetRadioIEEEAddress(void );
LRWPAN_STATUS_ENUM halSetRadioIEEEFrequency(PHY_FREQ_ENUM frequency, BYTE channel);
void halSetRadioPANID(UINT16 panid);
void halSetRadioShortAddr(SADDR saddr);
LRWPAN_STATUS_ENUM halSendPacket(BYTE flen, BYTE *frm);
void halRepeatSendPacket(BYTE DsnIndex);
//void halSendAckFrame(BYTE DSN);
LRWPAN_STATUS_ENUM halSetChannel(BYTE channel);

UINT32 halMacTicksToUs(UINT32 x);

#ifdef LRWPAN_COORDINATOR
static void halDirectSendBeacon(void);
#endif

UINT8 halGetRandomByte(void);
//void halSleep(UINT32 msecs);    //put processor to sleep
void halSuspend(UINT32 msecs);  //suspends process, intended for Win32, dummy on others
void halUtilMemCopy(BYTE *dst, BYTE *src, BYTE len);
void halWait(BYTE wait);
void halWaitMs(UINT32 msecs);
void halShutdown(void);
void halWarmstart(void);


//call backs to PHY, MAC from HAL
void phyRxCallback(void);
void phyTxStartCallBack(BYTE *ptr);
void phyTxEndCallBack(void);
void macRxCallback(BYTE *ptr, INT8 rssi, UINT8 corr);
//void macTxCallback(void);

void evbIntCallback(void);  //Evaluation board slow timer interrupt callback
void usrIntCallback(void);   //general interrupt callback , when this is called depends on the HAL layer.


#endif

