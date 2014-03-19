#ifndef HAL_H
#define HAL_H
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "ioCC2530.h"
#include "lrwpan_config.h"         //user configurations
#include "lrwpan_common_types.h"   //types common acrosss most files


/////////////////////////////////////////////////////////////////////////////////////////
// Common values
#ifndef FALSE
   #define FALSE 0
#endif

#ifndef TRUE
   #define TRUE 1
#endif

#ifndef NULL
   #define NULL 0
#endif

#ifndef HIGH
   #define HIGH 1
#endif

#ifndef LOW
   #define LOW 0
#endif

#define BV(n)      (1 << (n))
#define st(x)      do { x } while (0)	//执行st(x) 就是do { x } while (0)以后直接用
//只执行一次的循环

//for optimized indexing of uint16's
#define UINT16_NDX0   0
#define UINT16_NDX1   1


//for optimized indexing of uint32's
#define UINT32_NDX0   0
#define UINT32_NDX1   1
#define UINT32_NDX2   2
#define UINT32_NDX3   3


#define halIdle()              //do nothing in idle state

/*Macros for configuring IO peripheral location:
Example usage:
IO_PER_LOC_TIMER1_AT_PORT0_PIN234();
IO_PER_LOC_TIMER4_AT_PORT2_PIN03();
IO_PER_LOC_USART1_AT_PORT0_PIN2345();
*/
//Timer 1 I/O locate at P0  (channel0--P0_2,channel1--P0_3,channel2--P0_4)
#define IO_PER_LOC_TIMER1_AT_PORT0_PIN234() do { PERCFG = (PERCFG&~0x40)|0x00; } while (0)
//Timer 1 I/O locate at P1  (channel0--P1_2,channel1--P1_1,channel2--P1_0)
#define IO_PER_LOC_TIMER1_AT_PORT1_PIN012() do { PERCFG = (PERCFG&~0x40)|0x40; } while (0)

//Timer 3 I/O locate at P1  (channel0--P1_3,channel1--P1_4)
#define IO_PER_LOC_TIMER3_AT_PORT1_PIN34()  do { PERCFG = (PERCFG&~0x20)|0x00; } while (0)
//Timer 3 I/O locate at P1  (channel0--P1_6,channel1--P1_7)
#define IO_PER_LOC_TIMER3_AT_PORT1_PIN67()  do { PERCFG = (PERCFG&~0x20)|0x20; } while (0)

//Timer 4 I/O locate at P1  (channel0--P1_0,channel1--P1_1)
#define IO_PER_LOC_TIMER4_AT_PORT1_PIN01()  do { PERCFG = (PERCFG&~0x10)|0x00; } while (0)
//Timer 3 I/O locate at P1  (channel0--P2_0,channel1--P2_3)
#define IO_PER_LOC_TIMER4_AT_PORT2_PIN03()  do { PERCFG = (PERCFG&~0x10)|0x10; } while (0)

//SPI 1 I/O locate at P0  (MISO--P0_5,MOSI--P0_4,SSN--P0_2,SCK--P0_3)
#define IO_PER_LOC_SPI1_AT_PORT0_PIN2345()  do { PERCFG = (PERCFG&~0x08)|0x00; } while (0)
//SPI 1 I/O locate at P1  (MISO--P1_7,MOSI--P1_6,SSN--P1_4,SCK--P1_5)
#define IO_PER_LOC_SPI1_AT_PORT1_PIN4567()  do { PERCFG = (PERCFG&~0x08)|0x08; } while (0)

//SPI 0 I/O locate at P0  (MISO--P0_2,MOSI--P0_3,SSN--P0_4,SCK--P0_5)
#define IO_PER_LOC_SPI0_AT_PORT0_PIN2345()  do { PERCFG = (PERCFG&~0x04)|0x00; } while (0)
//SPI 0 I/O locate at P1  (MISO--P1_4,MOSI--P1_5,SSN--P1_2,SCK--P1_3)
#define IO_PER_LOC_SPI0_AT_PORT1_PIN2345()  do { PERCFG = (PERCFG&~0x04)|0x04; } while (0)

//UART 1 I/O locate at P0  (RXDATA--P0_5,TXDATA--P0_4,RTS--P0_3,CTS--P0_2)
#define IO_PER_LOC_UART1_AT_PORT0_PIN2345() do { PERCFG = (PERCFG&~0x02)|0x00; } while (0)
//UART 1 I/O locate at P1  (RXDATA--P1_7,TXDATA--P1_6,RTS--P1_5,CTS--P1_4)
#define IO_PER_LOC_UART1_AT_PORT1_PIN4567() do { PERCFG = (PERCFG&~0x02)|0x02; } while (0)

//UART 0 I/O locate at P0  (RXDATA--P0_5,TXDATA--P0_3,RTS--P0_2,CTS--P0_4)
#define IO_PER_LOC_UART0_AT_PORT0_PIN2345() do { PERCFG = (PERCFG&~0x01)|0x00; } while (0)//(*)UART0 IO口定位
//UART 0 I/O locate at P1  (RXDATA--P1_4,TXDATA--P1_5,RTS--P1_3,CTS--P1_2)
#define IO_PER_LOC_UART0_AT_PORT1_PIN2345() do { PERCFG = (PERCFG&~0x01)|0x01; } while (0)


