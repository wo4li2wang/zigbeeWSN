#ifndef MAC_RADIO_DEFS_H
#define MAC_RADIO_DEFS_H

/* ------------------------------------------------------------------------------------------------
 *                                             Includes
 * ------------------------------------------------------------------------------------------------
 */



/* ------------------------------------------------------------------------------------------------
 *                                      Target Specific Defines
 * ------------------------------------------------------------------------------------------------
 */


/* FSCTRLL */
#define FREQ_2405MHZ                  0x65

/* T2CNF */
#define CMPIF           BV(7)
#define PERIF           BV(6)
#define OFCMPIF         BV(5)
#define SYNC            BV(1)
#define RUN             BV(0)


/* MDMCTRL1L */
#define MDMCTRL1L_RESET_VALUE         0x00
#define RX_MODE(x)                    ((x) << 0)
#define RX_MODE_INFINITE_RECEPTION    RX_MODE(2)
#define RX_MODE_NORMAL_OPERATION      RX_MODE(0)

/* FSMSTATE */
#define FSM_FFCTRL_STATE_RX_INF       31      /* infinite reception state - not documented in datasheet */

/* RFPWR */
#define ADI_RADIO_PD                  BV(4)
#define RREG_RADIO_PD                 BV(3)

/* ADCCON1 */
#define RCTRL1                        BV(3)
#define RCTRL0                        BV(2)
#define RCTRL_BITS                    (RCTRL1 | RCTRL0)
#define RCTRL_CLOCK_LFSR              RCTRL0

/* FSMTC1 */
#define ABORTRX_ON_SRXON              BV(5)
#define PENDING_OR                    BV(1)

/* CSPCTRL */
#define CPU_CTRL                      BV(0)
#define CPU_CTRL_ON                   CPU_CTRL
#define CPU_CTRL_OFF                  (!(CPU_CTRL))


/* ------------------------------------------------------------------------------------------------
 *                                      Common Radio Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MAC_RADIO_TURN_ON_POWER()                   st(;) //CC2530// st( RFPWR &= ~RREG_RADIO_PD; while((RFPWR & ADI_RADIO_PD)); )
#define MAC_RADIO_TURN_OFF_POWER()                  st(;) // CC2530//st( RFPWR |=  RREG_RADIO_PD; )

#define MAC_RADIO_RX_FIFO_HAS_OVERFLOWED()            ((RFSTATUS & FIFOP) && !(RFSTATUS & FIFO))
#define MAC_RADIO_RX_FIFO_IS_EMPTY()                  (!(RFSTATUS & FIFO) && !(RFSTATUS & FIFOP))

#define MAC_RADIO_SET_RX_THRESHOLD(x)                 st( FIFOPCTRL = ((x)-1); )    //CC2530(IOCFG0>FIFOPCTRL)
#define MAC_RADIO_RX_IS_AT_THRESHOLD()                (RFSTATUS & FIFOP)
#define MAC_RADIO_ENABLE_RX_THRESHOLD_INTERRUPT()     MAC_MCU_FIFOP_ENABLE_INTERRUPT()
#define MAC_RADIO_DISABLE_RX_THRESHOLD_INTERRUPT()    MAC_MCU_FIFOP_DISABLE_INTERRUPT()
#define MAC_RADIO_CLEAR_RX_THRESHOLD_INTERRUPT_FLAG() st( RFIRQF0 = ~IRQ_FIFOP; )       //CC2530(RFIF>RFIRQF0)   

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : work around for chip bug #267, replace these lines with following.
#ifndef _REMOVE_REV_B_WORKAROUNDS
#define MAC_RADIO_TX_ACK()                           st(;) //st( FSMTC1 &= ~PENDING_OR; )//CC2530
#define MAC_RADIO_TX_ACK_PEND()                      st(;) //st( FSMTC1 |=  PENDING_OR; )//CC2530
#else
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//  keep this code, delete the rest
#define MAC_RADIO_TX_ACK()                            st( RFST = ISACK; )
#define MAC_RADIO_TX_ACK_PEND()                       st( RFST = ISACKPEND; )
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAC_RADIO_RX_ON()           ISRXON
#define MAC_RADIO_RXTX_OFF()        ISRFOFF
#define MAC_RADIO_FLUSH_RX_FIFO()   ISFLUSHRX; ISFLUSHRX
#define MAC_RADIO_FLUSH_TX_FIFO()   ISFLUSHTX


#define MAC_RADIO_SET_PAN_COORDINATOR(b)              st( FRMFILT0 = (FRMFILT0 & ~PAN_COORDINATOR) | (PAN_COORDINATOR * (b!=0)); )    //CC2530(MDMCTRL0H>FRMFILT0)
#define MAC_RADIO_SET_CHANNEL(x)                      st( FSCTRLL = FREQ_2405MHZ + 5 * ((x) - 11); )
#define MAC_RADIO_SET_TX_POWER(x)                     st( TXCTRLL = x; )

#define MAC_RADIO_READ_PAN_ID()                       (PANIDL + (macMemReadRamByte(&PANIDH) << 8))
#define MAC_RADIO_SET_PAN_ID(x)                       st( PANIDL = (x) & 0xFF; PANIDH = (x) >> 8; )
#define MAC_RADIO_SET_SHORT_ADDR(x)                   st( SHORTADDRL = (x) & 0xFF; SHORTADDRH = (x) >> 8; )
#define MAC_RADIO_SET_IEEE_ADDR(p)                    macMemWriteRam((macRam_t *) &IEEE_ADDR0, p, 8)

#define MAC_RADIO_REQUEST_ACK_TX_DONE_CALLBACK()      st( MAC_MCU_TXDONE_CLEAR_INTERRUPT(); MAC_MCU_TXDONE_ENABLE_INTERRUPT(); )
#define MAC_RADIO_CANCEL_ACK_TX_DONE_CALLBACK()       MAC_MCU_TXDONE_DISABLE_INTERRUPT()

#define MAC_RADIO_RANDOM_BYTE()                       macMcuRandomByte()

#define MAC_RADIO_TX_RESET()                          macCspTxReset()
#define MAC_RADIO_TX_PREP_CSMA_UNSLOTTED()            macCspTxPrepCsmaUnslotted()
#define MAC_RADIO_TX_PREP_CSMA_SLOTTED()              macCspTxPrepCsmaSlotted()
#define MAC_RADIO_TX_PREP_SLOTTED()                   macCspTxPrepSlotted()
#define MAC_RADIO_TX_GO_CSMA()                        macCspTxGoCsma()
#define MAC_RADIO_TX_GO_SLOTTED()                     macCspTxGoSlotted()

#define MAC_RADIO_FORCE_TX_DONE_IF_PENDING()          macCspForceTxDoneIfPending()

#define MAC_RADIO_TX_REQUEST_ACK_TIMEOUT_CALLBACK()   macCspTxRequestAckTimeoutCallback()
#define MAC_RADIO_TX_CANCEL_ACK_TIMEOUT_CALLBACK()    macCspTxCancelAckTimeoutCallback()

#define MAC_RADIO_TIMER_TICKS_PER_USEC()              HAL_CPU_CLOCK_MHZ /* never fractional */
#define MAC_RADIO_TIMER_TICKS_PER_BACKOFF()           (HAL_CPU_CLOCK_MHZ * MAC_SPEC_USECS_PER_BACKOFF)
#define MAC_RADIO_TIMER_TICKS_PER_SYMBOL()            (HAL_CPU_CLOCK_MHZ * MAC_SPEC_USECS_PER_SYMBOL)

