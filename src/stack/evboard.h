#ifndef EVBOARD_H
#define EVBOARD_H

//定义开关状态的共用体
typedef union _EVB_SW_STATE {
  BYTE val;
  struct _SW_STATE_bits {
    unsigned s1_val:1;
    unsigned s1_last_val:1;
    unsigned s1_tgl:1;
    unsigned s2_val:1;
    unsigned s2_last_val:1;
    unsigned s2_tgl:1;
  }bits;
}EVB_SW_STATE;



extern EVB_SW_STATE sw_state;


#define EVB_LED1_ON()       evbLedSet(1,TRUE)   //控制LED1灯开
#define EVB_LED1_OFF()     evbLedSet(1,FALSE)   //控制LED1灯灭
#define EVB_LED2_ON()       evbLedSet(2,TRUE)	//控制LED2灯开
#define EVB_LED2_OFF()     evbLedSet(2,FALSE)	//控制LED2灯灭
#define EVB_LED3_ON()       evbLedSet(3,TRUE)
#define EVB_LED3_OFF()     evbLedSet(3,FALSE)
#define EVB_LED4_ON()       evbLedSet(4,TRUE)
#define EVB_LED4_OFF()     evbLedSet(4,FALSE)

#define EVB_LED1_STATE()  evbLedGet(1)  //判断LED灯的状态，开为'1'，灭为'0'
#define EVB_LED2_STATE()  evbLedGet(2)

//通过sw_state.bits.s1_val状态来判断开关S1是否按下或放开，'1'为按下，'0'为放开
#define EVB_SW1_PRESSED()     (sw_state.bits.s1_val)
#define EVB_SW1_RELEASED()    (!sw_state.bits.s1_val)

//通过sw_state.bits.s2_val状态来判断开关S2是否按下或放开，'1'为按下，'0'为放开
#define EVB_SW2_PRESSED()     (sw_state.bits.s2_val)
#define EVB_SW2_RELEASED()    (!sw_state.bits.s2_val)



#define EVB_SW1_TOGGLED()     (sw_state.bits.s1_tgl)
#define EVB_SW1_CLRTGL()      sw_state.bits.s1_tgl=0
#define EVB_SW2_TOGGLED()     (sw_state.bits.s2_tgl)
#define EVB_SW2_CLRTGL()      sw_state.bits.s2_tgl=0

//更新开关状态共用体的状态变量，有两种方式：一、查询，二、中断。
void evbPoll(void);
void evbInit(void);

//LED灯的开关设置
void evbLedSet(BYTE lednum, BOOL state);
//获取LED灯的开关状态
BOOL evbLedGet(BYTE lednum);
void controlU (void);

#endif




