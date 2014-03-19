#ifndef WXL_LRWPAN_H
#define WXL_LRWPAN_H

typedef unsigned char       INT8U;
typedef unsigned short       INT16U;
typedef unsigned long       INT32U;

#include "compiler.h"               //compiler specific

#include "lrwpan_common_types.h"   //types common acrosss most files
#include "hal.h"
#include "memalloc.h"
#include "neighbor.h"
#include "halStack.h"
#include "evboard.h"
#include "ieee_lrwpan_defs.h"
#include "phy.h"
#include "mac.h"
#include "nwk.h"
#include "aps.h"
#include "mib.h"
#ifdef SECURITY_GLOBALS
#include "security.h"
#endif
#include "user.h"
#include "Hal_sleep.h"
/******************************************************************************
LCD I/O
*******************************************************************************/
#define	LCD_RST     	P2_0
#define	LCD_RS      	P1_1
#define	LCD_CLK     	P1_5
#define	LCD_SDO     	P1_6
#define	LCD_CS      	P1_4



#endif
