//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

/*++

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    I2C.C   I2C H/W Access routines (logical PDD)

Abstract:

    Supports:
    ---------
    o) Master Tx/Rx Modes only.
    o) Multi-byte Tx/Rx (slave must support auto increment for multi-byte)

Functions:

Notes:

--*/

#include <windows.h>
#include <nkintr.h>
#include <bsp.h>

#include "drv.h"


//
// I2C Registers
//
#define rIICCON     (pI2C->pI2CReg->IICCON)     // Control Register
#define rIICSTAT    (pI2C->pI2CReg->IICSTAT)    // Status Register
#define rIICADD     (pI2C->pI2CReg->IICADD)     // Address Register
#define rIICDS      (pI2C->pI2CReg->IICDS)      // Data Shift Register


// BUGBUG: need ioctl to set timeout parms based on num chars client is expecting
DWORD gIntrIIC 		= SYSINTR_NOP;
#define RX_TIMEOUT  3000
#define TX_TIMEOUT  3000

#define IIC_POWER_ON   (1<<17)    // PCLK_GATE bit 17

static DWORD I2C_IST(LPVOID Context);


VOID
InitRegs(
    PI2C_CONTEXT pI2C
    )
{
    EnterCriticalSection(&pI2C->RegCS);

    //
    // enable the I2C Clock (PCLK)
    //
    pI2C->g_pSYSCONReg->PCLK_GATE |= IIC_POWER_ON;

     // set SCL
    pI2C->g_pGPIOReg->GPBCON = (pI2C->g_pGPIOReg->GPBCON & ~(0xf<<20)) | (0x2<<20);

    // set SDA
    pI2C->g_pGPIOReg->GPBCON = (pI2C->g_pGPIOReg->GPBCON & ~(0xf<<24)) | (0x2<<24);

    // set SCL pull-up
    pI2C->g_pGPIOReg->GPBPUD = (pI2C->g_pGPIOReg->GPBPUD & ~(0x3<<10)) | (0x0<<10);

    // set SDA pull-up
    pI2C->g_pGPIOReg->GPBPUD = (pI2C->g_pGPIOReg->GPBPUD & ~(0x3<<12)) | (0x0<<12);

    // config controller
    rIICCON  = RESUME_ACK;

    rIICSTAT = M_IDLE;
    
    pI2C->State = IDLE;

    pI2C->DataCount = INVALID_DATA_COUNT;

    LeaveCriticalSection(&pI2C->RegCS);
}


DWORD
HW_Init(
    PI2C_CONTEXT pI2C
    )
{
    DWORD dwErr = ERROR_SUCCESS;
	UINT32 Irq;
    
//    RETAILMSG(1,(TEXT("I2C Init\r\n")));
    if ( !pI2C ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    DEBUGMSG(ZONE_TRACE,(TEXT("+I2C_Init: %u, 0x%x, 0x%x \r\n"), 
        pI2C->Mode, pI2C->SlaveAddress));

    InitializeCriticalSection(&pI2C->RegCS);
    
    pI2C->Status    = 0;
    pI2C->Data      = NULL;
    pI2C->DataCount = INVALID_DATA_COUNT;
    pI2C->Flags.DropRxAddr = FALSE;

    pI2C->hProc = (HANDLE)GetCurrentProcessId();

    InitRegs(pI2C);

    // create I/O Done Event
    if ( (pI2C->DoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
    {
        dwErr = GetLastError();
        DEBUGMSG(ZONE_ERR,(TEXT("I2C_Init ERROR: Unable to create Done event: %u \r\n"), dwErr));
        goto _init_error;
    }

    // setup Operating Mode
    if ( pI2C->Mode == INTERRUPT ) {
        // create IST event
        if ( (pI2C->ISTEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERR,(TEXT("I2C_Init ERROR: Unable to create IST event: %u \r\n"), dwErr));
            goto _init_error;
        }

	// Obtain sysintr values from the OAL for the camera interrupt.
	//
	Irq = IRQ_I2C;
	if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &Irq, sizeof(UINT32), &gIntrIIC, sizeof(UINT32), NULL))
	{
		DEBUGMSG(ZONE_ERR, (TEXT("ERROR: Failed to request the IIC sysintr.\r\n")));
		gIntrIIC = SYSINTR_UNDEFINED;
		return(FALSE);
	}
//	RETAILMSG(1, (TEXT("IIC IRQ mapping: [IRQ:%d->sysIRQ:%d].\r\n"), Irq, gIntrIIC));

        // initialize the interrupt
        if( !InterruptInitialize(gIntrIIC, pI2C->ISTEvent, NULL, 0) ) 
        {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERR,(TEXT("I2C_Init ERROR: Unable to initialize interrupt: %u\r\n"), dwErr));
            goto _init_error;
        }

        InterruptDone(gIntrIIC);

        // create the IST
        if ( (pI2C->IST = CreateThread(NULL, 0, I2C_IST, (LPVOID)pI2C, 0, NULL)) == NULL)
        {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERR,(TEXT("I2C_Init ERROR: Unable to create IST: %u\r\n"), dwErr));
            goto _init_error;
        }

        // TODO: registry override
        if ( !CeSetThreadPriority(pI2C->IST, I2C_THREAD_PRIORITY)) {
            dwErr = GetLastError();
            DEBUGMSG(ZONE_ERR, (TEXT("I2C_Init ERROR: CeSetThreadPriority ERROR:%d\n"), dwErr));
            goto _init_error;
        }
    }

    DEBUGMSG(ZONE_TRACE,(TEXT("-I2C_Init \r\n")));

    return dwErr;

_init_error:
    HW_Deinit(pI2C);

    return dwErr;
}


DWORD
HW_Deinit(
    PI2C_CONTEXT pI2C
    )
{
    if ( !pI2C )
        return ERROR_INVALID_PARAMETER;
        
    DEBUGMSG(ZONE_TRACE,(TEXT("I2C_Deinit: %u \r\n"), pI2C->State));

    if ( pI2C->State == IDLE ) {

        pI2C->State = OFF; // also signals the IST to terminate
        
        rIICSTAT = M_IDLE;

        if (pI2C->DoneEvent && CloseHandle(pI2C->DoneEvent))
            pI2C->DoneEvent = NULL;

        if (pI2C->Mode == INTERRUPT) {
            
            InterruptDisable(gIntrIIC);
        
            if (pI2C->ISTEvent && CloseHandle(pI2C->ISTEvent))
                pI2C->ISTEvent = NULL;
            
            if (pI2C->IST && CloseHandle(pI2C->IST))
                pI2C->IST = NULL;
        }

        pI2C->Mode = POLLING;
        pI2C->Data  = NULL;
        pI2C->DataCount = INVALID_DATA_COUNT;
        pI2C->Flags.DropRxAddr = FALSE;

        DeleteCriticalSection(&pI2C->RegCS);

        return ERROR_SUCCESS;
        
    } else {
        DEBUGMSG(ZONE_ERR,(TEXT("I2C_Deinit ERROR: %u \r\n"), pI2C->State));
        
        return ERROR_BUSY;
    }
}


DWORD
HW_Open(
    PI2C_CONTEXT pI2C
    )
{
    // BUGBUG: power on device
    return ERROR_SUCCESS;
}


DWORD
HW_Close(
    PI2C_CONTEXT pI2C
    )
{
    // BUGBUG: power off device
    return ERROR_SUCCESS;
}