// Actual MCU pin configuration:
//
// Peripheral I/O signal     Alt1       Alt2
// -------------------------------------------
// Timer1 channel0           P0.2       P1.2
// Timer1 channel1           P0.3       P1.1
// Timer1 channel2           P0.4       P1.0
// Timer3 channel0           P1.3       P1.6
// Timer3 channel1           P1.4       P1.7
// Timer4 channel0           P1.0       P2.0
// Timer4 channel1           P1.1       P2.3
// USART0 TXD/MOSI           P0.3       P1.5
// USART0 RXD/MISO           P0.2       P1.4
// USART0 RTS/SCK            P0.5       P1.3
// USART0 CTS/SS_N           P0.4       P1.2
// USART1 TXD/MOSI           P0.4       P1.6
// USART1 RXD/MISO           P0.5       P1.7
// USART1 RTS/SCK            P0.3       P1.5
// USART1 CTS/SS_N           P0.2       P1.4

// Macros for configuring IO direction:
// Example usage:
//   IO_DIR_PORT_PIN(0, 3, IO_IN);    // Set P0_3 to input
//   IO_DIR_PORT_PIN(2, 1, IO_OUT);   // Set P2_1 to output

//(*)AD使用
#define IO_DIR_PORT_PIN(port, pin, dir)  \
   do {                                  \
      if (dir == IO_OUT)                 \
         P##port##DIR |= (0x01<<(pin));  \
      else                               \
         P##port##DIR &= ~(0x01<<(pin)); \
   }while(0)

// Where port={0,1,2}, pin={0,..,7} and dir is one of:
#define IO_IN   0
#define IO_OUT  1

// Macros for configuring IO input mode:
// Example usage:
//   IO_IMODE_PORT_PIN(0,0,IO_IMODE_PUD);	// Set P0_0 to Pull-up/Pull-down
//   IO_IMODE_PORT_PIN(2,0,IO_IMODE_TRI);	// Set P2_0 to Tristate
//   IO_IMODE_PORT_PIN(1,3,IO_IMODE_PUD);	// Set P1_3 to Pull-up/Pull-down

#define IO_IMODE_PORT_PIN(port, pin, imode) \
   do {                                     \
      if (imode == IO_IMODE_TRI)            \
         P##port##INP |= (0x01<<(pin));     \
      else                                  \
         P##port##INP &= ~(0x01<<(pin));    \
   } while (0)

// where imode is one of:
#define IO_IMODE_PUD  0 // Pull-up/pull-down
#define IO_IMODE_TRI  1 // Tristate

// Macro for configuring IO drive mode:
// Example usage:
//   IIO_PUD_PORT(0, IO_PULLUP);	// Set P0 to Pull-up
//   IIO_PUD_PORT(1, IO_PULLDOWN);	// Set P1 to Pull-down
//   IIO_PUD_PORT(2, IO_PULLUP);	// Set P2 to Pull-up

#define IO_PUD_PORT(port, pud)        \
   do {                               \
      if (pud == IO_PULLDOWN)         \
         P2INP |= (0x01 << (port+5)); \
      else                            \
         P2INP &= ~(0x01 << (port+5));\
   } while (0)

#define IO_PULLUP          0
#define IO_PULLDOWN        1

// Macros for function select (General purpose I/O or Peripheral function):
// Example usage:
//   IO_FUNC_PORT0_PIN0(0, 0, IO_FUNC_PERIPH);	// Set P0_0 to Peripheral function
//   IO_FUNC_PORT0_PIN1(0, 1, IO_FUNC_GIO);		// Set P0_1 to General purpose I/O
//   IO_FUNC_PORT2_PIN3(2, 3, IO_FUNC_PERIPH);	// Set P2_3 to Peripheral function

#define IO_FUNC_PORT_PIN(port, pin, func)  \
   do {                                    \
      if((port == 2) && (pin == 3)){       \
         if (func) {                       \
            P2SEL |= 0x02;                 \
         } else {                          \
            P2SEL &= ~0x02;                \
         }                                 \
      }                                    \
      else if((port == 2) && (pin == 4)){  \
         if (func) {                       \
            P2SEL |= 0x04;                 \
         } else {                          \
            P2SEL &= ~0x04;                \
         }                                 \
      }                                    \
      else{                                \
         if (func) {                       \
            P##port##SEL |= (0x01<<(pin)); \
         } else {                          \
            P##port##SEL &= ~(0x01<<(pin));\
        }                                  \
      }                                    \
   } while (0)

// where func is one of:
#define IO_FUNC_GIO     0 // General purpose I/O
#define IO_FUNC_PERIPH  1 // Peripheral function

// Macros for configuring the ADC input:
// Example usage:
//   IO_ADC_PORT0_PIN(0, IO_ADC_EN);	//P0_0 as ADC inputs AIN0
//   IO_ADC_PORT0_PIN(4, IO_ADC_DIS);	//P0_4 as ADC inputs AIN4
//   IO_ADC_PORT0_PIN(6, IO_ADC_EN);	//P0_6 as ADC inputs AIN6

#define IO_ADC_PORT0_PIN(pin, adcEn) \
   do {                              \
      if (adcEn)                     \
         ADCCFG |= (0x01<<pin);      \
      else                           \
         ADCCFG &= ~(0x01<<pin); }   \
   while (0)

