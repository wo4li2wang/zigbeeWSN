#include "hal.h"
#include "halStack.h"
#include "evboard.h"
#include "evbConfig.h"
/*******************************************************************************
Joystick
*******************************************************************************/
#define JOYSTICK_PUSH         P2_0   //¶¨Òå²Ù×Ý¸Ë×´Ì¬
#define JOYSTICK_PRESSED()    JOYSTICK_PUSH
//³õÊ¼»¯¼Ä´æÆ÷P2DIRºÍP2INP,ÅäÖÃP2_0ÎªÊäÈëºÍÈýÌ¬
#define INIT_JOYSTICK_PUSH() \
    do {                     \
        P2DIR &= ~0x01;      \
        P2INP |= 0x01;       \
    } while (0)

BOOL joystickPushed( void );

//¶¨Òå²Ù×Ý¸Ë·½Ïò×´Ì¬£¬Ã¶¾Ù
typedef enum {
  CENTRED,
  LEFT,
  RIGHT,
  UP,
  DOWN
} JOYSTICK_DIRECTION;


#define JOYSTICK              P0_6  //ÉèÖÃ²Ù×Ý¸Ë×´Ì¬
#define INIT_JOYSTICK()       IO_DIR_PORT_PIN(0, 6, IO_IN) //³õÊ¼»¯P0_6ÎªÊäÈë
#define ADC_INPUT_JOYSTICK    0x06  //ÉèÖÃADCµÄÍ¨µÀµØÖ·
//¾ÍÊÇP0_6

//»ñÈ¡²Ù×Ý¸Ë·½Ïò×´Ì¬
JOYSTICK_DIRECTION getJoystickDirection( void );

//¿ª¹Ø×´Ì¬
EVB_SW_STATE sw_state;

//»ñÈ¡²Ù×Ý¸Ë·½Ïò×´Ì¬£¬ÅÐ¶ÏÁ½´Î

 JOYSTICK_DIRECTION getJoystickDirection( void ) {
    INT8 adcValue, i;
    JOYSTICK_DIRECTION direction[2];


    for(i = 0; i < 2; i++){
      //Æô¶¯×ª»»
       adcValue = halAdcSampleSingle(ADC_REF_AVDD, ADC_8_BIT, ADC_INPUT_JOYSTICK);
	  //1000 0000 , 000 0000 , 0000 0110
	  //ADC_REF_AVDD¾ÍÊÇ¡¡¡¡ 20½Å£¨AVDD_SOC£©£ºÎªÄ£ÄâµçÂ·Á¬½Ó2.0¡«3.6 VµÄµçÑ¹¡
	  //=AVDD5Òý½Å²Î¿¼µçÑ¹,64³éÑùÂÊ,ANI6,¾ÍÊÇP0_6
	  //µ½ÕâÀï¾Í»ñµÃÁËµçÑ¹
       if (adcValue < 0x29) {
          direction[i] = DOWN;  // Measured 0x01
       } else if (adcValue < 0x50) {
          direction[i] = LEFT;  // Measured 0x30
       } else if (adcValue < 0x45) {
          direction[i] = CENTRED;  // Measured 0x40
       } else if (adcValue < 0x35) {
          direction[i] = RIGHT; // Measured 0x4D
       } else if (adcValue < 0x20) {
          direction[i] = UP;    // Measured 0x5C
       } else {
          direction[i] = CENTRED; // Measured 0x69
       }
    }

    if(direction[0] == direction[1]){
       return direction[0];
    }
    else{
       return CENTRED;
    }
}

//¶¨Òå¿ª¹Ø°´¼üÊ±¼ä
#define SW_POLL_TIME   MSECS_TO_MACTICKS(100)

UINT32 last_switch_poll;
/*º¯Êý¹¦ÄÜ:Ã¿Ò»100ms¸üÐÂÒ»´Î¿ª¹Ø×´Ì¬±äÁ¿£¬Èç¹û¶¨Òå LRWPAN_ENABLE_SLOW_TIMER£¬
           ÔòÊ¹ÓÃÖÐ¶Ï£»·ñÔò²ÉÓÃ²éÑ¯·½Ê½*/