__inline
DWORD
SyncIst(    
    PI2C_CONTEXT pI2C,
    DWORD dwTimeout
    )
{
    DWORD w;
    if (pI2C->Mode == INTERRUPT) {

        DEBUGMSG(ZONE_READ|ZONE_WRITE,(TEXT("SyncIst...\r\n")));
//        RETAILMSG(1,(TEXT("SyncIst...\r\n")));

        w = WaitForSingleObject(pI2C->DoneEvent, dwTimeout);

        if (WAIT_OBJECT_0 != w) {
            pI2C->State = IO_ABANDONED;
            if (pI2C->LastError == ERROR_SUCCESS)
                pI2C->LastError = ERROR_TIMEOUT;
            DEBUGMSG(ZONE_WRN|ZONE_READ|ZONE_WRITE,(TEXT("SyncIst: IO_ABANDONED\r\n")));
//            RETAILMSG(1, (TEXT("SyncIst: IO_ABANDONED\r\n")));
        }
        
        DEBUGMSG(ZONE_READ|ZONE_WRITE,(TEXT("...SyncIst: 0x%X\r\n"), pI2C->LastError));
        
        return pI2C->LastError;
        
    } else {
        while(pI2C->DataCount != INVALID_DATA_COUNT) {
            // poll INT pending bit
            if (pI2C->Mode == POLLING && (rIICCON & 0x10))
                I2C_IST(pI2C);
//		     RETAILMSG(1, (TEXT("SynIst] I2C_IST : Mode = %d"), pI2C->Mode));
        }
    }
    
    return ERROR_SUCCESS;
}