#define MAC_RADIO_TIMER_CAPTURE()                     macMcuTimerCapture()
#define MAC_RADIO_TIMER_FORCE_DELAY(x)                st( T2TLD = (x) & 0xFF;  T2THD = (x) >> 8; )  /* timer must be running */

#define MAC_RADIO_TIMER_SLEEP()                       st( T2CTRL &= ~RUN; while (T2CTRL & RUN); )    //CC2530(T2CNF>T2CTRL)
#define MAC_RADIO_TIMER_WAKE_UP()                     st( T2CTRL |=  RUN; while (!(T2CTRL & RUN)); )   //CC2530(T2CNF>T2CTRL)

#define MAC_RADIO_BACKOFF_COUNT()                     macMcuOverflowCount()
#define MAC_RADIO_BACKOFF_CAPTURE()                   macMcuOverflowCapture()
#define MAC_RADIO_BACKOFF_SET_COUNT(x)                macMcuOverflowSetCount(x)
#define MAC_RADIO_BACKOFF_SET_COMPARE(x)              macMcuOverflowSetCompare(x)

#define MAC_RADIO_BACKOFF_COMPARE_CLEAR_INTERRUPT()   st( T2CTRL = RUN | SYNC; T2IRQF=&0xef)    //CC2530(T2CNF>T2CTRL;*)
#define MAC_RADIO_BACKOFF_COMPARE_ENABLE_INTERRUPT()  macMcuOrT2PEROF2(OFCMPIM)
#define MAC_RADIO_BACKOFF_COMPARE_DISABLE_INTERRUPT() macMcuAndT2PEROF2(~OFCMPIM)

#define MAC_RADIO_RECORD_MAX_RSSI_START()             macMcuRecordMaxRssiStart()
#define MAC_RADIO_RECORD_MAX_RSSI_STOP()              macMcuRecordMaxRssiStop()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  REV_B_WORKAROUND : work around for chip bug #267, autoack is used as work around, need to disble for promiscuous mode
//  easiest to do here as part of address recognition since bug is exclusive to 2430.  Replace these lines with following
//  when Rev B is obsoleted.
#ifndef _REMOVE_REV_B_WORKAROUNDS
#define MAC_RADIO_TURN_ON_RX_FRAME_FILTERING()        st( MFRMFILT0 |=  ADDR_DECODE; FRMCTRL0 |=  AUTOACK; )     //CC2530(MDMCTRL0H>FRMFILT0;MDMCTRL0L>FRMCTRL0)
#define MAC_RADIO_TURN_OFF_RX_FRAME_FILTERING()       st( FRMFILT0 &= ~ADDR_DECODE; FRMCTRL0 &= ~AUTOACK; )    //CC2530(MDMCTRL0H>FRMFILT0;MDMCTRL0L>FRMCTRL0)
#else
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//  keep this code, delete the rest
#define MAC_RADIO_TURN_ON_RX_FRAME_FILTERING()        st( FRMFILT0 |=  ADDR_DECODE; )   //CC2530(MDMCTRL0H>FRMFILT0)
#define MAC_RADIO_TURN_OFF_RX_FRAME_FILTERING()       st( FRMFILT0 &= ~ADDR_DECODE; )   //CC2530(MDMCTRL0H>FRMFILT0)
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif
