//*****************************************************************************
//
// ringbuf.h - Defines and Macros for the ring buffer utilities.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
// 130728: LGT: Adapted for a base-type of an int (32 bit) instead of a char
//
//*****************************************************************************

#ifndef __RINGBUF_H__
#define __RINGBUF_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// The structure used for encapsulating all the items associated with a
// ring buffer.
//
//*****************************************************************************
typedef unsigned int UINT;

typedef struct
{
    unsigned long          ulSize;		 // The ring buffer size
    volatile unsigned long ulWriteIndex; // The ring buffer write index.
    volatile unsigned long ulReadIndex;  // The ring buffer read index.
    UINT                   *pucBuf;      // The ring buffer.
} tRingBufObject;

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern bool RingBufFull(tRingBufObject *ptRingBuf);
extern bool RingBufEmpty(tRingBufObject *ptRingBuf);
extern void RingBufFlush(tRingBufObject *ptRingBuf);
extern unsigned long RingBufUsed(tRingBufObject *ptRingBuf);
extern unsigned long RingBufFree(tRingBufObject *ptRingBuf);
extern unsigned long RingBufContigUsed(tRingBufObject *ptRingBuf);
extern unsigned long RingBufContigFree(tRingBufObject *ptRingBuf);
extern unsigned long RingBufSize(tRingBufObject *ptRingBuf);
extern UINT RingBufReadOne(tRingBufObject *ptRingBuf);
extern void RingBufRead(tRingBufObject *ptRingBuf, UINT *pucData, unsigned long ulLength);
extern void RingBufWriteOne(tRingBufObject *ptRingBuf, UINT ucData);
extern void RingBufWrite(tRingBufObject *ptRingBuf, UINT *pucData, unsigned long ulLength);
extern void RingBufAdvanceWrite(tRingBufObject *ptRingBuf, unsigned long ulNumBytes);
extern void RingBufAdvanceRead(tRingBufObject *ptRingBuf, unsigned long ulNumBytes);
extern void RingBufInit(tRingBufObject *ptRingBuf, UINT *pucBuf, unsigned long ulSize);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __RINGBUF_H__
