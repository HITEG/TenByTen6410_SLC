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

    drv.c   I2C (logical) MDD

Abstract:

   Streams interface driver

Functions:

Notes:

--*/

#include <windows.h>
#include <nkintr.h>

#include <winreg.h>
#include <winioctl.h>
#include <ceddk.h>
#include <devload.h>

#include "drv.h"

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("I2C"), {
    TEXT("Error"), TEXT("Warn"),  TEXT("Init"),  TEXT("Open"),
    TEXT("Read"),  TEXT("Write"), TEXT("IOCTL"), TEXT("IST"),
    TEXT("Power"), TEXT("9"),     TEXT("10"),    TEXT("11"), 
    TEXT("12"),    TEXT("13"),    TEXT("14"),    TEXT("Trace"),
    },
    0x0003 // ZONE_WRN|ZONE_ERR
};
#endif  // DEBUG


BOOL
I2C_PowerUp(
   PVOID Context
   );

BOOL
I2C_PowerDown(
   PVOID Context
   );

BOOL
DllEntry(
    HINSTANCE   hinstDll,             /*@parm Instance pointer. */
    DWORD   dwReason,                 /*@parm Reason routine is called. */
    LPVOID  lpReserved                /*@parm system parameter. */
    )
{
    if ( dwReason == DLL_PROCESS_ATTACH ) {
        DEBUGREGISTER(hinstDll);
        DEBUGMSG (ZONE_INIT, (TEXT("I2C: Process Attach\r\n")));
    }

    if ( dwReason == DLL_PROCESS_DETACH ) {
        DEBUGMSG (ZONE_INIT, (TEXT("I2C: Process Detach\r\n")));
    }

    return(TRUE);
}


BOOL
GetRegistryData(PI2C_CONTEXT pI2C, LPCTSTR regKeyPath)
{
    LONG    regError;
    HKEY    hKey;
    DWORD   dwDataSize;

    DEBUGMSG(ZONE_INIT, (TEXT("Try to open %s\r\n"), regKeyPath));

    // We've been handed the name of a key in the registry that was generated
    // on the fly by device.exe.  We're going to open that key and pull from it
    // a value that is the name of this drivers's real key.  That key
    // will have the DeviceArrayIndex that we're trying to find.  
    hKey = OpenDeviceKey(regKeyPath);
    if ( hKey == NULL ) {
        DEBUGMSG(ZONE_INIT | ZONE_ERR,(TEXT("Failed to open device key\r\n")));
        return ( FALSE );        
    }

    dwDataSize = REG_MODE_VAL_LEN;
    regError = RegQueryValueEx(
                hKey, 
                REG_MODE_VAL_NAME, 
                NULL, 
                NULL,
                (LPBYTE)(&pI2C->Mode),
                &dwDataSize);

    if (regError)
        goto _done;


    dwDataSize = REG_SLAVEADDR_VAL_LEN;
    regError = RegQueryValueEx(
                hKey, 
                REG_SLAVEADDR_VAL_NAME,
                NULL, 
                NULL,
                (LPBYTE)(&pI2C->SlaveAddress),
                &dwDataSize);

    if (regError)
        goto _done;


_done:
    RegCloseKey (hKey);

    if ( regError != ERROR_SUCCESS ) {
        DEBUGMSG(ZONE_ERR, (TEXT("Failed to get registry values, Error 0x%X\r\n"),regError));
        return ( FALSE );
    }

    DEBUGMSG (ZONE_INIT,(TEXT("I2C_Init - Mode: %s\r\n"),
        pI2C->Mode == POLLING ? L"POLLING" : L"INTERRUPT"));

    return ( TRUE ); 
}

#ifdef UTLDRV
static
DWORD
InternalMapRegisters(
    PI2C_CONTEXT pI2C
    )
{
    HANDLE       hUtil;
    UTL_FASTCALL utlFc = {0};
    DWORD dwErr = ERROR_SUCCESS, bytes;

    // Open the util driver so we can map in our registers VA
    hUtil = CreateFile( TEXT("UTL0:"), GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, 0);
    
    if (INVALID_HANDLE_VALUE == hUtil) {
        dwErr = GetLastError();
        DEBUGMSG(ZONE_ERR, (TEXT("CreateFile ERROR: %d\n"), dwErr));
        goto _error_exit;
    }

    //  get it's FAST CALL function pointers.
    if( !DeviceIoControl(hUtil, 
                         IOCTL_UTL_GET_FASTCALL,
                         NULL, 0, 
                         &utlFc, sizeof(utlFc),
                         &bytes, NULL)) {
        dwErr = GetLastError();
        DEBUGMSG(ZONE_ERR, (TEXT("DeviceIoControl ERROR: %d\n"), dwErr));
        goto _error_exit;
    }

    // map in I2C registers
    dwErr = utlFc.GetRegisterVA(utlFc.pContext, 
                                IIC_BASE,
                                sizeof(IICreg),
                                FALSE, 
                                (DWORD *)&pI2C->pI2CReg);
    if ( dwErr ) {
        DEBUGMSG(ZONE_ERR, (TEXT("GetRegisterVA(IIC_BASE) ERROR: %d\n"), dwErr));
        goto _error_exit;
    }

    // map in GPIO
    dwErr = utlFc.GetRegisterVA(utlFc.pContext, 
                                IOP_BASE, 
                                sizeof(IOPreg),
                                FALSE, 
                                (DWORD *)&pI2C->pIOPReg);
    if ( dwErr ) {
        DEBUGMSG(ZONE_ERR, (TEXT("GetRegisterVA(IOP_BASE) ERROR: %d\n"), dwErr));
        goto _error_exit;
    }

    // map in CLK/PWR registers
    dwErr = utlFc.GetRegisterVA(utlFc.pContext, 
                                CLKPWR_BASE, 
                                sizeof(CLKPWRreg),
                                FALSE, 
                                (DWORD *)&pI2C->pCLKPWRReg);
    if ( dwErr ) {
        DEBUGMSG(ZONE_ERR, (TEXT("GetRegisterVA(CLKPWR_BASE) ERROR: %d\n"), dwErr));
        goto _error_exit;
    }

_error_exit:
    // We only mapped in registers, which remain in utildrv forever.
    // If we mapped in memory we would need to keep the handle and FreeMemVA later.
    if (INVALID_HANDLE_VALUE != hUtil)
        CloseHandle(hUtil);
        
    return dwErr;
}


#else

static
DWORD
InternalMapRegisters(
    PI2C_CONTEXT pI2C
    )
{
    PUCHAR  pVMem;
    BOOL    bMapReturn;
    DWORD   err = ERROR_SUCCESS;
  
    // reserve enough space for our registers
    pVMem = (PUCHAR)VirtualAlloc(0, PAGE_SIZE*3, MEM_RESERVE, PAGE_NOACCESS);
    
    if (pVMem) {
        // map in I2C registers
        bMapReturn = VirtualCopy( pVMem,
                                  (LPVOID)(S3C6410_BASE_REG_PA_IICBUS>>8),
                                  PAGE_SIZE,
                                  PAGE_READWRITE | PAGE_NOCACHE |PAGE_PHYSICAL);
        if (!bMapReturn) {
            err = GetLastError();
            DEBUGMSG(ZONE_ERR, (TEXT("Virtual Copy ERROR for IICBUS regs: %d\r\n"), err));
            return err;
        }
        pI2C->pI2CReg = (volatile S3C6410_IIC_REG*)(pVMem);

        // map in GPIO registers
        pVMem += PAGE_SIZE;
        bMapReturn = VirtualCopy(pVMem,
                                 (LPVOID)(S3C6410_BASE_REG_PA_GPIO>>8),
                                 PAGE_SIZE,
                                 PAGE_READWRITE | PAGE_NOCACHE |PAGE_PHYSICAL);
        if (!bMapReturn) {
            err = GetLastError();
            DEBUGMSG(ZONE_ERR, (TEXT("Virtual Copy ERROR for IOP regs: %d\r\n"), err));
            return err;
        }
        pI2C->g_pGPIOReg = (volatile S3C6410_GPIO_REG*)(pVMem);
        
        // map in CLK/PWR  registers 
        pVMem += PAGE_SIZE;
        bMapReturn = VirtualCopy(pVMem,
                                 (LPVOID)(S3C6410_BASE_REG_PA_SYSCON>>8),
                                 PAGE_SIZE,
                                 PAGE_READWRITE | PAGE_NOCACHE |PAGE_PHYSICAL);
        if (!bMapReturn) {
            err = GetLastError();
            DEBUGMSG(ZONE_ERR, (TEXT("Virtual Copy ERROR for CLK power regs: %d\r\n"), err));
            return err;
        }
        pI2C->g_pSYSCONReg = (volatile S3C6410_SYSCON_REG*)(pVMem);

    } else {
        err = GetLastError();
        DEBUGMSG(ZONE_ERR, (TEXT("Virtual Alloc ERROR: %d\r\n"), err));
    }
  
    return err;
}

#endif


BOOL
I2C_Deinit(
   PI2C_CONTEXT pI2C
   )
{
   DEBUGMSG(ZONE_INIT, (TEXT(">I2C_Deinit\n")));

    if (!pI2C)
        return FALSE;
        
    HW_Deinit(pI2C);
    
#ifndef UTLDRV
    if (pI2C->pI2CReg)
        VirtualFree((PVOID)pI2C->pI2CReg, 0, MEM_RELEASE);

    if (pI2C->g_pGPIOReg)
        VirtualFree((PVOID)pI2C->g_pGPIOReg, 0, MEM_RELEASE);

    if (pI2C->g_pSYSCONReg)
        VirtualFree((PVOID)pI2C->g_pSYSCONReg, 0, MEM_RELEASE);
#endif

	LocalFree(pI2C);

	DEBUGMSG(ZONE_INIT, (TEXT("<I2C_Deinit\n")));

	return TRUE;
}