// where adcEn is one of:
#define IO_ADC_EN           1 // ADC input enabled
#define IO_ADC_DIS          0 // ADC input disabled

//from ChipCon,we use CC2430-128
//#define CC2430_FLASH_SIZE 32
//#define CC2430_FLASH_SIZE 64
#define CC2430_FLASH_SIZE 128

#if (CC2430_FLASH_SIZE == 32)
#define IEEE_ADDRESS_ARRAY 0x7FF8
#elif (CC2430_FLASH_SIZE == 64) || (CC2430_FLASH_SIZE == 128)
#define IEEE_ADDRESS_ARRAY 0xFFF8
#endif


#define HAL_SUSPEND(x)         //dummy in uC, only needed for Win32

//MACTimer Support
//For the CC2430, we set the timer to exactly 1 tick per symbol
//assuming a clock frequency of 32MHZ, and 2.4GHz
#define SYMBOLS_PER_MAC_TICK()     1
#define SYMBOLS_TO_MACTICKS(x) (x/SYMBOLS_PER_MAC_TICK())   //convert Symbols to MAC Ticks
#define MSECS_TO_MACTICKS(x)   (x*(LRWPAN_SYMBOLS_PER_SECOND/1000))////convert  ms to MAC Ticks
#define MACTIMER_MAX_VALUE 0x000FFFFF   //Overflow count is 20 bit counter
//#define halMACTimerNowDelta(x) ((halGetMACTimer()-(x))& MACTIMER_MAX_VALUE)//halGetMACTimer() get current Timer 2 Overflow count
//#define halMACTimerNowDelta(x) ((x>halGetMACTimer())?(halGetMACTimer()+T2CMPVAL-x):(halGetMACTimer()-x)))& MACTIMER_MAX_VALUE)//halGetMACTimer() get current Timer 2 Overflow count
//#define halMACTimerDelta(x,y) ((x-(y))& MACTIMER_MAX_VALUE)

/******************************************************************************
*******************       Interrupt functions/macros        *******************
******************************************************************************/

// Macros which simplify access to interrupt enables, interrupt flags and
// interrupt priorities. Increases code legibility.

//******************************************************************************

#define INT_ON   1
#define INT_OFF  0
#define INT_SET  1
#define INT_CLR  0


#define INT_GLOBAL_ENABLE(on) EA=(!!on)	//Enables global interrupt
#define SAVE_AND_DISABLE_GLOBAL_INTERRUPT(x) {x=EA;EA=0;}// Save global interrupt and disable global interrupt
#define RESTORE_GLOBAL_INTERRUPT(x) EA=x//Restore global interrupt
#define ENABLE_GLOBAL_INTERRUPT() INT_GLOBAL_ENABLE(INT_ON)//允许中断
#define DISABLE_GLOBAL_INTERRUPT() INT_GLOBAL_ENABLE(INT_OFF)//Disable global interrupt
#define DISABLE_ALL_INTERRUPTS() (IEN0 = IEN1 = IEN2 = 0x00)//Disable all interrupts

#define HAL_ENTER_CRITICAL_SECTION(x)   SAVE_AND_DISABLE_GLOBAL_INTERRUPT(x)
#define HAL_EXIT_CRITICAL_SECTION(x)    RESTORE_GLOBAL_INTERRUPT(x)

/***********See Page 52 ,Table 29: Interrupts Overview***************/
#define INUM_RFERR 0
#define INUM_ADC   1
#define INUM_URX0  2
#define INUM_URX1  3
#define INUM_ENC   4
#define INUM_ST    5
#define INUM_P2INT 6
#define INUM_UTX0  7
#define INUM_DMA   8
#define INUM_T1    9
#define INUM_T2    10
#define INUM_T3    11
#define INUM_T4    12
#define INUM_P0INT 13
#define INUM_UTX1  14
#define INUM_P1INT 15
#define INUM_RF    16
#define INUM_WDT   17

#define NBR_OF_INTERRUPTS 18


#define INT_ENABLE_RF(on)     { (on) ? (IEN2 |= 0x01) : (IEN2 &= ~0x01); }//RF general interrupt enable
#define INT_ENABLE_RFERR(on)  { RFERRIE = on; }//RF TX/RX FIFO interrupt enable
#define INT_ENABLE_T2(on)     { T2IE    = on; }//Timer 2 interrupt enable
#define INT_ENABLE_URX0(on)   { URX0IE  = on; }//USART0 RX interrupt enable


#define INT_GETFLAG_RFERR() RFERRIF//TCON.RFERRIF(b1)
#define INT_GETFLAG_RF()    S1CON &= ~0x03//b1b0:  S1CON.RFIF_1 S1CON.RFIF_0


#define INT_SETFLAG_RFERR(f) RFERRIF= f
#define INT_SETFLAG_RF(f)  { (f) ? (S1CON |= 0x03) : (S1CON &= ~0x03); }
#define INT_SETFLAG_T2(f)  { T2IF  = f;  }




/******************************************************************************
*******************         Common USART functions/macros   *******************
******************************************************************************/

// The macros in this section are available for both SPI and UART operation.

//*****************************************************************************

// Example usage:
//   USART0_FLUSH();
#define USART_FLUSH(num)              (U##num##UCR |= 0x80)
#define USART0_FLUSH()                USART_FLUSH(0)//Stop the current operation and return the USART0 to idle state
#define USART1_FLUSH()                USART_FLUSH(1)//Stop the current operation and return the USART1 to idle state

// Example usage:
//   if (USART0_BUSY())
#define USART_BUSY(num)               (U##num##CSR & 0x01 == 0x01)
#define USART0_BUSY()                 USART_BUSY(0)//true is USART0 busy,false is USART0 idle
#define USART1_BUSY()                 USART_BUSY(1)//true is USART1 busy,false is USART1 idle

// Example usage:
//   while(!USART1_BYTE_RECEIVED())
#define USART_BYTE_RECEIVED(num)      ((U##num##CSR & 0x04) == 0x04)
#define USART0_BYTE_RECEIVED()        USART_BYTE_RECEIVED(0)//true is USART0 received byte ready,false is USART0 no byte received
#define USART1_BYTE_RECEIVED()        USART_BYTE_RECEIVED(1)//true is USART1 received byte ready,false is USART1 no byte received

// Example usage:
//   if(USART1_BYTE_TRANSMITTED())
#define USART_BYTE_TRANSMITTED(num)   ((U##num##CSR & 0x02) == 0x02)
#define USART0_BYTE_TRANSMITTED()     USART_BYTE_TRANSMITTED(0)//true is USART0 last byte transmitted,false is USART0 byte not transmitted
#define USART1_BYTE_TRANSMITTED()     USART_BYTE_TRANSMITTED(1)//true is USART1 last byte transmitted,false is USART1 byte not transmitted


/******************************************************************************
*******************  USART-UART specific functions/macros   *******************
******************************************************************************/
// The macros in this section simplify UART operation.
//According to band, we get the clkDivPow(UxGCR.BAUD_E)
#define BAUD_E(baud, clkDivPow) (     \
    (baud==1200)   ?  5  +clkDivPow : \
    (baud==2400)   ?  6  +clkDivPow : \
    (baud==4800)   ?  7  +clkDivPow : \
    (baud==9600)   ?  8  +clkDivPow : \
    (baud==14400)  ?  8  +clkDivPow : \
    (baud==19200)  ?  9  +clkDivPow : \
    (baud==28800)  ?  9  +clkDivPow : \
    (baud==38400)  ?  10 +clkDivPow : \
    (baud==57600)  ?  10 +clkDivPow : \
    (baud==76800)  ?  11 +clkDivPow : \
    (baud==115200) ?  11 +clkDivPow : \
    (baud==153600) ?  12 +clkDivPow : \
    (baud==230400) ?  12 +clkDivPow : \
    (baud==307200) ?  13 +clkDivPow : \
    0  )

//According to band, we get the UxGCR.BAUD_M
#define BAUD_M(baud) (      \
    (baud==1200)   ?  59  : \
    (baud==2400)   ?  59  : \
    (baud==4800)   ?  59  : \
    (baud==9600)   ?  59  : \
    (baud==14400)  ?  216 : \
    (baud==19200)  ?  59  : \
    (baud==28800)  ?  216 : \
    (baud==38400)  ?  59  : \
    (baud==57600)  ?  216 : \
    (baud==76800)  ?  59  : \
    (baud==115200) ?  216 : \
    (baud==153600) ?  59  : \
    (baud==230400) ?  216 : \
    (baud==307200) ?  59  : \
  0)



//*****************************************************************************

// Macro for setting up a UART transfer channel. The macro sets the appropriate
// pins for peripheral operation, sets the baudrate, and the desired options of
// the selected uart. _uart_ indicates which uart to configure and must be
// either 0 or 1. _baudRate_ must be one of 2400, 4800, 9600, 14400, 19200,
// 28800, 38400, 57600, 76800, 115200, 153600, 230400 or 307200. Possible
// options are defined below.
//
// Example usage:
//
//      UART_SETUP(0,115200,HIGH_STOP);
//
// This configures uart 0 for contact with "hyperTerminal", setting:
//      Baudrate:           115200
//      Data bits:          8
//      Parity:             None
//      Stop bits:          1
//      Flow control:       None
//
/*根据Baudrate, 设置串口参数*//*给串口配置引脚*/
#define UART_SETUP(uart, baudRate, options)      \
   do {                                          \
      if((uart) == 0){                           \
         if(PERCFG & 0x01){                      \
            P1SEL |= 0x30;                       \
         } else {                                \
            P0SEL |= 0x0C;                       \
         }                                       \
      }                                          \
      else {                                     \
         if(PERCFG & 0x02){                      \
            P1SEL |= 0xC0;                       \
         } else {                                \
            P0SEL |= 0x30;                       \
         }                                       \
      }                                          \
      U##uart##GCR = BAUD_E((baudRate),CLKSPD);  \
      U##uart##BAUD = BAUD_M(baudRate);          \
      U##uart##CSR |= 0x80;                      \
      U##uart##UCR |= ((options) | 0x80);        \
      if((options) & TRANSFER_MSB_FIRST){        \
         U##uart##GCR |= 0x20;                   \
      }                                          \
      U##uart##CSR |= 0x40;                      \
   } while(0)
//(*)UART0参数9600 8 N 1  


//can do this via a macro
//Configure USART0 to UART mode,according to band, we get BAUD_E and BAUD_M,
//Flush unit,  High stop bit,  Flow control disabled,   8 bits transfer,  1 stop bit
//LSB first,   Receiver enabled
#define halSetBaud(baud) UART_SETUP(0, baud, HIGH_STOP);


// Options for UART_SETUP macro
#define FLOW_CONTROL_ENABLE         0x40
#define FLOW_CONTROL_DISABLE        0x00
#define EVEN_PARITY                 0x20
#define ODD_PARITY                  0x00
#define NINE_BIT_TRANSFER           0x10
#define EIGHT_BIT_TRANSFER          0x00
#define PARITY_ENABLE               0x08
#define PARITY_DISABLE              0x00
#define TWO_STOP_BITS               0x04
#define ONE_STOP_BITS               0x00
#define HIGH_STOP                   0x02
#define LOW_STOP                    0x00
#define HIGH_START                  0x01
#define TRANSFER_MSB_FIRST          0x80
#define TRANSFER_MSB_LAST           0x00
#define UART_ENABLE_RECEIVE         0x40


// Example usage:
//   if(UART0_PARERR())
#define UART_PARERR(num)      ((U##num##CSR & 0x08) == 0x08)
#define UART0_PARERR()        UART_PARERR(0)
//true is byte receivered with parity error in USART0,false is no parity error detected in USART0
#define UART1_PARERR()        UART_PARERR(1)
//true is byte receivered with parity error in USART1,false is no parity error detected in USART1

// Example usage:
//   if(UART1_FRAMEERR())
#define UART_FRAMEERR(num)    ((U ##num## CSR & 0x10) == 0x10)
//true is byte receivered with incorrect stop bit level error in USART0,false is no framing error detected in USART0
#define UART0_FRAMEERR()      UART_FRAMEERR(0)
//true is byte receivered with incorrect stop bit level error in USART1,false is no framing error detected in USART1
#define UART1_FRAMEERR()      UART_FRAMEERR(1)


// Example usage:
//   char ch = 'A';
//   UART1_SEND(ch);
//   UART1_RECEIVE(ch);
#define UART_SEND(num, x)   U##num##DBUF = x
#define UART0_SEND(x)       UART_SEND(0, x)//UART0 send data x
#define UART1_SEND(x)       UART_SEND(1, x)//UART1 send data x

#define UART_RECEIVE(num, x)  x = U##num##DBUF
#define UART0_RECEIVE(x)      UART_RECEIVE(0, x)//UART0 receiver data, is x
#define UART1_RECEIVE(x)      UART_RECEIVE(1, x)//UART1 receiver data, is x

/******************************************************************************
*******************      Power and clock management        ********************
*******************************************************************************

These macros are used to set power-mode, clock source and clock speed.

******************************************************************************/

// Macro for getting the clock division factor
#define CLKSPD  (CLKCONSTA & 0x07)//2530(CLKCON>CLKCONSTA)

// Macro for getting the timer tick division factor.
#define TICKSPD ((CLKCONSTA & 0x38) >> 3)//2530(CLKCON>CLKCONSTA)

// Macro for checking status of the crystal oscillator
#define XOSC_STABLE (!CLKCONSTA & 0x40)//2530(SLEEP>!CLKCONSTA)

// Macro for checking status of the high frequency RC oscillator.
#define HIGH_FREQUENCY_RC_OSC_STABLE    (CLKCONSTA & 0x40)//2530(SLEEP>CLKCONSTA;0x20>0x40)


// Macro for setting the 32 KHz clock source
#define SET_32KHZ_CLOCK_SOURCE(source) \
   do {                                \
      if( source ) {                   \
         CLKCONCMD |= 0x80;   \
      } else {                         \
         CLKCONCMD &= ~0x80;  \
      }                                \
   } while (0)
   //2530(CLKCON>CLKCONCMD)

// Where _source_ is one of
#define CRYSTAL 0x00
#define RC      0x01

// Macro for setting the main clock oscillator source,
//turns off the clock source not used
//changing to XOSC will take approx 150 us


//(*)
/*
#define SET_MAIN_CLOCK_SOURCE(source) \
   do {                               \
      if(source) {                    \
        CLKCONCMD |= 0x40;    \
        while(!HIGH_FREQUENCY_RC_OSC_STABLE); \
        if(TICKSPD == 0){             \
          CLKCONCMD |= 0x08;     \
        }                             \
        SLEEPCMD |= 0x04;                \
      }                               \
      else {                          \
        while(!XOSC_STABLE);          \
        asm("NOP");                   \
        CLKCONCMD &= ~0x47;      \
        SLEEPCMD |= 0x04;                \
      }                               \
   }while (0)
*/


#define SET_MAIN_CLOCK_SOURCE(source) \
   do {                               \
      if(source) {                    \
        CLKCONCMD = 0x49;             \
        while(CLKCONSTA!=0x49);       \
      }                               \
      else {                          \
        CLKCONCMD = 0x88;           \
         while(CLKCONSTA!=0x88);      \
      }                               \
        SLEEPCMD |= 0x04;             \
   }while (0)


/*
#### RADIO Support
*/

#define STOP_RADIO()        ISRFOFF;

// RF interrupt flags,see page 165
//#define IRQ_RREG_ON         0x80//Voltage regulator for radio has been turned on     //CC2530
#define IRQ_TXDONE          0x02//TX completed with packet sent   //CC2530(0x40>0x02)
#define IRQ_FIFOP           0x04//Number of bytes in RXFIFO is above thresthold set by IOCFG0.FIFO_THR  //CC2530(0x20>0x04)
#define IRQ_SFD             0x02//Start of frame delimiter(SFD) has been detected  //CC2530(0x10>0x02)
//#define IRQ_CCA             0x08//Clear channel assessment(CCA) indicates that channel is clear  //CC2530
#define IRQ_CSP_WT          0x20//CSMA-CA/strobe processor(CSP) wait condition is true  //CC2530(0x04>0x20)
#define IRQ_CSP_STOP        0x10//CSMA-CA/strobe processor(CSP)program execution stopped  //CC2530(0x02>0x10)
#define IRQ_CSP_INT         0x08//CSMA-CA/strobe processor(CSP) INT instruction executed  //CC2530(0x01>0x08)

#define IRQ_TXACKDONE         0x01//TX completed with ACK sent   //CC2530
#define IRQ_RF_IDLE         0x04//Radio state machine has entered the idle state    //CC2530
#define IRQ_SRC_MATCH_DONE         0x08//Source matching complete    //CC2530
#define IRQ_SRC_MATCH_FOUND        0x10//Source match found      //CC2530
#define IRQ_SRC_FRAME_ACCEPTED      0x20//Frame has passed frame filtering      //CC2530
#define IRQ_RXPKTDONE     0x40//A complete frame has been received      //CC2530
#define IRQ_RXMASKZERO      0x80//The RXENABLE register has gone from a nonzero state to an all-zero state     //CC2530

// RF status flags
#define TX_ACTIVE_FLAG      0x02//CC2530(0x10>0x02)
#define FIFO_FLAG           0x80//CC2530(0x08>0x80)
#define FIFOP_FLAG          0x40//CC2530(0x04>0x40)
#define SFD_FLAG            0x20//CC2530(0x02>0x20)
#define CCA_FLAG            0x10//CC2530(0x01>0x10)

#define RX_ACTIVE_FLAG            0x01//CC2530
#define LOCK_STATUS_FLAG            0x04//CC2530
#define SAMPLED_CCA_FLAG            0x08//CC2530


// Radio status states
#define TX_ACTIVE   (FSMSTAT1 & TX_ACTIVE_FLAG)//0:TX inactive,1:TX active
#define FIFO        (FSMSTAT1 & FIFO_FLAG)//0:No data available in RXFIFO,1:One or more bytes available in RXFIFO
#define FIFOP       (FSMSTAT1 & FIFOP_FLAG)//see page 215
#define SFD         (FSMSTAT1 & SFD_FLAG)//0:SFD not detected,1:SED detected
#define CCA         (FSMSTAT1 & CCA_FLAG)//see page 215
//CC2530(RFSTATUS>FSMSTAT1)

// Various radio settings
#define PAN_COORDINATOR     0x02  //CC2530(0x10>0x02)
#define ADR_DECODE          0x01  //CC2530(0x08>0x01)
#define AUTO_CRC            0x40//CC2530(0x20>0x40)
#define AUTO_ACK            0x20//CC2530(0x10>0x20)
#define AUTO_TX2RX_OFF      0x08
#define RX2RX_TIME_OFF      0x04
#define ACCEPT_ACKPKT       0x01






