//*****************************************************************************
//
// ringbuf.c - Ring buffer management utilities.
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

#include <stdint.h>
#include <stdbool.h>
#include "../tivaware/inc/hw_types.h"
#include "../tivaware/driverlib/debug.h"
#include "../tivaware/driverlib/interrupt.h"
#include "ringbuf.h"

//*****************************************************************************
//
//! \addtogroup ringbuf_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// Define NULL, if not already defined.
//
//*****************************************************************************
#ifndef NULL
#define NULL                    ((void *)0)
#endif

//*****************************************************************************
//
// Change the value of a variable atomically.
//
// \param pulVal points to the index whose value is to be modified.
// \param ulDelta is the number of UINT to increment the index by.
// \param ulSize is the size of the buffer the index refers to.
//
// This function is used to increment a read or write buffer index that may be
// written in various different contexts. It ensures that the read/modify/write
// sequence is not interrupted and, hence, guards against corruption of the
// variable. The new value is adjusted for buffer wrap.
//
// \return None.
//
//*****************************************************************************
static void
UpdateIndexAtomic(volatile unsigned long *pulVal, unsigned long ulDelta,
                  unsigned long ulSize)
{
    bool bIntsOff;

    //
    // Turn interrupts off temporarily.
    //
    bIntsOff = IntMasterDisable();

    //
    // Update the variable value.
    //
    *pulVal += ulDelta;

    //
    // Correct for wrap. We use a loop here since we don't want to use a
    // modulus operation with interrupts off but we don't want to fail in
    // case ulDelta is greater than ulSize (which is extremely unlikely but...)
    //
    while(*pulVal >= ulSize)
    {
        *pulVal -= ulSize;
    }

    //
    // Restore the interrupt state
    //
    if(!bIntsOff)
    {
        IntMasterEnable();
    }
} // UpdateIndexAtomic()

//*****************************************************************************
//
//! Determines whether the ring buffer whose pointers and size are provided
//! is full or not.
//!
//! \param ptRingBuf is the ring buffer object to empty.
//!
//! This function is used to determine whether or not a given ring buffer is
//! full.  The structure is specifically to ensure that we do not see
//! warnings from the compiler related to the order of volatile accesses
//! being undefined.
//!
//! \return Returns \b true if the buffer is full or \b false otherwise.
//
//*****************************************************************************
bool RingBufFull(tRingBufObject *ptRingBuf)
{
    unsigned long ulWrite;
    unsigned long ulRead;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Copy the Read/Write indices for calculation.
    //
    ulWrite = ptRingBuf->ulWriteIndex;
    ulRead = ptRingBuf->ulReadIndex;

    //
    // Return the full status of the buffer.
    //
    return((((ulWrite + 1) % ptRingBuf->ulSize) == ulRead) ? true : false);
} // RingBufFull()

//*****************************************************************************
//
//! Determines whether the ring buffer whose pointers and size are provided
//! is empty or not.
//!
//! \param ptRingBuf is the ring buffer object to empty.
//!
//! This function is used to determine whether or not a given ring buffer is
//! empty.  The structure is specifically to ensure that we do not see
//! warnings from the compiler related to the order of volatile accesses
//! being undefined.
//!
//! \return Returns \b true if the buffer is empty or \b false otherwise.
//
//*****************************************************************************
bool RingBufEmpty(tRingBufObject *ptRingBuf)
{
    unsigned long ulWrite;
    unsigned long ulRead;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Copy the Read/Write indices for calculation.
    //
    ulWrite = ptRingBuf->ulWriteIndex;
    ulRead  = ptRingBuf->ulReadIndex;

    //
    // Return the empty status of the buffer.
    //
    return((ulWrite == ulRead) ? true : false);
} // RingBufEmpty()

//*****************************************************************************
//
//! Empties the ring buffer.
//!
//! \param ptRingBuf is the ring buffer object to empty.
//!
//! Discards all data from the ring buffer.
//!
//! \return None.
//
//*****************************************************************************
void RingBufFlush(tRingBufObject *ptRingBuf)
{
    bool bIntsOff;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Set the Read/Write pointers to be the same. Do this with interrupts
    // disabled to prevent the possibility of corruption of the read index.
    //
    bIntsOff = IntMasterDisable();
    ptRingBuf->ulReadIndex = ptRingBuf->ulWriteIndex;
    if(!bIntsOff)
    {
        IntMasterEnable();
    }
} // RingBufFlush()

