/*
**************************************************************************
FileName   : Led.c
Description: S3C6410 (C), Led driver
Date       : 11/05/2009
**************************************************************************
*/
#include <windows.h>
#include <s3c6410.h>
#include <ceddk.h>
#include "bsp_cfg.h"

#include "Led.h"




#if DEBUG
#define ZONE_ERROR          DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_FUNCTION       DEBUGZONE(2)
#define ZONE_INIT           DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST            DEBUGZONE(5)

DBGPARAM dpCurSettings =
{
    TEXT("LED"),
    {
         TEXT("Errors"),TEXT("Warnings"),TEXT("Function"),TEXT("Init"),
         TEXT("Info"),TEXT("Ist"),TEXT("Undefined"),TEXT("Undefined"),
         TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),
         TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined")
    },
        (1 << 0)   // Errors
    |   (1 << 1)   // Warnings
};
#endif


volatile S3C6410_GPIO_REG *v_pIOP_LED_regs = NULL;

#define     GPM0		0
#define     GPM1		4
#define     GPM2		8
#define     GPM3		12

#define     LED1           GPM0
#define     LED2           GPM1
#define     LED3           GPM2
#define     LED4           GPM3
// LED Control  bit

#define     LED1_BIT   0
#define     LED2_BIT   1
#define     LED3_BIT   2
#define     LED4_BIT   3
// #define     Led_On(x)		(v_pIOP_LED_regs->GPMDAT |= (0x1 << (x)))
// #define     Led_Off(x)		(v_pIOP_LED_regs->GPMDAT &= ~(0x1 << (x)))
static void Led_On(int x)
{
	v_pIOP_LED_regs->GPMDAT |= (0x1 << (x));
	v_pIOP_LED_regs->GPNDAT &= ~(0x1 << (x+2));
}

static void Led_Off(int x)
{
	v_pIOP_LED_regs->GPMDAT &= ~(0x1 << (x));
	v_pIOP_LED_regs->GPNDAT |= (0x1 << (x+2));
	
}


void  InitLedAddr()
{
	//DWORD dwIOBase;
	DWORD dwIOSize;
	PHYSICAL_ADDRESS  IOPhyAdr= { S3C6410_BASE_REG_PA_GPIO, 0 };
	DEBUGMSG(ZONE_INFO, (TEXT("++InitLedAddr\r\n")));
	dwIOSize = sizeof(S3C6410_GPIO_REG);
	if ( NULL == v_pIOP_LED_regs)
	{
		v_pIOP_LED_regs = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(IOPhyAdr, (ULONG)dwIOSize, FALSE);	
		if ( NULL == v_pIOP_LED_regs)
		{
			DEBUGMSG(ZONE_ERROR, (TEXT("LED init address failed\r\n")));
		}
	}
}

void GPIOInit()
{

	if (v_pIOP_LED_regs)
	{
		//GPM0~GPM3
		v_pIOP_LED_regs->GPMPUD &= ~((0x3<<0)|(0x3<<2)|(0x3<<4)|(0x3<<6));//Disable GPD0, GPD4 pull up/down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<0);//Disable GPM0 pull down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<2);//Disable GPM1 pull down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<4);//Disable GPM2 pull down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<6);//Disable GPM3 pull down
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<0);
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<4);
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<8);
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<12);
		v_pIOP_LED_regs->GPMCON |= (0x1<<0);//Disable GPD0, GPD4 as output
		v_pIOP_LED_regs->GPMCON |= (0x1<<4);//Disable GPD0, GPD4 as output
		v_pIOP_LED_regs->GPMCON |= (0x1<<8);
		v_pIOP_LED_regs->GPMCON |=(0x1<<12);

//****************************************************************************
		//GPN2~GPN5
		v_pIOP_LED_regs->GPNPUD &= ~((0x3<<2)|(0x3<<4)|(0x3<<6)|(0x3<<8));//Disable GPD0, GPD4 pull up/down
		v_pIOP_LED_regs->GPNPUD |= (0x1<<2);//Disable GPM0 pull down
		v_pIOP_LED_regs->GPNPUD |= (0x1<<4);//Disable GPM1 pull down
		v_pIOP_LED_regs->GPNPUD |= (0x1<<6);//Disable GPM2 pull down
		v_pIOP_LED_regs->GPNPUD |= (0x1<<8);//Disable GPM3 pull down
		v_pIOP_LED_regs->GPNCON &= ~(0x3<<4);
		v_pIOP_LED_regs->GPNCON &= ~(0x3<<6);
		v_pIOP_LED_regs->GPNCON &= ~(0x3<<8);
		v_pIOP_LED_regs->GPNCON &= ~(0x3<<10);
		v_pIOP_LED_regs->GPNCON |= (0x1<<4);//Disable GPD0, GPD4 as output
		v_pIOP_LED_regs->GPNCON |= (0x1<<6);//Disable GPD0, GPD4 as output
		v_pIOP_LED_regs->GPNCON |= (0x1<<8);
		v_pIOP_LED_regs->GPNCON |= (0x1<<10);

		Led_Off(LED1_BIT);
		Led_Off(LED2_BIT);
		Led_Off(LED3_BIT);
		Led_Off(LED4_BIT);
	}
}