/*++

Called by Device Manager to initialize the streams interface in response to ActivateDevice.
We passed ActivateDevice a pointer to our device context, but must read it out of the registry as "ClientInfo".

Returns context used in XXX_Open, XXX_PowerDown, XXX_PowerUp, and XXX_Deinit

--*/
PI2C_CONTEXT
I2C_Init(
   PVOID Context
   )
{
    LPTSTR ActivePath = (LPTSTR)Context; // HKLM\Drivers\Active\xx
    PI2C_CONTEXT pI2C;
    BOOL bRc = FALSE;

    DEBUGMSG(ZONE_INIT, (TEXT(">I2C_Init(%p)\n"), ActivePath));

    // Allocate for our main data structure and one of it's fields.
    pI2C = (PI2C_CONTEXT)LocalAlloc( LPTR,  sizeof(I2C_CONTEXT) );
    if ( !pI2C )
        return( NULL );

    pI2C->Sig = I2C_SIG;

    // read our config from registry
    if ( !GetRegistryData(pI2C, (LPCTSTR)Context) ) {
        DEBUGMSG (ZONE_ERR, (TEXT("I2C_Init - Unable to read registry data.  Failing Init !!! \r\n")));
        goto ALLOCFAILED;
    }

    // map in register space
    if ( InternalMapRegisters(pI2C) != ERROR_SUCCESS)
        goto ALLOCFAILED;

    if ( HW_Init(pI2C) != ERROR_SUCCESS)
        goto ALLOCFAILED;

    pI2C->Dx = D0;
        
    DEBUGMSG(ZONE_INIT, (TEXT("<I2C_Init:0x%x\n"), pI2C ));

    return (pI2C);

ALLOCFAILED:
    I2C_Deinit(pI2C);

    return NULL;
}


PI2C_CONTEXT
I2C_Open(
   PI2C_CONTEXT pI2C,       // context returned by I2C_Init.
   DWORD        AccessCode, // @parm access code
   DWORD        ShareMode   // @parm share mode
   )
{
    UNREFERENCED_PARAMETER(ShareMode);
    UNREFERENCED_PARAMETER(AccessCode);

    DEBUGMSG(ZONE_OPEN,(TEXT(">I2C_Open(0x%x, 0x%x, 0x%x)\n"),pI2C, AccessCode, ShareMode));

    pI2C->OpenCount++;

    HW_Open(pI2C);

    DEBUGMSG(ZONE_OPEN,(TEXT("<I2C_Open:%u\n"), pI2C->OpenCount ));

    return pI2C;
}


BOOL
I2C_Close(
   PI2C_CONTEXT pI2C
   )
{
   DEBUGMSG(ZONE_OPEN,(TEXT("I2C_Close(0x%x)\n"),pI2C));

    if ( pI2C->OpenCount ) {
        
        pI2C->OpenCount--;

        // BUGBUG: power off if no longer open
        HW_Close(pI2C);
    }

    return TRUE;
}


ULONG
I2C_Write(
   PI2C_CONTEXT pI2C,
   PUCHAR pBuffer,
   ULONG  BufferLength
   )
{
    return 0;
}


ULONG
I2C_Read(
   PI2C_CONTEXT pI2C,
   PUCHAR pBuffer,
   ULONG  BufferLength
   )
{
   return 0;
}



