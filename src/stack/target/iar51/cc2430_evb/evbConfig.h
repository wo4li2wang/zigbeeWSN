#ifndef EVBCONFIG_H
#define EVBCONFIG_H

//macros taken from Chipcon EVB header file

#define FOSC 32000000    //CPU频率32MHz

#define LED_OFF 0  //定义LED关为1
#define LED_ON  1  //定义LED开为0

//I could not get LED2, LED4 to work on my CC2430EB
#define LED1          P1_0 //定义LED1连P1_0
#define LED2          P1_1 //定义LED2连P1_2
#define LED3          P1_2 //定义LED3连P1_3 模拟
#define LED4          P1_3 //定义LED4连P1_4 充电

#define INIT_LED1()   do { LED1 = LED_OFF; IO_DIR_PORT_PIN(1, 0, IO_OUT); P1SEL &= ~0x01;} while (0)
//定义 LED1灯灭，P1_0为输出，P1SEL_0='0'
#define INIT_LED2()   do { LED2 = LED_OFF; IO_DIR_PORT_PIN(1, 1, IO_OUT); P1SEL &= ~0x02;} while (0)
//定义 LED1灯灭，P1_3为输出，P1SEL_3='0'
#define INIT_LED3()   do { LED3 = LED_OFF; IO_DIR_PORT_PIN(1, 2, IO_OUT); P1SEL &= ~0x04;} while (0)
#define INIT_LED4()   do { LED4 = LED_OFF; IO_DIR_PORT_PIN(1, 3, IO_OUT); P1SEL &= ~0x08;} while (0)


#define LED1_ON()  (LED1 = LED_ON)  //定义 LED灯开
#define LED2_ON()  (LED2 = LED_ON)
#define LED3_ON()  (LED3 = LED_ON)
#define LED4_ON()  (LED4 = LED_ON)



#define LED1_OFF()  (LED1 = LED_OFF)  //定义 LED灯灭
#define LED2_OFF()  (LED2 = LED_OFF)
#define LED3_OFF()  (LED3 = LED_OFF)
#define LED4_OFF()  (LED4 = LED_OFF)

#define LED1_STATE() (LED1 == LED_ON) //定义 LED灯状态，'1'为开，'0'为关
#define LED2_STATE() (LED2 == LED_ON)




#endif







