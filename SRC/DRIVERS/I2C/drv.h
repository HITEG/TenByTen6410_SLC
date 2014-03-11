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

    drv.h   I2C (logical) MDD

Abstract:

   Streams interface driver

Functions:

Notes:

--*/

#ifndef _I2C_DRV_H_
#define _I2C_DRV_H_

#include <pm.h>
#include "pmplatform.h"

#include "s3c6410.h"
#include <i2c.h>


//
// I2C_STATE
//
typedef enum I2C_STATE {
    OFF,
    IDLE,
    WRITE_DATA,
    WRITE_ACK,
    READ_DATA,
    SET_READ_ADDR,
    IO_COMPLETE,
    IO_ABANDONED,
    SUSPEND,
    RESUME,
} I2C_STATE, *PI2C_STATE;


typedef struct _FLAGS {

    UCHAR   DropRxAddr : 1;
    UCHAR   WordAddr   : 1;
    
} FLAGS, *PFLAGS;


#define I2C_SIG ' C2I'

#define VALID_CONTEXT( p ) \
   ( p && p->Sig && I2C_SIG == p->Sig )

#define INVALID_DATA_COUNT  (-1)

//
// I2C_CONTEXT
//
typedef struct _I2C_CONTEXT {

    DWORD   Sig;    // Signature

    // I2C Registers
    volatile S3C6410_IIC_REG *pI2CReg;

    // GPIO Ports
    volatile S3C6410_GPIO_REG *g_pGPIOReg;

    // Clock / Power
    volatile S3C6410_SYSCON_REG *g_pSYSCONReg;

    CRITICAL_SECTION RegCS; // Register CS
    
    // State
    I2C_MODE    Mode;
    I2C_STATE   State;
    int         Status;

    FLAGS       Flags;

    // Data
    PUCHAR      Data;           // pointer to R/W data buffer
    int         DataCount;      // nBytes to R/W to/from data buffer
    UCHAR       WordAddr;       // slave word address
    UCHAR       RxRetAddr;      // returned slave address on Rx

    DWORD       SlaveAddress;   // Our I2C Slave Address

    HANDLE      DoneEvent;      // I/O Done Event
    HANDLE      ISTEvent;       // IST Event
    HANDLE      IST;            // IST Thread

    DWORD       OpenCount;

    DWORD       LastError;

    HANDLE      hProc;

    CEDEVICE_POWER_STATE    Dx;

} I2C_CONTEXT, *PI2C_CONTEXT;


//
// I2C Master Commands (rIICSTAT)
//
#define M_IDLE          0x00    // Disable Rx/Tx
#define M_ACTIVE        0x10    // Enable  Rx/Tx
#define MTX_START       0xF0    // Master Tx Start
#define MTX_STOP        0xD0    // Master Tx Stop
#define MRX_START       0xB0    // Master Rx Start
#define MRX_STOP        0x90    // Master Rx Stop

#define INVALID_IICSTAT 0x100


//
// I2C Control commands (rIICCON)
//
//#define RESUME_ACK      0xAF    // clear interrupt pending bit, ACK enabled, div=16, IICCLK=PCLK/16
//#define RESUME_NO_ACK   0x2F    // clear interrupt pending bit, ACK disabled, div=16, IICCLK=PCLK/16

#define RESUME_ACK      0xEF    // clear interrupt pending bit, ACK enabled, div=512, IICCLK=PCLK/512
#define RESUME_NO_ACK   0x6F    // clear interrupt pending bit, ACK disabled, div=512, IICCLK=PCLK/512

//
// Registry names 
//
#define REG_MODE_VAL_NAME TEXT("Mode") 
#define REG_MODE_VAL_LEN  sizeof( DWORD )

#define REG_SLAVEADDR_VAL_NAME TEXT("SlaveAddress") 
#define REG_SLAVEADDR_VAL_LEN  sizeof( DWORD )

#define REG_IRQ_VAL_NAME TEXT("IRQ") 
#define REG_IRQ_VAL_LEN  sizeof( DWORD )

#define REG_IOBASE_VAL_NAME TEXT("IoBase") 
#define REG_IOBASE_VAL_LEN  sizeof( DWORD )
#define REG_IOLEN_VAL_NAME TEXT("IoLen") 
#define REG_IOLEN_VAL_LEN  sizeof( DWORD )

#define REG_CONFIGBASE_VAL_NAME TEXT("ConfigBase") 
#define REG_CONFIGBASE_VAL_LEN  sizeof( DWORD )
#define REG_CONFIGLEN_VAL_NAME TEXT("ConfigLen") 
#define REG_CONFIGLEN_VAL_LEN  sizeof( DWORD )


//
// PDD functions
//
DWORD 
HW_Init(
    PI2C_CONTEXT pI2C
    );

DWORD 
HW_Deinit( 
    PI2C_CONTEXT pI2C
    );

DWORD 
HW_Open(
    PI2C_CONTEXT pI2C
    );

DWORD 
HW_Close(
    PI2C_CONTEXT pI2C
    );

DWORD 
HW_Read(
    PI2C_CONTEXT pI2C,
    DWORD  SlaveAddr,   // slave address
    UCHAR  WordAddr,    // starting word address
    PUCHAR pData,       // pdata
    DWORD  Count        // bytes to read
    );

DWORD 
HW_Write(
    PI2C_CONTEXT pI2C,  
    DWORD slvAddr,      // target slave address
    UCHAR addr,         // starting slave word address
    PUCHAR pData,       // buffer
    DWORD count         // nBytes to write
    );

BOOL
HW_PowerUp(
    PI2C_CONTEXT pI2C
   );

BOOL
HW_PowerDown(
    PI2C_CONTEXT pI2C
   );

BOOL
HW_PowerCapabilities(
    PI2C_CONTEXT pI2C,
    PPOWER_CAPABILITIES ppc
   );

BOOL
HW_PowerSet(
    PI2C_CONTEXT pI2C,
    PCEDEVICE_POWER_STATE pDx
   );

BOOL
HW_PowerGet(
    PI2C_CONTEXT pI2C,
    PCEDEVICE_POWER_STATE pDx
   );

#ifdef DEBUG
#define ZONE_ERR            DEBUGZONE(0)
#define ZONE_WRN            DEBUGZONE(1)
#define ZONE_INIT           DEBUGZONE(2)
#define ZONE_OPEN           DEBUGZONE(3)
#define ZONE_READ           DEBUGZONE(4)
#define ZONE_WRITE          DEBUGZONE(5)
#define ZONE_IOCTL          DEBUGZONE(6)
#define ZONE_IST            DEBUGZONE(7)
#define ZONE_POWER          DEBUGZONE(9)
//...
#define ZONE_TRACE          DEBUGZONE(15)

//
// these should be removed in the code if you can 'g' past these successfully
//
#define TEST_TRAP { \
   NKDbgPrintfW( TEXT("%s: Code Coverage Trap in: I2C, Line: %d\n"), TEXT(__FILE__), __LINE__); \
   DebugBreak();  \
}
#else
#define TEST_TRAP
#endif

#endif _I2C_DRV_H_