//-----------------------------------------------------------------------------
// Command Strobe Processor (CSP) instructions
//-----------------------------------------------------------------------------
#define DECZ        do{RFST = 0xC5;                       }while(0)   //CC2530(0xBF>0xC5)
#define DECY        do{RFST = 0xC4;                       }while(0)   //CC2530(0xBE>0xC4)
#define INCY        do{RFST = 0xC1;                       }while(0)   //CC2530(0xBD>0xC1)
#define INCMAXY(m)  do{RFST = (0xC8 | m);                 }while(0) // m < 8 !!   //CC2530(0xB8>0xC8)
#define RANDXY      do{RFST = 0xBD;                       }while(0)   //CC2530(0xBC>0xBD)
#define INT         do{RFST = 0xBA;                       }while(0)   //CC2530(0xB9>0xBA)
#define WAITX       do{RFST = 0xBC;                       }while(0)   //CC2530(0xBB>0xBC)
#define WAIT(w)     do{RFST = (0x80 | w);                 }while(0) // w < 64 !!
#define WEVENT      do{RFST = 0xB8;                      }while(0)  //CC2530(WEVENT1)
#define WEVENT2      do{RFST = 0xB9;                      }while(0)  //CC2530(WEVENT2)
#define LABEL       do{RFST = 0xBB;                       }while(0)   //CC2530(0xBA>0xBB)
#define RPT(n,c)    do{RFST = (0xA0 | (n << 3) | c);      }while(0) // n = TRUE/FALSE && (c < 8)
#define SKIP(s,n,c) do{RFST = ((s << 4) | (n << 3) | c);  }while(0) // && (s < 8)
#define STOP        do{RFST = 0xD2;                       }while(0)   //CC2530(0xDF>0xD2)
#define SNOP        do{RFST = 0xD0;                       }while(0)   //CC2530(0xC0>0xD0)
//#define STXCALN     do{RFST = 0xC1;                       }while(0)   //CC2530
#define SRXON       do{RFST = 0xD3;                       }while(0)  //CC2530(0xC2>0xD3)
#define STXON       do{RFST = 0xD9;                       }while(0)   //CC2530(0xC3>0xD9)
#define STXONCCA    do{RFST = 0xDA;                       }while(0)   //CC2530(0xC4>0xDA)
#define SRFOFF      do{RFST = 0xDF;                       }while(0)   //CC2530(0xC5>0xDF)
#define SFLUSHRX    do{RFST = 0xDD;                       }while(0)   //CC2530(0xC6>0xDD)
#define SFLUSHTX    do{RFST = 0xDE;                       }while(0)   //CC2530(0xC7>0xDE)
#define SACK        do{RFST = 0xD6;                       }while(0)   //CC2530(0xC8>0xD6)
#define SACKPEND    do{RFST = 0xD7;                       }while(0)   //CC2530(0xC9>0xD7)
#define ISSTOP      do{RFST = 0xE2;                       }while(0)   //CC2530(0xFF>0xE2)
#define ISSTART     do{RFST = 0xE1;                       }while(0)   //CC2530(0xFE>0xE1)
//#define ISTXCALN    do{RFST = 0xE1;                       }while(0)   //CC2530
#define ISRXON      do{RFST = 0xE3;                       }while(0)   //CC2530(0xE2>0xE3)
#define ISTXON      do{RFST = 0xE9;                       }while(0)   //CC2530(0xE3>0xE9)
#define ISTXONCCA   do{RFST = 0xEA;                       }while(0)   //CC2530(0xE4>0xEA)
#define ISRFOFF     do{RFST = 0xEF;                       }while(0)   //CC2530(0xE5>0xEF)
#define ISFLUSHRX   do{RFST = 0xED;                       }while(0)   //CC2530(0xE6>0xED)
#define ISFLUSHTX   do{RFST = 0xEE;                       }while(0)   //CC2530(0xE7>0xEE)
#define ISACK       do{RFST = 0xE6;                       }while(0)   //CC2530(0xE8>0xE6)
#define ISACKPEND   do{RFST = 0xE7;                       }while(0)   //CC2530(0xE9>0xE7)

#define PACKET_FOOTER_SIZE 2    //bytes after the payload,is FCS length


/******************************************************************************
*******************              Utility functions          *******************
******************************************************************************/

/******************************************************************************
* @fn  halWait
*
* @brief
*      This function waits approximately a given number of m-seconds
*      regardless of main clock speed.
*
* Parameters:
*
* @param  BYTE	 wait
*         The number of m-seconds to wait.
*
* @return void
*
******************************************************************************/
//software delay, waits is in milliseconds




/******************************************************************************
*******************             ADC macros/functions        *******************
*******************************************************************************

These functions/macros simplifies usage of the ADC.

******************************************************************************/
// Macro for setting up a single conversion. If ADCCON1.STSEL = 11, using this
// macro will also start the conversion.Configure reference voltage, decimation rate and input channel
#define ADC_SINGLE_CONVERSION(settings) \
   do{ ADCCON3 = settings; }while(0)//(*)选择参考电压，分辨率，通道  

// Macro for setting up a single conversion.Configure reference voltage, decimation rate and input channel
#define ADC_SEQUENCE_SETUP(settings) \
   do{ ADCCON2 = settings; }while(0)

// Where _settings_ are the following:
// Reference voltage:
#define ADC_REF_1_25_V      0x00     // Internal 1.25V reference
#define ADC_REF_P0_7        0x40     // External reference on AIN7 pin
#define ADC_REF_AVDD        0x80     // AVDD_SOC pin
#define ADC_REF_P0_6_P0_7   0xC0     // External reference on AIN6-AIN7 differential input

// Resolution (decimation rate):
#define ADC_8_BIT           0x00     //  64 decimation rate(8 bits resolution)
#define ADC_10_BIT          0x10     // 128 decimation rate(10 bits resolution)
#define ADC_12_BIT          0x20     // 256 decimation rate(12 bits resolution)
#define ADC_14_BIT          0x30     // 512 decimation rate(14 bits resolution)
// Input channel:
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
/*
#define ADC_AIN01			0x08	//AIN0-AIN1
#define ADC_AIN23			0x09	//AIN2-AIN3
#define ADC_AIN45			0x0A	//AIN4-AIN5
#define ADC_AIN67			0x0B	//AIN6-AIN7
#define ADC_POSITIVE_VOLTAGE	0x0D	//Positive voltage reference
*/


//-----------------------------------------------------------------------------
// Macro for starting the ADC in continuous conversion mode.Full speed.Do not wait for triggers
#define ADC_SAMPLE_CONTINUOUS() \
   do { ADCCON1 &= ~0x30; ADCCON1 |= 0x10; } while (0)

// Macro for stopping the ADC in continuous mode (and setting the ADC to be
// started manually by ADC_SAMPLE_SINGLE() )ADCCON1.ST=1
#define ADC_STOP() \
  do { ADCCON1 |= 0x30; } while (0)