//*****************************************************************************
//
//! Returns number of UINT stored in ring buffer.
//!
//! \param ptRingBuf is the ring buffer object to check.
//!
//! This function returns the number of UINT stored in the ring buffer.
//!
//! \return Returns the number of UINT stored in the ring buffer.
//
//*****************************************************************************
unsigned long RingBufUsed(tRingBufObject *ptRingBuf)
{
    unsigned long ulWrite;
    unsigned long ulRead;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Copy the Read/Write indices for calculation.
    //
    ulWrite = ptRingBuf->ulWriteIndex;
    ulRead  = ptRingBuf->ulReadIndex;

    //
    // Return the number of UINT contained in the ring buffer.
    //
    return((ulWrite >= ulRead) ? (ulWrite - ulRead) :
           (ptRingBuf->ulSize - (ulRead - ulWrite)));
} // RingBufUsed()

//*****************************************************************************
//
//! Returns number of UINT available in a ring buffer.
//!
//! \param ptRingBuf is the ring buffer object to check.
//!
//! This function returns the number of UINT available in the ring buffer.
//!
//! \return Returns the number of UINT available in the ring buffer.
//
//*****************************************************************************
unsigned long RingBufFree(tRingBufObject *ptRingBuf)
{
    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Return the number of UINT available in the ring buffer.
    //
    return((ptRingBuf->ulSize - 1) - RingBufUsed(ptRingBuf));
} // RingBufFree()

//*****************************************************************************
//
//! Returns number of contiguous UINT of data stored in ring buffer ahead of
//! the current read pointer.
//!
//! \param ptRingBuf is the ring buffer object to check.
//!
//! This function returns the number of contiguous UINT of data available in
//! the ring buffer ahead of the current read pointer. This represents the
//! largest block of data which does not straddle the buffer wrap.
//!
//! \return Returns the number of contiguous UINT available.
//
//*****************************************************************************
unsigned long RingBufContigUsed(tRingBufObject *ptRingBuf)
{
    unsigned long ulWrite;
    unsigned long ulRead;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Copy the Read/Write indices for calculation.
    //
    ulWrite = ptRingBuf->ulWriteIndex;
    ulRead  = ptRingBuf->ulReadIndex;

    //
    // Return the number of contiguous UINT available.
    //
    return((ulWrite >= ulRead) ? (ulWrite - ulRead) :
           (ptRingBuf->ulSize - ulRead));
} // RingBufContigUsed()

//*****************************************************************************
//
//! Returns number of contiguous free UINT available in a ring buffer.
//!
//! \param ptRingBuf is the ring buffer object to check.
//!
//! This function returns the number of contiguous free UINT ahead of the
//! current write pointer in the ring buffer.
//!
//! \return Returns the number of contiguous UINT available in the ring
//! buffer.
//
//*****************************************************************************
unsigned long RingBufContigFree(tRingBufObject *ptRingBuf)
{
    unsigned long ulWrite;
    unsigned long ulRead;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Copy the Read/Write indices for calculation.
    //
    ulWrite = ptRingBuf->ulWriteIndex;
    ulRead = ptRingBuf->ulReadIndex;

    //
    // Return the number of contiguous UINT available.
    //
    if(ulRead > ulWrite)
    {
        //
        // The read pointer is above the write pointer so the amount of free
        // space is the difference between the two indices minus 1 to account
        // for the buffer full condition (write index one behind read index).
        //
        return((ulRead - ulWrite) - 1);
    }
    else
    {
        //
        // If the write pointer is above the read pointer, the amount of free
        // space is the size of the buffer minus the write index. We need to
        // add a special-case adjustment if the read index is 0 since we need
        // to leave 1 UINT empty to ensure we can tell the difference between
        // the buffer being full and empty.
        //
        return(ptRingBuf->ulSize - ulWrite - ((ulRead == 0) ? 1 : 0));
    }
} // RingBufContigFree()

//*****************************************************************************
//
//! Return size in UINT of a ring buffer.
//!
//! \param ptRingBuf is the ring buffer object to check.
//!
//! This function returns the size of the ring buffer.
//!
//! \return Returns the size in UINT of the ring buffer.
//
//*****************************************************************************
unsigned long RingBufSize(tRingBufObject *ptRingBuf)
{
    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Return the number of UINT available in the ring buffer.
    //
    return(ptRingBuf->ulSize);
} // RingBufSize()

//*****************************************************************************
//
//! Reads a single UINT of data from a ring buffer.
//!
//! \param ptRingBuf points to the ring buffer to be written to.
//!
//! This function reads a single UINT of data from a ring buffer.
//!
//! \return The UINT read from the ring buffer.
//
//*****************************************************************************
UINT RingBufReadOne(tRingBufObject *ptRingBuf)
{
    UINT ucTemp;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Verify that space is available in the buffer.
    //
    ASSERT(RingBufUsed(ptRingBuf) != 0);

    //
    // Write the data UINT.
    //
    ucTemp = ptRingBuf->pucBuf[ptRingBuf->ulReadIndex];

    //
    // Increment the read index.
    //
    UpdateIndexAtomic(&ptRingBuf->ulReadIndex, 1, ptRingBuf->ulSize);

    //
    // Return the character read.
    //
    return(ucTemp);
} // RingBufReadOne()

//*****************************************************************************
//
//! Reads data from a ring buffer.
//!
//! \param ptRingBuf points to the ring buffer to be read from.
//! \param pucData points to where the data should be stored.
//! \param ulLength is the number of UINT to be read.
//!
//! This function reads a sequence of UINT from a ring buffer.
//!
//! \return None.
//
//*****************************************************************************
void RingBufRead(tRingBufObject *ptRingBuf, UINT *pucData, unsigned long ulLength)
{
    unsigned long ulTemp;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);
    ASSERT(pucData != NULL);
    ASSERT(ulLength != 0);

    //
    // Verify that data is available in the buffer.
    //
    ASSERT(ulLength <= RingBufUsed(ptRingBuf));

    //
    // Read the data from the ring buffer.
    //
    for(ulTemp = 0; ulTemp < ulLength; ulTemp++)
    {
        pucData[ulTemp] = RingBufReadOne(ptRingBuf);
    }
} // RingBufRead()

//*****************************************************************************
//
//! Remove UINT from the ring buffer by advancing the read index.
//!
//! \param ptRingBuf points to the ring buffer from which UINT are to be
//! removed.
//! \param ulNumUINT is the number of UINT to be removed from the buffer.
//!
//! This function advances the ring buffer read index by a given number of
//! UINT, removing that number of UINT of data from the buffer. If \e
//! ulNumUINT is larger than the number of UINT currently in the buffer, the
//! buffer is emptied.
//!
//! \return None.
//
//*****************************************************************************
void RingBufAdvanceRead(tRingBufObject *ptRingBuf, unsigned long ulNumUINT)
{
    unsigned long ulCount;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Make sure that we are not being asked to remove more data than is
    // there to be removed.
    //
    ulCount = RingBufUsed(ptRingBuf);
    ulCount =  (ulCount < ulNumUINT) ? ulCount : ulNumUINT;

    //
    // Advance the buffer read index by the required number of UINT.
    //
    UpdateIndexAtomic(&ptRingBuf->ulReadIndex, ulCount,ptRingBuf->ulSize);
} // RingBufAdvanceRead()