/*only do this if the slow timer not enabled as reading the joystick takes a while.
If the slowtimer is enabled, then that interrupt is handing polling*/
void evbPoll(void){
#ifndef LRWPAN_ENABLE_SLOW_TIMER
  if ( halMACTimerNowDelta(last_switch_poll) > SW_POLL_TIME) {
   evbIntCallback();
   last_switch_poll = halGetMACTimer();
  }
#endif

}

/*º¯Êý¹¦ÄÜ:¿ª·¢µ×°å³õÊ¼»¯£¬°üÀ¨µ¥Æ¬»úÊ±ÖÓ¡¢´®¿ÚµÈ£¬¼üÅÌºÍLED*/
void evbInit(void){
  halInit();
  INIT_JOYSTICK();
  sw_state.val = 0;
  INIT_LED1();
  INIT_LED2();
  INIT_LED3();
  INIT_LED4();
}

/*º¯Êý¹¦ÄÜ:ÉèÖÃLED1»òLED2µÄ¿ª¹Ø×´Ì¬*/
void evbLedSet(BYTE lednum, BOOL state) {
    switch(lednum) {
       case 1:    if (state) LED1_ON(); else LED1_OFF(); break;
       case 2:    if (state) LED2_ON(); else LED2_OFF(); break;
       case 3:    if (state) LED3_ON(); else LED3_OFF(); break;
       case 4:    if (state) LED4_ON(); else LED4_OFF(); break;
    }
}

/*º¯Êý¹¦ÄÜ:»ñµÃLED1»òLED2µÄ¿ª¹Ø×´Ì¬£¬TRUE Îª ¿ª£¬FALSE Îª ¹Ø*/
BOOL evbLedGet(BYTE lednum){
  switch(lednum) {
       case 1:    return(LED1_STATE());
       case 2:    return(LED2_STATE());
    }
  return(FALSE);
}


/*if joystick pushed up, consider this a S1 button press,
if joystick pushed down, consider this a S2 button press,
does not allow for both buttons to be pressed at once,
tgl bits are set if the state bits become different*/
/*º¯Êý¹¦ÄÜ:Ê×ÏÈ»ñÈ¡²Ù×Ý¸ËµÄ·½Ïò£¬ÔÙ¸üÐÂ¿ª¹Ø×´Ì¬¹²ÓÃÌåµÄ×´Ì¬Öµ*/
void evbIntCallback(void){
  JOYSTICK_DIRECTION x;
  x = getJoystickDirection(); //xÎª²Ù×Ý¸Ë·½Ïò
  if (x == CENTRED) {
    sw_state.bits.s1_val = 0;
    sw_state.bits.s2_val = 0;
  }
  else  if (x == UP) sw_state.bits.s1_val = 1;
  else if (x == DOWN) sw_state.bits.s2_val = 1;
  if (sw_state.bits.s1_val != sw_state.bits.s1_last_val) sw_state.bits.s1_tgl = 1;
  if (sw_state.bits.s2_val != sw_state.bits.s2_last_val) sw_state.bits.s2_tgl = 1;
  sw_state.bits.s1_last_val = sw_state.bits.s1_val;
  sw_state.bits.s2_last_val = sw_state.bits.s2_val;
}

//general interrupt callback , when this is called depends on the HAL layer.
void usrIntCallback(void)
{
}

//ÒÔÏÂÊÇÎÒÐ´µÄ²¿·Ö
void controlU (void)
{
    INT8 i,adcValue;
      //Æô¶¯×ª»»
       adcValue = halAdcSampleSingle(ADC_REF_AVDD, ADC_8_BIT, ADC_INPUT_JOYSTICK);
	//µçÑ¹ÒÔAVDD5Îª²Î¿¼
       if (adcValue < 0x29) {//0010 1001
          LED1_ON();
	   LED2_OFF();
	   LED3_ON();
	   LED4_OFF();
       } else {
         LED1_OFF();
	  LED2_ON();
	  LED3_OFF();
	  LED4_ON();
       }
}




