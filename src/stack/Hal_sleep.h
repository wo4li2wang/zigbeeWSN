#ifndef HAL_SLEEP_H
#define HAL_SLEEP_H

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Execute power management procedure
 */
extern void halSleep( UINT16 osal_timer );

/*
 * Used in mac_mcu
 */
extern void halSleepWait(UINT16 duration);

/*
 * Used in hal_drivers, AN044 - DELAY EXTERNAL INTERRUPTS
 */
extern void halRestoreSleepLevel( void );

#endif