//*****************************************************************************
//
//! Add UINT to the ring buffer by advancing the write index.
//!
//! \param ptRingBuf points to the ring buffer to which UINT have been added.
//! \param ulNumUINT is the number of UINT added to the buffer.
//!
//! This function should be used by clients who wish to add data to the buffer
//! directly rather than via calls to RingBufWrite() or RingBufWriteOne(). It
//! advances the write index by a given number of UINT.  If the \e ulNumUINT
//! parameter is larger than the amount of free space in the buffer, the
//! read pointer will be advanced to cater for the addition.  Note that this
//! will result in some of the oldest data in the buffer being discarded.
//!
//! \return None.
//
//*****************************************************************************
void RingBufAdvanceWrite(tRingBufObject *ptRingBuf, unsigned long ulNumUINT)
{
    unsigned long ulCount;
    bool      bIntsOff;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Make sure we were not asked to add a silly number of UINT.
    //
    ASSERT(ulNumUINT <= ptRingBuf->ulSize);

    //
    // Determine how much free space we currently think the buffer has.
    //
    ulCount = RingBufFree(ptRingBuf);

    //
    // Advance the buffer write index by the required number of UINT and
    // check that we have not run past the read index. Note that we must do
    // this within a critical section (interrupts disabled) to prevent
    // race conditions that could corrupt one or other of the indices.
    //
    bIntsOff = IntMasterDisable();

    //
    // Update the write pointer.
    //
    ptRingBuf->ulWriteIndex += ulNumUINT;

    //
    // Check and correct for wrap.
    //
    if(ptRingBuf->ulWriteIndex >= ptRingBuf->ulSize)
    {
        ptRingBuf->ulWriteIndex -= ptRingBuf->ulSize;
    }

    //
    // Did the client add more UINT than the buffer had free space for?
    //
    if(ulCount < ulNumUINT)
    {
        //
        // Yes - we need to advance the read pointer to ahead of the write
        // pointer to discard some of the oldest data.
        //
        ptRingBuf->ulReadIndex = ptRingBuf->ulWriteIndex + 1;

        //
        // Correct for buffer wrap if necessary.
        //
        if(ptRingBuf->ulReadIndex >= ptRingBuf->ulSize)
        {
            ptRingBuf->ulReadIndex -= ptRingBuf->ulSize;
        }
    }

    //
    // Restore interrupts if we turned them off earlier.
    //
    if(!bIntsOff)
    {
        IntMasterEnable();
    }
} // RingBufAdvanceWrite()

//*****************************************************************************
//
//! Writes a single byte of data to a ring buffer.
//!
//! \param ptRingBuf points to the ring buffer to be written to.
//! \param ucData is the byte to be written.
//!
//! This function writes a single byte of data into a ring buffer.
//!
//! \return None.
//
//*****************************************************************************
void RingBufWriteOne(tRingBufObject *ptRingBuf, UINT ucData)
{
    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);

    //
    // Verify that space is available in the buffer.
    //
    ASSERT(RingBufFree(ptRingBuf) != 0);

    //
    // Write the data byte.
    //
    ptRingBuf->pucBuf[ptRingBuf->ulWriteIndex] = ucData;

    //
    // Increment the write index.
    //
    UpdateIndexAtomic(&ptRingBuf->ulWriteIndex, 1, ptRingBuf->ulSize);
} // RingBufWriteOne()

//*****************************************************************************
//
//! Writes data to a ring buffer.
//!
//! \param ptRingBuf points to the ring buffer to be written to.
//! \param pucData points to the data to be written.
//! \param ulLength is the number of UINT to be written.
//!
//! This function write a sequence of UINT into a ring buffer.
//!
//! \return None.
//
//*****************************************************************************
void RingBufWrite(tRingBufObject *ptRingBuf, UINT *pucData, unsigned long ulLength)
{
    unsigned long ulTemp;

    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);
    ASSERT(pucData != NULL);
    ASSERT(ulLength != 0);

    //
    // Verify that space is available in the buffer.
    //
    ASSERT(ulLength <= RingBufFree(ptRingBuf));

    //
    // Write the data into the ring buffer.
    //
    for(ulTemp = 0; ulTemp < ulLength; ulTemp++)
    {
        RingBufWriteOne(ptRingBuf, pucData[ulTemp]);
    }
} // RingBufWrite()

//*****************************************************************************
//
//! Initialize a ring buffer object.
//!
//! \param ptRingBuf points to the ring buffer to be initialized.
//! \param pucBuf points to the data buffer to be used for the ring buffer.
//! \param ulSize is the size of the buffer in UINT.
//!
//! This function initializes a ring buffer object, preparing it to store data.
//!
//! \return None.
//
//*****************************************************************************
void RingBufInit(tRingBufObject *ptRingBuf, UINT *pucBuf, unsigned long ulSize)
{
    //
    // Check the arguments.
    //
    ASSERT(ptRingBuf != NULL);
    ASSERT(pucBuf != NULL);
    ASSERT(ulSize != 0);

    //
    // Initialize the ring buffer object.
    //
    ptRingBuf->ulSize = ulSize;
    ptRingBuf->pucBuf = pucBuf;
    ptRingBuf->ulWriteIndex = ptRingBuf->ulReadIndex = 0;
} // RingBufInit()

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
