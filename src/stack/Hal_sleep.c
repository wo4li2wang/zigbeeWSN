/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal.h"
#include "halStack.h"
#include "mac.h"
#include "Mac_sleep.h"
#include "evboard.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* 32 kHz clock source select in CLKCONSTA */
//#define OSC32K_CRYSTAL_INSTALLED
#if defined (OSC32K_CRYSTAL_INSTALLED)
#define OSC_32KHZ                        0x00  /* external 32 KHz xosc */
#else
#define OSC_32KHZ                        0x80  /* internal 32 KHz rcosc */
#endif

/* POWER CONSERVATION DEFINITIONS
 * Sleep mode H/W definitions (enabled with POWER_SAVING compile option)
 */
#define CC2430_PM0            0  /* PM0, Clock oscillators on, voltage regulator on */
#define CC2430_PM1            1  /* PM1, 32.768 kHz oscillators on, voltage regulator on */
#define CC2430_PM2            2  /* PM2, 32.768 kHz oscillators on, voltage regulator off */
#define CC2430_PM3            3  /* PM3, All clock oscillators off, voltage regulator off */

/* HAL power management mode is set according to the power management state. The default
 * setting is HAL_SLEEP_OFF. The actual value is tailored to different HW platform. Both
 * HAL_SLEEP_TIMER and HAL_SLEEP_DEEP selections will:
 *   1. turn off the system clock, and
 *   2. halt the MCU.
 * HAL_SLEEP_TIMER can be woken up by sleep timer interrupt, I/O interrupt and reset.
 * HAL_SLEEP_DEEP can be woken up by I/O interrupt and reset.
 */
#define HAL_SLEEP_OFF         CC2430_PM0
#define HAL_SLEEP_TIMER       CC2430_PM2
#define HAL_SLEEP_DEEP        CC2430_PM3

/* MAX_SLEEP_TIME calculation:
 *   Sleep timer maximum duration = 0xFFFF7F / 32768 Hz = 511.996 seconds
 *   Round it to 510 seconds or 510000 ms
 */
#define MAX_SLEEP_TIME                   510000             /* maximum time to sleep allowed by ST */

/* minimum time to sleep, this macro is to:
 * 1. avoid thrashing in-and-out of sleep with short OSAL timer (~2ms)
 * 2. define minimum safe sleep period for different CC2430 revisions
 * AN044 - MINIMUM SLEEP PERIODS WITH PULL-DOWN RESISTOR
 */
#define PM_MIN_SLEEP_TIME                14                 /* default to minimum safe sleep time for CC2430 Rev B */

/* This value is used to adjust the sleep timer compare value such that the sleep timer
 * compare takes into account the amount of processing time spent in function halSleep().
 * The first value is determined by measuring the number of sleep timer ticks it from
 * the beginning of the function to entering sleep mode.  The second value is determined
 * by measuring the number of sleep timer ticks from exit of sleep mode to the call to
 * osal_adjust_timers().
 */
#define HAL_SLEEP_ADJ_TICKS   (9 + 25 +40)

#define CLEAR_SLEEP_MODE() SLEEPCMD &= ~0x03

/* set CC2430 power mode; always use PM2 */
#define HAL_SLEEP_SET_POWER_MODE(mode)                          \
    do {                                                        \
        SLEEPCMD &= ~0x03;  /* clear mode bits */                 \
        SLEEPCMD |= mode;   /* set mode bits   */                  \
        asm("NOP");                                             \
        asm("NOP");                                             \
        asm("NOP");                                             \
        if( SLEEPCMD & 0x03 ) {                                    \
            PCON |= 0x01;  /* enable mode */                    \
            asm("NOP");    /* first instruction after sleep*/   \
        }                                                       \
    } while (0)
    //2530(SLEEP>SLEEPCMD)