DWORD 
HW_Read(
    PI2C_CONTEXT pI2C,
    DWORD  SlaveAddr,   // slave address
    UCHAR  WordAddr,    // starting word address
    PUCHAR pData,       // pdata
    DWORD  Count        // bytes to read
    )
{
    DWORD  dwErr;

    if ( !VALID_CONTEXT(pI2C) )
        return ERROR_INVALID_PARAMETER;
        
    DEBUGMSG(ZONE_READ|ZONE_TRACE,(TEXT("+I2C_Read[%u]: 0x%X, 0x%X, 0x%X, %u\r\n"), 
        pI2C->State, SlaveAddr, WordAddr, pData, Count));
    
    if ( !pData || !Count || IsBadWritePtr(pData, Count) ) {
        DEBUGMSG(ZONE_ERR,(TEXT("I2C_Read ERROR: invalid parameter \r\n")));
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection(&pI2C->RegCS);
    
    if ( pI2C->State != IDLE) {
        DEBUGMSG(ZONE_ERR,(TEXT("I2C_Read ERROR: i2cState: %u \r\n"), pI2C->State));
        LeaveCriticalSection(&pI2C->RegCS);
        return ERROR_BUSY;
    }

    pI2C->LastError = ERROR_SUCCESS;
    
    ResetEvent(pI2C->DoneEvent);

    rIICSTAT = M_ACTIVE;
    
    // pre-setup word address
    pI2C->Data = NULL;
    pI2C->State = SET_READ_ADDR;
    pI2C->WordAddr = WordAddr;
    pI2C->Flags.WordAddr = TRUE;
    pI2C->DataCount = 1;
    
    // enable the slave address drop
    pI2C->Flags.DropRxAddr = TRUE;

    // write slave address
    rIICDS = (UCHAR)SlaveAddr;
    rIICSTAT = MTX_START;

    // Wait for IST to write the word address
    if (WAIT_OBJECT_0 != SyncIst(pI2C, RX_TIMEOUT)) {
        
        DEBUGMSG(ZONE_READ|ZONE_ERR,(TEXT("RX_TIMEOUT.1\r\n")));

        goto _done;
    }

    ResetEvent(pI2C->DoneEvent);

    // get read data
    pI2C->State = READ_DATA;
    pI2C->Data = pData;
    pI2C->DataCount = Count;
    rIICDS = (UCHAR)SlaveAddr;
    rIICSTAT = MRX_START;
    rIICCON  = RESUME_ACK;  // Resume IIC operation (clear bit 4)

    // Wait for IST to get data
    if (WAIT_OBJECT_0 != SyncIst(pI2C, RX_TIMEOUT)) {
        
        DEBUGMSG(ZONE_READ|ZONE_ERR,(TEXT("RX_TIMEOUT.2\r\n")));

        goto _done;
    }



_done:    
    rIICSTAT  = M_IDLE;    // disable Rx/Tx
    pI2C->State = IDLE;
    pI2C->Data  = NULL;
    pI2C->DataCount = INVALID_DATA_COUNT;

    if ( !pI2C->LastError && (SlaveAddr != pI2C->RxRetAddr) ) {

        DEBUGMSG(ZONE_READ|ZONE_ERR,(TEXT("I2C_Read Invalid Return Address: 0x%X != 0x%X \r\n"), 
            pI2C->RxRetAddr, SlaveAddr ));

        pI2C->LastError = ERROR_INCORRECT_ADDRESS;

        TEST_TRAP;
    }
    
    pI2C->RxRetAddr = 0;

    dwErr = pI2C->LastError;
    
    LeaveCriticalSection(&pI2C->RegCS);
    
    DEBUGMSG(ZONE_READ|ZONE_TRACE,(TEXT("-I2C_Read:%u \r\n"), dwErr ));
	//RETAILMSG(1,(_T("data = 0x%X\r\n", *pData)));

    return dwErr;
}


DWORD
HW_Write(
    PI2C_CONTEXT pI2C,
    DWORD  SlaveAddr,   // slave address
    UCHAR  WordAddr,    // starting slave word address
    PUCHAR pData,       // pdata
    DWORD  Count        // bytes to write
    )    
{
    DWORD dwErr;

    if ( !VALID_CONTEXT(pI2C) )
        return ERROR_INVALID_PARAMETER;
        
//    DEBUGMSG(ZONE_WRITE|ZONE_TRACE,(TEXT("+I2C_Write[%u]: 0x%X, 0x%X, 0x%X, %u \r\n"), 
//   RETAILMSG(1,(TEXT("+I2C_Write[%u]: 0x%X, 0x%X, 0x%X, %u \r\n"), 
//        pI2C->State, SlaveAddr, WordAddr, *pData, Count));
    
    if ( !pData || !Count || IsBadReadPtr(pData, Count) ) {
        DEBUGMSG(ZONE_ERR,(TEXT("I2C_Write ERROR: invalid parameter \r\n")));
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection(&pI2C->RegCS);
    
    if ( pI2C->State != IDLE) {
        DEBUGMSG(ZONE_ERR,(TEXT("I2C_Write ERROR: i2cState: %u \r\n"), pI2C->State));
        LeaveCriticalSection(&pI2C->RegCS);
        return ERROR_BUSY;
    }

    pI2C->LastError = ERROR_SUCCESS;
        
    ResetEvent(pI2C->DoneEvent);

    rIICSTAT = M_ACTIVE;

    // pre-setup write data
    pI2C->State     = WRITE_DATA;
    pI2C->DataCount = 1 + Count; // slave word address + data 
    pI2C->WordAddr  = WordAddr;
    pI2C->Flags.WordAddr = TRUE;    
    pI2C->Data      = pData;

    // write slave address
    rIICDS   = (UCHAR)SlaveAddr;
    rIICSTAT = MTX_START;

    // IST writes the slave word address & data
    if (WAIT_OBJECT_0 != SyncIst(pI2C, TX_TIMEOUT)) {
        
        DEBUGMSG(ZONE_WRITE|ZONE_ERR,(TEXT("TX_TIMEOUT.1\r\n")));

        goto _done;
    }

// LJY, 20040724, commented out
#if 0
    // get ACK
    while(1)
    {
        pI2C->State  = WRITE_ACK;
        pI2C->Status = INVALID_IICSTAT;
        
        rIICDS   = (UCHAR)SlaveAddr;
        rIICSTAT = MTX_START;
        rIICCON  = RESUME_ACK;

        while ( pI2C->Status == INVALID_IICSTAT ) {
            if (WAIT_OBJECT_0 != SyncIst(pI2C, TX_TIMEOUT)) {
                DEBUGMSG(ZONE_WRITE|ZONE_ERR,(TEXT("TX_TIMEOUT.2\r\n")));
                break;
            }
        }

        // done when ACK received
        if ( !(pI2C->Status & 0x1))
            break; 
    }
#endif

_done:
    rIICSTAT = MTX_STOP;     
    rIICCON  = RESUME_ACK;  

    rIICSTAT  = M_IDLE;    // disable Rx/Tx    
    pI2C->State = IDLE;
    pI2C->Data  = NULL;
    pI2C->DataCount = INVALID_DATA_COUNT;

    dwErr = pI2C->LastError;

    LeaveCriticalSection(&pI2C->RegCS);
    
    DEBUGMSG(ZONE_WRITE|ZONE_TRACE,(TEXT("-I2C_Write: %u \r\n"), dwErr));
    
    return dwErr;
}


static DWORD
I2C_IST(
    LPVOID Context
    )
{
    PI2C_CONTEXT pI2C = (PI2C_CONTEXT)Context;
    DWORD i2cSt;
    BOOL bDone = FALSE;
#ifdef DEBUG
    DWORD r = 0;
#endif

    if ( !pI2C ) {
        TEST_TRAP;
        return ERROR_INVALID_PARAMETER;
    }

    //DEBUGMSG(ZONE_IST|ZONE_TRACE,(TEXT("+I2C_IST[%u, %u, %d] \r\n"), pI2C->Mode, pI2C->State, pI2C->DataCount));

    do  {
        
        if (pI2C->Mode == INTERRUPT) {
            
            DWORD we;

            bDone = FALSE;
            
            we = WaitForSingleObject(pI2C->ISTEvent, INFINITE);
            
            //DEBUGMSG(ZONE_IST|ZONE_TRACE,(TEXT("I2C_IST[%u, %d] \r\n"), pI2C->State, pI2C->DataCount));

            // clear the interrupt here because we re-arm another below
            InterruptDone(gIntrIIC);

            //
            // Ensure correct state initiated by Read/Write
            //
            DEBUGMSG(ZONE_IST|ZONE_TRACE,(TEXT("I2C_IST[%u, %d] \r\n"), 
                pI2C->State, pI2C->DataCount));

            switch(pI2C->State)
            {
                case OFF:
                    DEBUGMSG(ZONE_IST|ZONE_TRACE,(TEXT("I2C_IST: ExitThread \r\n")));
                    ExitThread(ERROR_SUCCESS);
                    break;

                case IDLE:
                    DEBUGMSG(ZONE_IST|ZONE_TRACE,(TEXT("I2C_IST: IDLE \r\n")));
                    continue;
                    break;

                default:
                    if (pI2C->State != WRITE_ACK && 
                        pI2C->State != RESUME &&
                        pI2C->DataCount == INVALID_DATA_COUNT) {
                        DEBUGMSG(ZONE_IST|ZONE_TRACE,(TEXT("I2C_IST: INVALID_DATA_COUNT\r\n")));
                        continue;
                    }
                    break;
            }
        }

//        EnterCriticalSection(&pI2C->RegCS);
        
        i2cSt = rIICSTAT; 

        if (i2cSt & 0x8) {
            DEBUGMSG(ZONE_ERR,(TEXT("I2C_IST[%u, %d]: bus arbitration failed \r\n"), 
                pI2C->State, pI2C->DataCount));
//           RETAILMSG(1, (TEXT("I2C_IST[%u, %d]: bus arbitration failed \r\n"), 
//                pI2C->State, pI2C->DataCount));
        } 

        if (i2cSt & 0x4) {
            DEBUGMSG(ZONE_ERR,(TEXT("I2C_IST[%u, %d]: slave address matches IICADD \r\n"), 
                pI2C->State, pI2C->DataCount));
//            RETAILMSG(1, (TEXT("I2C_IST[%u, %d]: slave address matches IICADD \r\n"), 
//                pI2C->State, pI2C->DataCount));
        }
        
        if (i2cSt & 0x2) {
            DEBUGMSG(ZONE_ERR,(TEXT("I2C_IST[%u, %d]: received slave address 0x0 \r\n"), 
                pI2C->State, pI2C->DataCount));
//            RETAILMSG(1, (TEXT("I2C_IST[%u, %d]: received slave address 0x0 \r\n"), 
//                pI2C->State, pI2C->DataCount));
        }

        if (i2cSt & 0x1) {
            DEBUGMSG(ZONE_READ|ZONE_WRITE,(TEXT("I2C_IST[%u, %d]: ACK NOT received \r\n"), 
                pI2C->State, pI2C->DataCount));
	     RETAILMSG(1, (TEXT("I2C_IST[%u, %d]: ACK NOT received \r\n"), 
               pI2C->State, pI2C->DataCount));
        }

        __try {
                
            switch(pI2C->State)
            {
                case IDLE:
                case SUSPEND:
                    continue;
                    break;
                    
                case RESUME:
                    DEBUGMSG(ZONE_WRN, (TEXT("I2C_IST: RESUME \r\n")));
//                  RETAILMSG(1,  (TEXT("I2C_IST: RESUME \r\n")));
                    InitRegs(pI2C);
                    pI2C->LastError = ERROR_OPERATION_ABORTED;
                    SetEvent(pI2C->DoneEvent);
                    break;

                case SET_READ_ADDR:
                    if ( (pI2C->DataCount--) == 0 )
                    {
                        DEBUGMSG(ZONE_READ|ZONE_TRACE, (TEXT("A2[%d] \r\n"), 
                            pI2C->DataCount ));
//                        RETAILMSG(1, (TEXT("A2[%d] \r\n"), 
//           		 pI2C->DataCount ));
                        bDone = TRUE;
                        break;
                    }

                    DEBUGMSG(ZONE_READ|ZONE_TRACE, (TEXT("A1[%d]: 0x%X \r\n"), 
                        pI2C->DataCount, pI2C->WordAddr));
//                    RETAILMSG(1, (TEXT("A1[%d]: 0x%X \r\n"), 
//                        pI2C->DataCount, pI2C->WordAddr));                    
                    // write word address
                    // For setup time of SDA before SCL rising edge, rIICDS must be written 
                    // before clearing the interrupt pending bit.
                    if (pI2C->Flags.WordAddr) {
                        rIICDS = pI2C->WordAddr;
                        // clear interrupt pending bit (resume)
                        rIICCON = RESUME_ACK;
                        pI2C->Flags.WordAddr = FALSE;
                    }
                    break;


                case READ_DATA:                    
                    ASSERT(pI2C->Data);
                    if ( (pI2C->DataCount--) == 0 )
                    {
                        bDone = TRUE;
                    
                        *pI2C->Data = (UCHAR)rIICDS;
#ifdef DEBUG                        
                        r = *pI2C->Data;
                        
                        //RETAILMSG(1,(TEXT("R3_1:0x%X, pI2C->Data(0x%X) \r\n"), r, pI2C->Data));
                        //RETAILMSG(1,(_T("pI2C->Data(0x%X)\r\n"), pI2C->Data));
#endif
                        pI2C->Data++;
                        
                        rIICSTAT = MRX_STOP;    
                        rIICCON  = RESUME_ACK;  // resume operation.
                        
                        //RETAILMSG(1,(_T("pI2C->Data(0x%X)\r\n"), pI2C->Data));
                        //RETAILMSG(1,(TEXT("R3_2:0x%X, pI2C->Data(0x%X) \r\n"), r, pI2C->Data));
                        //DEBUGMSG(ZONE_READ|ZONE_TRACE,(TEXT("R3:0x%X \r\n"), r));
                                    
                        //The pending bit will not be set after issuing stop condition.
                        break;    
                    }

                    // Drop the returned Slave WordAddr?
                    if ( pI2C->Flags.DropRxAddr )
                    {
                        pI2C->RxRetAddr = (UCHAR)rIICDS;
                        pI2C->Flags.DropRxAddr = FALSE;
                        DEBUGMSG(ZONE_READ|ZONE_TRACE,(TEXT("Drop: 0x%X \r\n"), 
                            pI2C->RxRetAddr));
                        
                    } else {
                        *pI2C->Data = (UCHAR)rIICDS;
#ifdef DEBUG
                        r = *pI2C->Data;
#endif
                        pI2C->Data++;
                    }

					//RETAILMSG(1,(TEXT("R3_3:0x%X, pI2C->Data(0x%X) \r\n"), r, pI2C->Data));
					
                    // The last data is read with no ack.
                    if ( pI2C->DataCount == 0 ) {
                        rIICCON = RESUME_NO_ACK;    // resume operation with NOACK.  
                        DEBUGMSG(ZONE_READ|ZONE_TRACE,(TEXT("R1:0x%X \r\n"), r));
                    } else {
                        rIICCON = RESUME_ACK;       // resume operation with ACK
                        DEBUGMSG(ZONE_READ|ZONE_TRACE,(TEXT("R2:0x%X \r\n"), r));
                    }
                    break;


                case WRITE_DATA:                        
                    ASSERT(pI2C->Data);
                    if ( (pI2C->DataCount--) == 0 )
                    {
                        DEBUGMSG(ZONE_WRITE|ZONE_TRACE,(TEXT("W3[%d] \r\n"), pI2C->DataCount));
                        //RETAILMSG(1,(TEXT("W3[%d] \r\n"), pI2C->DataCount));
                        
                        bDone = TRUE;

                        rIICSTAT = MTX_STOP;    
                        rIICCON  = RESUME_ACK;  // resume operation.

                        //The pending bit will not be set after issuing stop condition.
                        break;    
                    }
                    
                    if (pI2C->Flags.WordAddr) {
                        rIICDS = pI2C->WordAddr;
                        DEBUGMSG(ZONE_WRITE|ZONE_TRACE,(TEXT("W1[%d]: 0x%X \r\n"), 
                            pI2C->DataCount, pI2C->WordAddr));
                        pI2C->Flags.WordAddr = FALSE;
                    } else {
                        DEBUGMSG(ZONE_WRITE|ZONE_TRACE,(TEXT("W2[%d]: 0x%X \r\n"), 
                            pI2C->DataCount, *pI2C->Data));
                        
                        rIICDS = (UCHAR)*pI2C->Data;
                        pI2C->Data++;
                    }

                    rIICCON = RESUME_ACK;   // resume operation.
                    break;


                case WRITE_ACK:
                    DEBUGMSG(ZONE_WRITE|ZONE_TRACE,(TEXT("WRITE_ACK \r\n") ));
                    //RETAILMSG(1,(TEXT("WRITE_ACK \r\n") ));
                    pI2C->Status = i2cSt;
                    bDone = TRUE;
                    break;


                case IO_COMPLETE:
                    DEBUGMSG(ZONE_IST, (TEXT("IO_COMPLETE \r\n")));
                    bDone = TRUE;
                    break;


                case IO_ABANDONED:
                    DEBUGMSG(ZONE_ERR|ZONE_WRN,(TEXT("IO_ABANDONED \r\n") ));
                    bDone = TRUE;
                    break;


                default:
                    DEBUGMSG(ZONE_ERR,(TEXT("!!! I2C_IST ERROR: Invalid State: %u !!!\r\n"), 
                        pI2C->State));
                    bDone = TRUE;
                    break;    
            }
        
        } _except(EXCEPTION_EXECUTE_HANDLER) {
            
            rIICSTAT = (pI2C->State == READ_DATA) ? MRX_STOP : MTX_STOP;
            rIICCON  = RESUME_ACK;

            pI2C->DataCount = INVALID_DATA_COUNT;
        
            pI2C->LastError = GetExceptionCode();
//            RETAILMSG(1,(TEXT("!!! I2C_IST EXCEPTION: 0x%X !!!\r\n"), pI2C->LastError ));
        }

            
        if (bDone) {
            DEBUGMSG(ZONE_IST, (TEXT("SetEvent DONE\r\n")));
            SetEvent(pI2C->DoneEvent);
        }            

//        LeaveCriticalSection(&pI2C->RegCS);
                    
    } while (pI2C->Mode == INTERRUPT);


    //DEBUGMSG(ZONE_IST|ZONE_TRACE,(TEXT("-I2C_IST[%u] \r\n"), pI2C->Mode));

    return ERROR_SUCCESS;
}


BOOL
HW_PowerUp(
    PI2C_CONTEXT pI2C
   )
{
    pI2C->State = RESUME;
    SetInterruptEvent(gIntrIIC);
    return TRUE;
}


BOOL
HW_PowerDown(
    PI2C_CONTEXT pI2C
   )
{
    // set idle
    rIICSTAT = M_IDLE;
    
    // turn off our clock
    pI2C->g_pSYSCONReg->PCLK_GATE &= ~IIC_POWER_ON;
    
    pI2C->State = SUSPEND;
    return TRUE;
}


BOOL
HW_PowerCapabilities(
    PI2C_CONTEXT pI2C,
    PPOWER_CAPABILITIES ppc
    )
{
    memset(ppc, 0, sizeof(*ppc));

    //
    // SPECIAL CASE: support D0 only. 
    // If we power off outside of system state Suspend then the PCF50606 
    // will no longer work, which means the whole system could be forced off 
    // by an internal PCF transition. The Battery driver would not work either.
    //
    // If we knew we did not have to support the PCF50606,
    // or didn't know what I2C Client drivers we could have,
    // then we should support D0 and D4. In this case you would use
    // RegisterPowerRelationship and serve as the Bus driver (parent/proxy)
    // to forward power transitions to all the I2C Client drivers.
    //
    ppc->DeviceDx = 0x1;    // D0

    // no wake
    // no inrush

    // Report our power consumption in uAmps rather than mWatts. 
    ppc->Flags = POWER_CAP_PREFIX_MICRO | POWER_CAP_UNIT_AMPS;
    ppc->Power[D0] = 600;   // 0.6 mA or 600 uA @ 200 MHz

    // no latency issues

    return TRUE;
}


BOOL
HW_PowerSet(
    PI2C_CONTEXT pI2C,
    PCEDEVICE_POWER_STATE pDx   // IN, OUT
   )
{   
    CEDEVICE_POWER_STATE NewDx = *pDx;

    if ( VALID_DX(NewDx) ) 
    {
        // We only support D0, so do nothing.
        // Just return current state.
        pI2C->Dx = *pDx = D0;

        DEBUGMSG(ZONE_POWER, (TEXT("I2C: IOCTL_POWER_SET: D%u => D%u \r\n"),
            NewDx, pI2C->Dx));

        return TRUE;
    }

    return FALSE;
}


BOOL
HW_PowerGet(
    PI2C_CONTEXT pI2C,
    PCEDEVICE_POWER_STATE pDx
   )
{   
    // return our Current Dx value
    *pDx = pI2C->Dx;

    DEBUGMSG(ZONE_POWER, (TEXT("I2C: IOCTL_POWER_GET: D%u \r\n"), pI2C->Dx));

    return TRUE;
}