void ControlLed(UINT8 u8Index, BOOL bHigh)
{
	if( bHigh)
		Led_On(u8Index);
	else
		Led_Off(u8Index);
}
	
BOOL ProcLed (UINT8 u8Index, BOOL bHigh)
{
	DEBUGMSG(ZONE_INFO, (TEXT("ProcLed+++\r\n")));
	if (u8Index>2)
	{
		DEBUGMSG(ZONE_ERROR, (TEXT("input error,exit function")));
		return FALSE;
	}
	
	switch(u8Index)
	{
		case  1:
			ControlLed( LED1_BIT, bHigh);
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
			break;
		case  2:
			ControlLed( LED2_BIT, bHigh);
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
		break;
		case  3:
			ControlLed( LED3_BIT, bHigh);
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
			break;
		case  4:
			ControlLed( LED4_BIT, bHigh);
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			DEBUGMSG(ZONE_FUNCTION, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
		break;

		default:
			break;
	}
	DEBUGMSG(ZONE_FUNCTION, (TEXT("Operate LED: %d\r\n"), u8Index));
	DEBUGMSG(ZONE_INFO, (TEXT("ProcLed---\r\n")));
	return TRUE;
}
BOOL WINAPI DllMain(HANDLE hinstDll, ULONG Reason, LPVOID Reserved)
{
	switch(Reason)
	{
		case DLL_PROCESS_ATTACH:
			DEBUGREGISTER(hinstDll);
			DEBUGMSG (ZONE_INIT, (TEXT("process attach\r\n")));
			DisableThreadLibraryCalls((HMODULE) hinstDll);
			break;
			
		case DLL_PROCESS_DETACH:
			DEBUGMSG(ZONE_INIT,(TEXT("LED: DLL_PROCESS_DETACH\r\n")));
			break;
			
		default:
			break;
	}
	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL LED_Close(DWORD dwOpenContext)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Close+++\r\n")));
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Close---\r\n")));	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL LED_Deinit(DWORD hDeviceContext)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Deinit+++\r\n")));
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Deinit---\r\n")));	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Init(
  LPCTSTR pContext, 
  LPCVOID lpvBusContext
)
{
	DEBUGMSG(ZONE_INIT, (TEXT("LED_Init+++\r\n")));
	InitLedAddr();
	GPIOInit();
	DEBUGMSG(ZONE_INIT, (TEXT("LED_Init---\r\n")));

	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL LED_IOControl(DWORD dwOpenContext, DWORD dwIoControlCode, LPBYTE lpInBuf, 
                   DWORD nInBufSize, LPBYTE lpOutBuf, DWORD nOutBufSize, 
                   LPDWORD lpBytesReturned)
{
	DEBUGMSG(ZONE_INFO,(TEXT("LED_IOControl+++\r\n")));
	DEBUGMSG(ZONE_INFO,(TEXT("dwIoControlCode = %d.\r\n"), dwIoControlCode));
	switch(dwIoControlCode)
	{
		case IOCTL_LED_ON:
			if(NULL != lpInBuf)
			{
				ProcLed(*lpInBuf, TRUE);
				DEBUGMSG(ZONE_FUNCTION,(TEXT("LED_IOControl: IO_LED_ON\r\n")));
			}
			else
				DEBUGMSG(ZONE_FUNCTION, (TEXT("The lpInBuf is NULL\r\n")));
			break;
			
		case IOCTL_LED_OFF:
			if(NULL != lpInBuf)
			{
				ProcLed(*lpInBuf, FALSE);
				DEBUGMSG(ZONE_FUNCTION,(TEXT("LED_IOControl: IO_LED_OFF\r\n")));
			}
			else
				DEBUGMSG(ZONE_FUNCTION, (TEXT("The lpInBuf is NULL\r\n")));
			break;
			
		default:
			DEBUGMSG(ZONE_FUNCTION,(TEXT("LED_IOControl: default\r\n")));
			break;
	}
	DEBUGMSG(ZONE_INFO,(TEXT("LED_IOControl---\r\n")));
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Open(DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Open+++\r\n")));
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Open---\r\n")));	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LED_PowerDown(DWORD hDeviceContext)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_PowerDown+++\r\n")));
	Led_Off(1);
	Led_Off(2);
	Led_Off(3);
	Led_Off(4);
	DEBUGMSG(ZONE_INFO, (TEXT("LED_PowerDown---\r\n")));	
	return;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LED_PowerUp(DWORD hDeviceContext)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_PowerUp+++\r\n")));
	DEBUGMSG(ZONE_INFO, (TEXT("LED_PowerUp---\r\n")));	
	return;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Read+++\r\n")));
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Read---\r\n")));	
    return TRUE;
} 

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Seek(DWORD hOpenContext, long Amount, DWORD Type)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Seek+++\r\n")));
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Seek---\r\n")));	
    return TRUE;
} 

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Write(DWORD hOpenContext, LPCVOID pSourceBytes, DWORD NumberOfBytes)
{
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Write+++\r\n")));
	DEBUGMSG(ZONE_INFO, (TEXT("LED_Write---\r\n")));	
    return TRUE;
}