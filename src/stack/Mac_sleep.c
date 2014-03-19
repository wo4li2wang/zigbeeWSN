/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal.h"
#include "mac.h"
#include "Hal_sleep.h"
#include "Mac_sleep.h"
//#include "Hal_defs.h"
#include "Mac_radio_defs.h"


/* ------------------------------------------------------------------------------------------------
 *                                         Global Variables
 * ------------------------------------------------------------------------------------------------
 */
BYTE macSleepState = MAC_SLEEP_STATE_AWAKE;


/**************************************************************************************************
 * @fn          macSleepWakeUp
 *
 * @brief       Wake up the radio from sleep mode.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void macSleepWakeUp(void)
{
    /* don't wake up radio if it's already awake */
    if (macSleepState == MAC_SLEEP_STATE_AWAKE)
    {
      return;
    }

    /* Calculation of new timer value and overflow count value */
    //halRestoreSystemTimeFromSleep();

    /* wake up MAC timer */
    MAC_RADIO_TIMER_WAKE_UP();

    /* if radio was completely off, restore from that state first */
    if (macSleepState == MAC_SLEEP_STATE_RADIO_OFF)
    {
        /* turn on radio power */
        MAC_RADIO_TURN_ON_POWER();
        /* power-up initialization of receive logic */
        //macRxRadioPowerUpInit();
    }

    /* turn on the oscillator */
    //MAC_RADIO_TURN_ON_OSC();
    /* wait for XOSC is powered up and stable */
    //while (!(SLEEPCMD & 0x40)); //2530(SLEEP>SLEEPCMD)

    /* update sleep state here before requesting to turn on receiver */
    macSleepState = MAC_SLEEP_STATE_AWAKE;

    /* turn on the receiver if enabled */
    //macRxOnRequest();
    ISRXON;//mini
}


/**************************************************************************************************
 * @fn          macSleep
 *
 * @brief       Puts radio into the selected sleep mode.
 *
 * @param       sleepState - selected sleep level, see #defines in .h file
 *
 * @return      TRUE if radio was successfully put into selected sleep mode.
 *              FALSE if it was not safe for radio to go to sleep.
 **************************************************************************************************
 */
BOOL macSleep(void)
{
    BOOL  s;

    /* disable interrupts until macSleepState can be set */
    HAL_ENTER_CRITICAL_SECTION(s);

    /* assert checks */
    //MAC_ASSERT(macSleepState == MAC_SLEEP_STATE_AWAKE); /* radio must be awake to put it to sleep */
    //MAC_ASSERT(macRxFilter == RX_FILTER_OFF); /* do not sleep when scanning or in promiscuous mode */
    if (macSleepState != MAC_SLEEP_STATE_AWAKE)
    {
        HAL_EXIT_CRITICAL_SECTION(s);
        return(FALSE);
    }

    /* if either RX or TX is active or any RX enable flags are set, it's not OK to sleep */
    if (macTXBusy())
    {
        HAL_EXIT_CRITICAL_SECTION(s);
        return(FALSE);
    }

    /* turn off the receiver */
    MAC_RADIO_RXTX_OFF();
    MAC_RADIO_FLUSH_RX_FIFO();

    /* update sleep state variable */
    macSleepState = MAC_SLEEP_STATE_RADIO_OFF;

    /* macSleepState is now set, re-enable interrupts */
    HAL_EXIT_CRITICAL_SECTION(s);

    /* put MAC timer to sleep */
    /* MAC timer 关闭时，
    T2THD:T2TLD自动保存到T2CAPHPH:T2CAPLPL，T2OF2:T2OF1:T2OF0自动保存到T2PEROF2:T2PEROF1:T2PEROF0 */
    MAC_RADIO_TIMER_SLEEP();

    /* put radio in selected sleep mode */
    MAC_RADIO_TURN_OFF_POWER();

    /* radio successfully entered sleep mode */
    return(TRUE);
}

