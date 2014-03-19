#ifndef MAC_SLEEP_H
#define MAC_SLEEP_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_SLEEP_STATE_AWAKE       0x00

/* macSleep() parameter values for sleepState */
#define MAC_SLEEP_STATE_OSC_OFF             0x01
#define MAC_SLEEP_STATE_RADIO_OFF           0x02

#define MAC_RADIO_RX_ON()           ISRXON
#define MAC_RADIO_RXTX_OFF()        ISRFOFF
#define MAC_RADIO_FLUSH_RX_FIFO()   ISFLUSHRX; ISFLUSHRX
#define MAC_RADIO_FLUSH_TX_FIFO()   ISFLUSHTX

void macSleepWakeUp(void);
BOOL macSleep(void);
#endif