/* set main clock source to crystal (exit sleep) */
#define HAL_SLEEP_SET_MAIN_CLOCK_CRYSTAL()                  \
    do {                                                    \
        while(!(SLEEPCMD & 0x40));  /* wait for XOSC */        \
        asm("NOP");                                         \
        CLKCONCMD = (0x00 | OSC_32KHZ);   /* 32MHx XOSC */     \
        while (CLKCONCMD != (0x00 | OSC_32KHZ));               \
        SLEEPCMD |= 0x04;)          /* turn off 16MHz RC */    \
    } while (0)
    //2530(SLEEP>SLEEPCMD)
    //2530(CLKCON>CLKCONCMD)

/* set main clock source to RC oscillator (enter sleep) */
#define HAL_SLEEP_SET_MAIN_CLOCK_RC()                       \
    do {                                                    \
        while(!(SLEEPCMD & 0x20));  /* wait for RC osc */      \
        asm("NOP");                                         \
        CLKCONCMD = (0x49 | OSC_32KHZ);  /* select RC osc */   \
        while (CLKCONCMD != (0x49 | OSC_32KHZ));               \
        SLEEPCMD |= 0x04;)           /* turn off XOSC */       \
    } while (0)
     //2530(SLEEP>SLEEPCMD)
    //2530(CLKCON>CLKCONCMD)

/* sleep timer interrupt control */
#define HAL_SLEEP_TIMER_ENABLE_INT()        IEN0 |= (0x01<<5)     /* enable sleep timer interrupt */
#define HAL_SLEEP_TIMER_DISABLE_INT()       IEN0 &= ~(0x01<<5)    /* disable sleep timer interrupt */
#define HAL_SLEEP_TIMER_CLEAR_INT()         IRCON &= ~0x80        /* clear sleep interrupt flag */

/* backup interrupt enable registers before sleep */
#define HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2)     \
    do {                                                      \
        ien0  = IEN0;    /* backup IEN0 register */           \
        ien1  = IEN1;    /* backup IEN1 register */           \
        ien2  = IEN2;    /* backup IEN2 register */           \
        IEN0 &= (0x01<<5); /* disable IEN0 except STIE */     \
        IEN1 &= 0x00; /* disable IEN1 */                      \
        IEN2 &= 0x00; /* disable IEN2 */                      \
    } while (0)

/* restore interrupt enable registers before sleep */
#define HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2)     \
    do {                                           \
        IEN0 = ien0;   /* restore IEN0 register */ \
        IEN1 = ien1;   /* restore IEN1 register */ \
        IEN2 = ien2;   /* restore IEN2 register */ \
    } while (0)



/* Internal (MCU) Stack addresses. This is to check if the stack is exceeding the disappearing
 * RAM boundary of 0xF000. If the stack does exceed the boundary (unlikely), do not enter sleep
 * until the stack is back to normal.
 */
#define CSTK_PTR _Pragma("segment=\"XSP\"") __segment_begin("XSP")

/* convert msec to 320 usec units with round */
#define HAL_SLEEP_MS_TO_320US(ms)           (((((UINT32) (ms)) * 100) + 31) / 32)

/* convert msec to 16 usec units with round */
#define HAL_SLEEP_MS_TO_16US(ms)           (((((UINT32) (ms)) * 1000) + 15) / 16)
/* ------------------------------------------------------------------------------------------------
 *                                        Local Variables
 * ------------------------------------------------------------------------------------------------
 */

/* HAL power management mode is set according to the power management state.
 */
static BYTE halPwrMgtMode = HAL_SLEEP_OFF;

/* stores the sleep timer count upon entering sleep */
static UINT32 halSleepTimerStart;

/* stores the accumulated sleep time */
//static UINT32 halAccumulatedSleepTime;

/* stores the deepest level the device is allowed to sleep */
static BYTE halSleepLevel = CC2430_PM2;