// Macro for initiating a single sample in single-conversion mode (ADCCON1.STSEL = 11).ADCCON1.ST=1
#define ADC_SAMPLE_SINGLE() \
  do { ADC_STOP(); ADCCON1 |= 0x40;  } while (0)//(*)开始转换 

// Macro for configuring the ADC to be started from T1 channel 0 compare event. (T1 ch 0 must be in compare mode!!)
#define ADC_TRIGGER_FROM_TIMER1()  do { ADC_STOP(); ADCCON1 &= ~0x10;  } while (0)

// Expression indicating whether a conversion is finished or not.
#define ADC_SAMPLE_READY()  (ADCCON1 & 0x80)//(*)

// Macro for setting/clearing a channel as input of the ADC,ch is from 0 to 7
#define ADC_ENABLE_CHANNEL(ch)   ADCCFG |=  (0x01<<ch)//(*)选择AD转换通道
#define ADC_DISABLE_CHANNEL(ch)  ADCCFG &= ~(0x01<<ch)



/******************************************************************************
* @fn  halAdcSampleSingle
*
* @brief
*      This function makes the adc sample the given channel at the given
*      resolution with the given reference.
*
* Parameters:
*
* @param BYTE reference
*          The reference to compare the channel to be sampled.
*        BYTE resolution
*          The resolution to use during the sample (8, 10, 12 or 14 bit)
*        BYTE input
*          The channel to be sampled.
*
* @return INT16
*          The conversion result
*
******************************************************************************/
INT16 halAdcSampleSingle(BYTE reference, BYTE resolution, UINT8 input);



/******************************************************************************
* @fn  halGetAdcValue
*
* @brief
*      Returns the result of the last ADC conversion.
*
* Parameters:
*
* @param  void
*
* @return INT16
*         The ADC value
*
******************************************************************************/
INT16 halGetAdcValue(void);


/******************************************************************************
*******************      Power and clock management        ********************
*******************************************************************************

These macros are used to set power-mode, clock source and clock speed.

******************************************************************************/

// Macro for getting the clock division factor
#define CLKSPD  (CLKCONSTA & 0x07)//2530(CLKCON>CLKCONSTA)

// Macro for getting the timer tick division factor.
#define TICKSPD ((CLKCONSTA & 0x38) >> 3)//2530(CLKCON>CLKCONSTA)

// Macro for checking status of the crystal oscillator
#define XOSC_STABLE (!CLKCONSTA & 0x40)//2530(SLEEP>!CLKCONSTA)

// Macro for checking status of the high frequency RC oscillator.
#define HIGH_FREQUENCY_RC_OSC_STABLE    (CLKCONSTA & 0x40)//2530(SLEEP>CLKCONSTA;0x20>0x40)


// Macro for setting power mode
#define SET_POWER_MODE(mode)                   \
   do {                                        \
      if(mode == 0)        { SLEEPCMD &= ~0x03; } \
      else if (mode == 3)  { SLEEPCMD |= 0x03;  } \
      else { SLEEPCMD &= ~0x03; SLEEPCMD |= mode;  } \
      PCON |= 0x01;                            \
      asm("NOP");                              \
   }while (0)
   //2530(SLEEP>SLEEPCMD)


// Where _mode_ is one of
#define POWER_MODE_0  0x00  // Clock oscillators on, voltage regulator on
#define POWER_MODE_1  0x01  // 32.768 KHz oscillator on, voltage regulator on
#define POWER_MODE_2  0x02  // 32.768 KHz oscillator on, voltage regulator off
#define POWER_MODE_3  0x03  // All clock oscillators off, voltage regulator off

// Macro for setting the 32 KHz clock source
#define SET_32KHZ_CLOCK_SOURCE(source) \
   do {                                \
      if( source ) {                   \
         CLKCONCMD |= 0x80;               \
      } else {                         \
         CLKCONCMD &= ~0x80;              \
      }                                \
   } while (0)
   //2530(CLKCON>CLKCONSCMD)

// Where _source_ is one of
#define CRYSTAL 0x00//Crystal oscillator
#define RC      0x01//RC oscillator

// Macro for setting the main clock oscillator source,
//turns off the clock source not used
//changing to XOSC will take approx 150 us

/*
#define SET_MAIN_CLOCK_SOURCE(source) \
   do {                               \
      if(source) {                    \
        CLKCON |= 0x40;               \
        while(!HIGH_FREQUENCY_RC_OSC_STABLE); \
        if(TICKSPD == 0){             \
          CLKCON |= 0x08;             \
        }                             \
        SLEEP |= 0x04;                \
      }                               \
      else {                          \
        while(!XOSC_STABLE);          \
        asm("NOP");                   \
        CLKCON &= ~0x47;              \
        SLEEP |= 0x04;                \
      }                               \
   }while (0)
*/

#define SET_MAIN_CLOCK_SOURCE(source) \
   do {                               \
      if(source) {                    \
        CLKCONCMD = 0x49;             \
        while(CLKCONSTA!=0x49);       \
      }                               \
      else {                          \
        CLKCONCMD = 0x88;           \
         while(CLKCONSTA!=0x88);      \
      }                               \
        SLEEPCMD |= 0x04;             \
   }while (0)


#endif
