#ifndef COMPILER_H
#define COMPILER_H

//compatible with both IAR8051 compiler

#ifdef __IAR_SYSTEMS_ICC__
#define IAR8051
#include <stdio.h>
#endif

/******************************************************************************
*******************              Commonly used types        *******************
******************************************************************************/
typedef unsigned char       BOOL;

// Data
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

// Unsigned numbers
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned long       UINT32;

// Signed numbers
typedef signed char         INT8;
typedef signed short        INT16;
typedef signed long         INT32;



//Define these compiler labels

//uncomment this if compiler is BIG ENDIAN
//define LWRPAN_COMPILER_BIG_ENDIAN

#if defined(IAR8051)

#define INTERRUPT_FUNC __interrupt static void

//char type in ROM memory
typedef  unsigned char const __code    ROMCHAR ;

#endif

#if defined(HI_TECH_C)
#include <intrpt.h>
#define LWRPAN_COMPILER_NO_RECURSION/*Ã»ÓÐµÝ¹é*/
#define INTERRUPT_FUNC interrupt static void
typedef  unsigned char const ROMCHAR ;
#include "cdefs.h"    //this has all #defines since demo C51 disables -D flag

#endif

#endif