/* ------------------------------------------------------------------------------------------------
 *                                      Function Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void halSleepSetTimer(UINT32 timeout);
UINT32 HalTimerElapsed( void );

/**************************************************************************************************
 * @fn          halSleep
 *
 * @brief       This function is called from the OSAL task loop using and existing OSAL
 *              interface.  It sets the low power mode of the MAC and the CC2430.
 *
 * input parameters
 *
 * @param       osal_timeout - Next OSAL timer timeout.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halSleep( UINT16 osal_timeout )
{
    UINT32 timeout;
    BYTE gie_status,ien0,ien1,ien2;

    //halAccumulatedSleepTime = 0;
    //get next OSAL timer expiration converted to 320 usec units
    //timeout = HAL_SLEEP_MS_TO_320US(osal_timeout);
    timeout = (UINT32)osal_timeout;

    //HAL_SLEEP_PM2 is entered only if the timeout is zero and the device is a stimulated device.
    halPwrMgtMode = (timeout == 0) ? HAL_SLEEP_DEEP : HAL_SLEEP_TIMER;
    //The sleep mode is also controlled by halSleepLevel which defined the deepest level of sleep allowed.
    //This is applied to timer sleep only.

    //set main clock source to RC oscillator
    //HAL_SLEEP_SET_MAIN_CLOCK_RC();
    SAVE_AND_DISABLE_GLOBAL_INTERRUPT(gie_status);
    //enable sleep timer interrupt
    if (timeout != 0) {
        if (timeout > MAX_SLEEP_TIME) {
            timeout -= MAX_SLEEP_TIME;
            halSleepSetTimer(MAX_SLEEP_TIME);
        } else {
            //set sleep timer
            halSleepSetTimer(timeout);
        }
        //set up sleep timer interrupt
        HAL_SLEEP_TIMER_CLEAR_INT();
        HAL_SLEEP_TIMER_ENABLE_INT();
    }
    if ( timeout > 0 && halPwrMgtMode > halSleepLevel ) {
        halPwrMgtMode = halSleepLevel;
    }

    //save interrupt enable registers and disable all interrupts
    HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2);
    //This is to check if the stack is exceeding the disappearing RAM boundary of 0xF000.
    //If the stack does exceed the boundary (unlikely), do not enter sleep until the stack is back to normal.
    //if ( ((UINT16)(*( __idata UINT16*)(CSTK_PTR)) >= 0xF000) ) {
    if(1) {
        EVB_LED1_OFF();
        EVB_LED2_OFF();
        //HAL_EXIT_CRITICAL_SECTION(intState);
        ENABLE_GLOBAL_INTERRUPT();

        macSleep();

        //set CC2430 power mode
        HAL_SLEEP_SET_POWER_MODE(halPwrMgtMode);

        //wake up from sleep
        macSleepWakeUp();

        //HAL_ENTER_CRITICAL_SECTION(intState);
        DISABLE_GLOBAL_INTERRUPT();
        EVB_LED2_ON();

    }
    //restore interrupt enable registers
    HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2);
    //disable sleep timer interrupt
    HAL_SLEEP_TIMER_DISABLE_INT();
    //set main clock source to crystal
    //HAL_SLEEP_SET_MAIN_CLOCK_CRYSTAL();

    //Calculate timer elasped
    //halAccumulatedSleepTime += HalTimerElapsed();
    RESTORE_GLOBAL_INTERRUPT(gie_status);
}

/**************************************************************************************************
 * @fn          halSleepSetTimer
 *
 * @brief       This function sets the CC2430 sleep timer compare value.  First it reads and
 *              stores the value of the sleep timer; this value is used later to update OSAL
 *              timers.  Then the timeout value is converted from 320 usec units to 32 kHz
 *              period units and the compare value is set to the timeout.
 *
 * input parameters
 *
 * @param       timeout - Timeout value in 320 usec units.  The sleep timer compare is set to
 *                        this value.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halSleepSetTimer(UINT32 timeout)
{
    UINT32 ticks;
    //read the sleep timer; ST0 must be read first
    ((BYTE *) &ticks)[UINT32_NDX0] = ST0;
    ((BYTE *) &ticks)[UINT32_NDX1] = ST1;
    ((BYTE *) &ticks)[UINT32_NDX2] = ST2;
    ((BYTE *) &ticks)[UINT32_NDX3] = 0;
    //store value for later
    halSleepTimerStart = ticks;
    //Compute sleep timer compare value.  The ratio of 32 kHz ticks to 320 usec ticks is 32768/3125 = 10.48576.
    //This is nearly 671/64 = 10.484375.
    //ticks += (timeout * 671) / 64;
    ticks += (timeout * 4096) / 125;
    //subtract the processing time spent in function halSleep()
    ticks -= HAL_SLEEP_ADJ_TICKS;
    //compare value must not be set higher than 0xFFFF7F
    if((ticks & 0x00FFFFFF) > 0x00FFFF7F) {
        ticks = 0xFFFF7F;
    }
    //set sleep timer compare; ST0 must be written last
    ST2 = ((BYTE *) &ticks)[UINT32_NDX2];
    ST1 = ((BYTE *) &ticks)[UINT32_NDX1];
    ST0 = ((BYTE *) &ticks)[UINT32_NDX0];
}

/**************************************************************************************************
 * @fn          TimerElapsed
 *
 * @brief       Determine the number of OSAL timer ticks elapsed during sleep.
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * None.
 *
 * @return      Number of timer ticks elapsed during sleep.
 **************************************************************************************************
 */