BOOL
I2C_IOControl(
    PI2C_CONTEXT pI2C,
    DWORD dwCode,
    PBYTE pBufIn,
    DWORD dwLenIn,
    PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
   )
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  bRc = TRUE;
    PUCHAR puc;

    DEBUGMSG(ZONE_IOCTL,(TEXT(">I2C_IOControl(0x%x, 0x%x, %d, 0x%x)\n"),
        dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut ));

    if ( !pI2C || !pI2C->OpenCount ) {
        DEBUGMSG (ZONE_IOCTL|ZONE_ERR, (TEXT("I2C_IOControl: ERROR_INVALID_HANDLE\r\n")));
        SetLastError (ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (pdwActualOut)
        *pdwActualOut = 0;

    //EnterCriticalSection(&pI2C->RegCS);

    switch (dwCode) {
        //
        // IOCTL_I2C_xxx
        //
        case IOCTL_I2C_READ:
            if ( (dwLenIn < sizeof(I2C_IO_DESC)) || !pBufIn || !((PI2C_IO_DESC)pBufIn)->Data ) {
                dwErr = ERROR_INVALID_PARAMETER;
                bRc = FALSE;
                break;
            }
            
            puc = (PUCHAR)MapPtrToProcess(((PI2C_IO_DESC)pBufIn)->Data, pI2C->hProc);
            
            dwErr = HW_Read(pI2C,
                            ((PI2C_IO_DESC)pBufIn)->SlaveAddr,
                            ((PI2C_IO_DESC)pBufIn)->WordAddr, 
                             puc,
                            ((PI2C_IO_DESC)pBufIn)->Count);
            
            UnMapPtr(puc);

            if ( ERROR_SUCCESS == dwErr ) {
                if (pdwActualOut)
                    *pdwActualOut = ((PI2C_IO_DESC)pBufIn)->Count;
            } else
                bRc = FALSE;
            
            break;

        case IOCTL_I2C_WRITE:
            if ( (dwLenIn < sizeof(I2C_IO_DESC)) || !pBufIn || !((PI2C_IO_DESC)pBufIn)->Data) {
                dwErr = ERROR_INVALID_PARAMETER;
                bRc = FALSE;
                break;
            }
            
            puc = (PUCHAR)MapPtrToProcess(((PI2C_IO_DESC)pBufIn)->Data, pI2C->hProc );
            
            dwErr = HW_Write(pI2C,
                             ((PI2C_IO_DESC)pBufIn)->SlaveAddr,
                             ((PI2C_IO_DESC)pBufIn)->WordAddr, 
                             puc,
                             ((PI2C_IO_DESC)pBufIn)->Count);

            UnMapPtr(puc);

            if ( ERROR_SUCCESS == dwErr ) {
                if (pdwActualOut)
                    *pdwActualOut = ((PI2C_IO_DESC)pBufIn)->Count;
            } else
                bRc = FALSE;
            
            break;

        case IOCTL_I2C_GET_FASTCALL:
            if ( (dwLenOut < sizeof(I2C_FASTCALL)) || !pBufOut ) {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            // Check caller process & fail if they are not in device.exe!
            if (GetCallerProcess() != pI2C->hProc ) {
                DEBUGMSG (ZONE_ERR, (TEXT("ERROR_ACCESS_DENIED: Caller(0x%X) != Current(0x%X)\r\n"), 
                    GetCallerProcess(), pI2C->hProc ));
                bRc = FALSE;
                dwErr = ERROR_ACCESS_DENIED;
                break;
            }
            
            ((PI2C_FASTCALL)pBufOut)->Context  = pI2C;
            ((PI2C_FASTCALL)pBufOut)->I2CRead  = HW_Read;
            ((PI2C_FASTCALL)pBufOut)->I2CWrite = HW_Write;

            if (pdwActualOut)
                *pdwActualOut = sizeof(I2C_FASTCALL);

            break;

        //
        // Power Management
        //
        case IOCTL_POWER_CAPABILITIES:
            if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(POWER_CAPABILITIES)) ) {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            bRc = HW_PowerCapabilities(pI2C, (PPOWER_CAPABILITIES)pBufOut);
            if ( bRc ) {
                *pdwActualOut = sizeof(POWER_CAPABILITIES);
            }
            break;

        case IOCTL_POWER_SET: 
            if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(CEDEVICE_POWER_STATE)) ) {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            bRc = HW_PowerSet(pI2C, (PCEDEVICE_POWER_STATE)pBufOut);
            if ( bRc ) {
                *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
            }
            break;

        case IOCTL_POWER_GET: 
            if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(CEDEVICE_POWER_STATE)) ) {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            bRc = HW_PowerGet(pI2C, (PCEDEVICE_POWER_STATE)pBufOut);
            if ( bRc ) {
                *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
            }
            break;


        default:
            bRc  = FALSE;
            dwErr = ERROR_INVALID_FUNCTION;
            DEBUGMSG (ZONE_ERR, (TEXT("I2C_IOControl Unknown Ioctl: 0x%X\r\n"), dwCode));
            break;            
    }

    //LeaveCriticalSection(&pI2C->RegCS);

    if ( !bRc ) {
        DEBUGMSG (ZONE_ERR, (TEXT("I2C_IOControl ERROR: %u\r\n"), dwErr));
        SetLastError(dwErr);
    }

    DEBUGMSG(ZONE_IOCTL,(TEXT("<I2C_IOControl:%d\n"), bRc));

    return bRc;
}


ULONG
I2C_Seek(
   PVOID Context,
   LONG  Position,
   DWORD Type
   )
{
    return (ULONG)-1;
}


BOOL
I2C_PowerUp(
   PVOID Context
   )
{
    return HW_PowerUp(Context);
}


BOOL
I2C_PowerDown(
   PVOID Context
   )
{
    return HW_PowerDown(Context);
}


// EOF