/*
UINT32 TimerElapsed( void )
{
    return ( halAccumulatedSleepTime );
}
*/
/**************************************************************************************************
 * @fn          HalTimerElapsed
 *
 * @brief       Determine the number of OSAL timer ticks elapsed during sleep.  This function
 *              relies on OSAL macro TICK_COUNT to be set to 1; then ticks are calculated in
 *              units of msec.  (Setting TICK_COUNT to 1 avoids a costly uint32 divide.)
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * None.
 *
 * @return      Number of timer ticks elapsed during sleep.
 **************************************************************************************************
 */
UINT32 HalTimerElapsed( void )
{
    UINT32 ticks;

    //read the sleep timer; ST0 must be read first
    ((BYTE *) &ticks)[UINT32_NDX0] = ST0;
    ((BYTE *) &ticks)[UINT32_NDX1] = ST1;
    ((BYTE *) &ticks)[UINT32_NDX2] = ST2;
    //set bit 24 to handle wraparound
    ((BYTE *) &ticks)[UINT32_NDX3] = 0x01;

    //calculate elapsed time
    ticks -= halSleepTimerStart;

    //add back the processing time spent in function halSleep()
    ticks = ticks + HAL_SLEEP_ADJ_TICKS;

    //mask off excess if no wraparound
    ticks &= 0x00FFFFFF;
    //Convert elapsed time in milliseconds with round.  1000/32768 = 125/4096
    //return ( ((ticks * 125) + 4095) / 4096 );
    return (ticks + ((ticks*297)/328));//16us
}

/**************************************************************************************************
 * @fn          halSleepWait
 *
 * @brief       Perform a blocking wait.
 *
 * input parameters
 *
 * @param       duration - Duration of wait in microseconds.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halSleepWait(UINT16 duration)
{
    while (duration--)
    {
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
        asm("NOP");
    }
}

/**************************************************************************************************
 * @fn          halRestoreSleepLevel
 *
 * @brief       Restore the deepest timer sleep level.
 *
 * input parameters
 *
 * @param       None
 *
 * output parameters
 *
 *              None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halRestoreSleepLevel( void )
{
    halSleepLevel = CC2430_PM2;
}

/**************************************************************************************************
 * @fn          halSleepTimerIsr
 *
 * @brief       Sleep timer ISR.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
#if defined(IAR8051)
#pragma vector=ST_VECTOR
#endif
#if defined(HI_TECH_C)
ROM_VECTOR(ST_VECTOR,halSleepTimerIsr);
#endif
INTERRUPT_FUNC halSleepTimerIsr( void )
{
      HAL_SLEEP_TIMER_CLEAR_INT();
      CLEAR_SLEEP_MODE();
}

