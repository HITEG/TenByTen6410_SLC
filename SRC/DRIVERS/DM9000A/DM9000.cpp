/********************************************************************************
 * 
 * $Id: dm9isa.cpp update v2.4 2010/09/14 by a.x.w fae $
 *
 * Copyright (c) 2000-2005 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/
#include	"dm9000.h"

#define DM9000_ADDR_OFFSET     0
#define DM9000_DATA_OFFSET     4
#define CMD_AUTO_Detect
#define CMD_MAX_ADDR		0x8000			//addr 15

#define IO_OALStall_SET         0
//#define RX_CRC_REC
#define MAX_ONE_TIME_REV_PACKET 0xff

#define INT_Edge_Detect
#define INT_Edge_Detect_Time    3                   //ms

//#define SOFT_INT_CHECK
#define SOFT_INT_CHECK_Time     10

#ifdef SOFT_INT_CHECK
    #undef INT_Edge_Detect
#endif

//#define SOFT_FIRST_RESET		20
//#define LINK_OFF_RESET		3





BOOL PROCESSOR_LanInterruptInit(int nINTMode)
{
	return TRUE;
}  

BOOL PROCESSOR_LanDataWidthInit(int nIoMode) //()
{    
	return TRUE;
}

void PROCESSOR_LanEnableInterrupt(void)
{

}

void PROCESSOR_LanClearInterruptPending(void)  // No Need [Testing...]
{

}

#if (IO_OALStall_SET)
#define IO_OALStall                     OALStall(IO_OALStall_SET)  //OALStall(30)
void OALStall(UINT32 microSeconds)
{
    if (0 == microSeconds)
        return ;
        
    volatile UINT32 i = (microSeconds / 2);
    
    if (microSeconds)
    {
        while (--i)
            ;
    }
    
    return;
}
#else
#define IO_OALStall
#endif

/*******************************************************************************
 *
 *
 *
 ********************************************************************************/
CONFIG_PARAMETER g_szDm9ConfigParams[] =
{
    { CID_IO_BASE_ADDRESS, 0x00000300, NDIS_STRING_CONST("IoAddress")},
    { CID_IRQ_NUMBER, 3, NDIS_STRING_CONST("IrqNumber")},
    { CID_INT_ACTIVE, INT_ACTIVE_HIGH, NDIS_STRING_CONST("Int_Active_High")},
    { CID_FC_ENABLE, FLOW_CONTROL_ENABLE, NDIS_STRING_CONST("Flow_Control_Enable")},
    { CID_MAC_LOAD_MODE, 0, NDIS_STRING_CONST("MAC_Load_Mode")},
    { CID_MAC_ADDR_0, 0x00, NDIS_STRING_CONST("MAC_ADDR_0")},
    { CID_MAC_ADDR_1, 0x22, NDIS_STRING_CONST("MAC_ADDR_1")},
    { CID_MAC_ADDR_2, 0x44, NDIS_STRING_CONST("MAC_ADDR_2")},
    { CID_MAC_ADDR_3, 0x66, NDIS_STRING_CONST("MAC_ADDR_3")},
    { CID_MAC_ADDR_4, 0x88, NDIS_STRING_CONST("MAC_ADDR_4")},
    { CID_MAC_ADDR_5, 0xaa, NDIS_STRING_CONST("MAC_ADDR_5")},
    { -1, -1, NULL}
};

/*******************************************************************************
 *
 * Device attributes and characteristics
 *
 ********************************************************************************/
PCONFIG_PARAMETER C_DM9000::DeviceConfigureParameters(void)
{
    return (PCONFIG_PARAMETER)&g_szDm9ConfigParams[0];
}

void C_DM9000::DeviceSetEepromFormat(void)
{
    m_szEepromFormat[EID_MAC_ADDRESS] = 0;
    m_szEepromFormat[EID_VENDOR_ID] = 8;
    m_szEepromFormat[EID_PRODUCT_ID] = 10;
}

void C_DM9000::EDeviceRegisterIoSpace(void)
{
    NIC_DEVICE_OBJECT::EDeviceRegisterIoSpace();
    
    /************************************************/
    
    if (!PROCESSOR_LanDataWidthInit(WORD_MODE))
    {
        THROW((ERR_STRING("PROCESSOR_LanDataWidthInit error \r\n")));
        return;
    }
    
    if (!PROCESSOR_LanInterruptInit(m_szConfigures[CID_INT_ACTIVE]))
    {
        THROW((ERR_STRING("PROCESSOR_LanInterruptInit error \r\n")));
        return;
    }
    
    /************************************************/
    
    U32 val;
    U32 cmd_check;
    
    m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS] = m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_ADDR_OFFSET;
    
#ifdef CMD_AUTO_Detect
    for (cmd_check = 2; cmd_check < CMD_MAX_ADDR; cmd_check <<= 1)
    {
        m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS] = m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + cmd_check;
        DEBUG_PRINT((TEXT("[DM9ISA_d]: cmd Detect is %08X !\r\n"), m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS]));
//        RETAILMSG(TRUE,(TEXT("[DM9ISA_d]: cmd Detect is %08X !\r\n"), m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS]));
        val = DeviceReadPort(DM9_VIDL);
        DEBUG_PRINT((TEXT("[DM9ISA_d]: cmd val is %08X !\r\n"), val));
//        RETAILMSG(TRUE,(TEXT("[DM9ISA_d]: cmd val is %08X !\r\n"), val));

        if (0x46 == val)
            break;
    }
#else
    m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS] = m_szCurrentSettings[SID_PORT_BASE_ADDRESS] + DM9000_DATA_OFFSET;
#endif
    
    RETAILMSG(TRUE, (TEXT("[DM9ISA]: REG_INDEX_ADDRESS is %08X !\r\n"), m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS]));
    RETAILMSG(TRUE, (TEXT("[DM9ISA]: REG_DATA_ADDRESS is %08X !\r\n"), m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS]));
    
    if (CMD_MAX_ADDR == cmd_check)
    {
        RETAILMSG(TRUE, (TEXT("[DM9ISA]: Unknown device io error is %08X !\r\n"), m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS]));
        THROW((ERR_STRING("Unknown device io error %x"), m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS]));
    }
    
    val  = DeviceReadPort(DM9_VIDL);
    val |= DeviceReadPort(DM9_VIDH) << 8;
    val |= DeviceReadPort(DM9_PIDL) << 16;
    val |= DeviceReadPort(DM9_PIDH) << 24;
    
    if (0x90000a46 != val)
    {
        RETAILMSG(TRUE, (TEXT("Unknown device id error %x !\r\n"), val));
        THROW((ERR_STRING("Unknown device id error %x"), val));
    }
    else
    {
        RETAILMSG(TRUE, (TEXT("[DM9ISA]: signature is %08X !\r\n"), val));
        
        if (INT_ACTIVE_HIGH == m_szConfigures[CID_INT_ACTIVE])
        {
            DeviceWritePort(DM9_INTFN, 0);
        }
        else
        {
            DeviceWritePort(DM9_INTFN, 1);
        }
        
        RETAILMSG(TRUE, (TEXT("[DM9ISA]: reg DM9_INTFN is %x \r\n"), DeviceReadPort(DM9_INTFN)));
        RETAILMSG(TRUE, (TEXT("[DM9ISA]: reg DM9_CHIPREV is %x \r\n"), DeviceReadPort(DM9_CHIPREV)));
        
        if (DeviceReadPort(DM9_INTFN) & 0x01)
        {
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: IRQ pin active is LOW \r\n")));
        }
        else
        {
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: IRQ pin active is HIGH \r\n")));
        }
        
        if (0x1a == DeviceReadPort(DM9_CHIPREV))
            DeviceWritePort(DM9_BUSPW, 0x6b);
        else
            DeviceWritePort(DM9_BUSPW, 0x2b);

		DeviceWritePort(DM9_GPCR, 0x01);
        DeviceWritePort(DM9_GPR, 0x01);
        DevicePolling(0x28, 0xff, 0x46, 100, 10);
    }
}

void C_DM9000::EDeviceValidateConfigurations(void)
{
    NDIS_HANDLE  hndis = m_pUpper->GetNdisHandle();
    
    // validate slot number
    
    if ((m_szConfigures[CID_IO_BASE_ADDRESS] == -1) ||
            (m_szConfigures[CID_IRQ_NUMBER] == -1) )
            THROW(());
        
    m_szConfigures[CID_CHECK_FOR_HANG_PERIOD] = 5;      // sec
    
    m_szConfigures[CID_IRQ_GEN_TYPE] = NdisInterruptLatched;
    
    m_szConfigures[CID_IRQ_SHARED] = TRUE;
    
    m_szConfigures[CID_IRQ_LEVEL] = 0x0F;
    
    m_szConfigures[CID_INTERFACE_TYPE] = NdisInterfaceIsa;
    
    m_szConfigures[CID_BUS_MASTER] = FALSE;
    
    // set receive mode
    // <5> discard long packet
    // <4> discard CRC error packet
    // <0> rx enable
#ifndef RX_CRC_REC
    m_szCurrentSettings[SID_OP_MODE] = MAKE_MASK3(5, 4, 0);
#else
    m_szCurrentSettings[SID_OP_MODE] = MAKE_MASK2(5, 0);
#endif
    
    m_szCurrentSettings[SID_802_3_MAXIMUM_LIST_SIZE] = DM9_MULTICAST_LIST;
    
    if (FLOW_CONTROL_ENABLE == m_szConfigures[CID_FC_ENABLE])
    {
        m_szCurrentSettings[SID_GEN_MAC_OPTIONS] |= NDIS_MAC_OPTION_FULL_DUPLEX;
    }
}


/*******************************************************************************
 *
 * Device access routines
 *
 ********************************************************************************/
#if 1
#define ENTER_CRITICAL_SECTION          m_spinAccessToken.Lock();
#define LEAVE_CRITICAL_SECTION          m_spinAccessToken.Release();
#else
#define ENTER_CRITICAL_SECTION          CSpinlock spinAccessToken;                  \
                                        spinAccessToken.Lock();
#define LEAVE_CRITICAL_SECTION          spinAccessToken.Release();
#endif


U32 C_DM9000::DeviceWritePort(
    U32 uPort,
    U32 uValue)
{
    ENTER_CRITICAL_SECTION;
    
    NdisRawWritePortUchar(m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS], (U8)uPort);
    IO_OALStall;
    NdisRawWritePortUchar(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (U8)uValue);
    IO_OALStall;
    
    LEAVE_CRITICAL_SECTION;
    
    return uValue;
}


U32 C_DM9000::DeviceReadPort(
    U32 uPort)
{
    U16 uValue;
    
    ENTER_CRITICAL_SECTION;
    
    NdisRawWritePortUchar(m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS], (U8)uPort);
    IO_OALStall;
    NdisRawReadPortUchar(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], &uValue);
    IO_OALStall;
    
    LEAVE_CRITICAL_SECTION;
    
    return (U32)uValue;
}

U16 C_DM9000::DeviceReadEeprom(
    U32 uWordAddress)
{
    U16 highbyte, lowbyte;
    
    // assign the register offset
    DeviceWritePort(DM9_EPADDR, uWordAddress);
    
    // issue EEPROM read command<2>
    DeviceWritePort(DM9_EPCNTL, MAKE_MASK1(2));
    
    // wait until status bit<0> cleared
    
    if (!DevicePolling(DM9_EPCNTL, 0xff, MAKE_MASK1(2), 100, 5))
        return (U16) - 1;
        
    // stop command
    DeviceWritePort(DM9_EPCNTL, 0);
    
    // retrive data
    highbyte = (U16)DeviceReadPort(DM9_EPHIGH);
    lowbyte  = (U16)DeviceReadPort(DM9_EPLOW);
    
    return (MAKE_WORD(highbyte, lowbyte));
}

U16 C_DM9000::DeviceWriteEeprom(
    U32  uWordAddress,
    U16  uValue)
{
    // assign the register offset
    DeviceWritePort(DM9_EPADDR, uWordAddress);
    
    // put data
    DeviceWritePort(DM9_EPHIGH, HIGH_BYTE(uValue));
    DeviceWritePort(DM9_EPLOW, LOW_BYTE(uValue));
    
    // issue EEPROM write enable<4> and write command<1>
    DeviceWritePort(DM9_EPCNTL, MAKE_MASK2(4, 1));
    
    // wait until status bit<0> cleared
    DevicePolling(DM9_EPCNTL, 0xff, MAKE_MASK2(4, 1), 200, 10);
    
    // stop command
    DeviceWritePort(DM9_EPCNTL, 0);
    
    return uValue;
}

U16 C_DM9000::DeviceReadPhy(
    U32  uPhyaddress,
    U32  uRegister)
{
    U16  highbyte, lowbyte;
    
    // assign the phy register offset, internal phy(0x40) plus phy offset
    DeviceWritePort(DM9_EPADDR, ((uPhyaddress & 0x03) << 6 | (uRegister & 0x3F)));
    
    // issue PHY select<3> and PHY read command<2>
    DeviceWritePort(DM9_EPCNTL, MAKE_MASK2(2, 3));
    
    // wait until status bit<0> cleared
    DevicePolling(DM9_EPCNTL, 0xff, MAKE_MASK2(2, 3), 100, 5);
    
    // stop command
    DeviceWritePort(DM9_EPCNTL, 0);
    
    // retrive data
    highbyte = (U16)DeviceReadPort(DM9_EPHIGH);
    lowbyte  = (U16)DeviceReadPort(DM9_EPLOW);
    
    return (MAKE_WORD(highbyte, lowbyte));
}

U16 C_DM9000::DeviceWritePhy(
    U32  uPhyaddress,
    U32  uRegister,
    U16  uValue)
{
    // assign the phy register offset, internal phy(0x40) plus phy offset
    DeviceWritePort(DM9_EPADDR, ((uPhyaddress & 0x03) << 6 | (uRegister & 0x3F)));
    
    // put data
    DeviceWritePort(DM9_EPHIGH, HIGH_BYTE(uValue));
    DeviceWritePort(DM9_EPLOW, LOW_BYTE(uValue));
    
    // issue PHY select<3> and write command<1>
    DeviceWritePort(DM9_EPCNTL, MAKE_MASK2(1, 3));
    
    // wait until status bit<0> cleared
    DevicePolling(DM9_EPCNTL, 0xff, MAKE_MASK2(1, 3), 150, 10);
    
    // stop command
    DeviceWritePort(DM9_EPCNTL, 0);
    
    return uValue;
}


U32  C_DM9000::DeviceReadData(void)
{
    U32  value;
    
    return *(PU32)DeviceReadString((PU8)&value, sizeof(value));
}

U32  C_DM9000::DeviceReadDataWithoutIncrement(void)
{
    U32  value, tmp;
    
    ENTER_CRITICAL_SECTION;
    
    tmp = DeviceReadPort(DM9_MRCMDX);
    tmp = DeviceReadPort(DM9_ISR);
    
    NdisRawWritePortUchar(m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS], DM9_MRCMDX);
    IO_OALStall;
    
    switch (m_nIoMode)
    {
    
        case BYTE_MODE:
            NdisRawReadPortUchar(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (PU8)&value);
            value = (value & 0x000000FF);
            break;
            
        case WORD_MODE:
            NdisRawReadPortUshort(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (PU16)&value);
            value = (value & 0x0000FFFF);
            break;
            
        case DWORD_MODE:
            NdisRawReadPortUlong(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (PU32)&value);
            break;
            
        default:
            break;
    } // of switch
    
    IO_OALStall;
    
    LEAVE_CRITICAL_SECTION;
    
    return value;
}


PU8  C_DM9000::DeviceReadString(
    PU8  ptrBuffer,
    int  nLength)
{
    int  count;
    
    count = (nLength + m_nIoMaxPad) / m_nIoMode;
    
    // select port to be read from
    ENTER_CRITICAL_SECTION;
    
    NdisRawWritePortUchar(m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS], DM9_MRCMD);
    IO_OALStall;
    
    switch (m_nIoMode)
    {
    
        case BYTE_MODE:
            NdisRawReadPortBufferUchar(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], ptrBuffer, count);
            break;
            
        case WORD_MODE:
            NdisRawReadPortBufferUshort(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (PU16)ptrBuffer, count);
            break;
            
        case DWORD_MODE:
            NdisRawReadPortBufferUlong(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (PU32)ptrBuffer, count);
            break;
            
        default:
            break;
    } // of switch
    
    IO_OALStall;
    
    LEAVE_CRITICAL_SECTION;
    
#if DBG
    DEBUG_PRINT((TEXT("\r\n [DM9ISA_d]: read packet len %d"), nLength));
    
    for (U16 i = 0 ; i < count; i ++)
    {
        if (!(i % 8))
        {
            DEBUG_PRINT((TEXT("\r\n %04x ==> "), i));
        }
        
        DEBUG_PRINT((TEXT("%04x "), ((PU16)ptrBuffer)[i]));
    }
    DEBUG_PRINT((TEXT("\r\n")));
    
#endif
    
    return ptrBuffer;
}


PU8  C_DM9000::DeviceWriteString(
    PU8  ptrBuffer,
    int  nLength)
{
    int  count;
    
    count = (nLength + m_nIoMaxPad) / m_nIoMode;
    
    // select port to be read from
    ENTER_CRITICAL_SECTION;
    
    NdisRawWritePortUchar(m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS], DM9_MWCMD);
    IO_OALStall;
    
    switch (m_nIoMode)
    {
    
        case BYTE_MODE:
            NdisRawWritePortBufferUchar(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], ptrBuffer, count);
            break;
            
        case WORD_MODE:
            NdisRawWritePortBufferUshort(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (PU16)ptrBuffer, count);
            break;
            
        case DWORD_MODE:
            NdisRawWritePortBufferUlong(m_szCurrentSettings[SID_PORT_REG_DATA_ADDRESS], (PU32)ptrBuffer, count);
            break;
            
        default:
            break;
    } // of switch
    
    IO_OALStall;
    
    LEAVE_CRITICAL_SECTION;
    
#if DBG
    DEBUG_PRINT((TEXT("\r\n [DM9ISA_d]: write packet len %d"), nLength));
    
    for (U16 i = 0 ; i < count; i ++)
    {
        if (!(i % 8))
        {
            DEBUG_PRINT((TEXT("\r\n %04x ==> "), i));
        }
        
        DEBUG_PRINT((TEXT("%04x "), ((PU16)ptrBuffer)[i]));
    }
    
    DEBUG_PRINT((TEXT("\r\n")));
    
#endif
    
    return ptrBuffer;
}

void C_DM9000::PRINT_REG_DATA(U8 p_level)
{
	U16 reg_index[] = 
	{
		0x00, 0x02, 0xfe, 0xff, 0x05, 0x38, 0x39, 0x2c, 0x31, 0x32, 0x28,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
		0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x55aa
	};
	
	U16 i;
	
	REG_INDEX_SAVE();
	
	for(i = 0; 0x55aa != reg_index[i]; i++)
	{
		DEBUG_PRINT((TEXT("[DM9ISA_d]: PRINT_REG_DATA reg %02x == %02x \r\n"), reg_index[i] , DeviceReadPort(reg_index[i])));
		RETAILMSG(TRUE, (TEXT("[DM9ISA_d]: PRINT_REG_DATA reg %02x == %02x \r\n"), reg_index[i] , DeviceReadPort(reg_index[i])));
		
		if((p_level < 1) && (0x28 == reg_index[i])) break;
	}
	
	REG_INDEX_RESTORE();
}

void C_DM9000::TX_SEND_CHECK(void)
{
	U8 tcr_status;
    if (m_nTxPendings)
    {
    	tcr_status = (U8)DeviceReadPort(DM9_TXCR);
        if (tcr_status & MAKE_MASK1(0))
        {
        	DEBUG_PRINT((TEXT("[DM9ISA]: TX_SEND_CHECK no free %x \r\n"), tcr_status));
        }
        else
        {
            m_nTxPendings = 0;
            DeviceWritePort(DM9_ISR, 0x02);
            DeviceSendCompleted(m_TQWaiting.Dequeue());
            DeviceIndication(NIC_IND_TX_IDLE);
//			NdisMSendResourcesAvailable(m_pUpper->GetNdisHandle());
        }
	}
}

void C_DM9000::REG_INDEX_SAVE(void)
{
    ENTER_CRITICAL_SECTION;
    IO_OALStall;
    NdisRawReadPortUchar(m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS], &reg_index_save_data);
    IO_OALStall;
	LEAVE_CRITICAL_SECTION;
}

void C_DM9000::REG_INDEX_RESTORE(void)
{
    ENTER_CRITICAL_SECTION;
    IO_OALStall;
    NdisRawWritePortUchar(m_szCurrentSettings[SID_PORT_REG_INDEX_ADDRESS], (U8)reg_index_save_data);
    IO_OALStall;
	LEAVE_CRITICAL_SECTION;
}


/********************************************************************************
 *
 * Devcie control routines
 *
 ********************************************************************************/

void C_DM9000::DeviceEnableInterrupt(void)
{
#if 1
    PROCESSOR_LanEnableInterrupt(); //enable int
#endif
	
    // bits to turn on interrupt latch
    // <7> buffer chain enable
    // <5> link chang
    // <3> rx overflow count overflow
    // <2> rx overflow
    // <1> tx completed indication
    // <0> rx completed indication
    // DeviceWritePort(DM9_IMR,((1<<7)|(1<<1)|(1<<0)));
    DeviceWritePort(DM9_IMR, MAKE_MASK4(0, 1, 5, 7));
}

void C_DM9000::DeviceDisableInterrupt(void)
{
#if 1
    PROCESSOR_LanClearInterruptPending(); //disable int
#endif
    // <7> buffer chain enable
    DeviceWritePort(DM9_IMR, MAKE_MASK1(7));
}

void  C_DM9000::DeviceEnableReceive(void)
{
    // RX enable RXCR<0>
    if (m_szCurrentSettings[SID_OP_MODE] & MAKE_MASK1(0))
        return;
        
    m_szCurrentSettings[SID_OP_MODE] |= MAKE_MASK1(0);
    
    DeviceWritePort(DM9_RXCR, m_szCurrentSettings[SID_OP_MODE]);
}

void  C_DM9000::DeviceDisableReceive(void)
{
    // RX enable RXCR<0>
    if (!(m_szCurrentSettings[SID_OP_MODE] & MAKE_MASK1(0)))
        return;
        
    m_szCurrentSettings[SID_OP_MODE] &= ~MAKE_MASK1(0);
    
    DeviceWritePort(DM9_RXCR, m_szCurrentSettings[SID_OP_MODE]);
}

U32 C_DM9000::DeviceGetInterruptStatus(void)
{
    // mask for bits
    // <3> rx overflow count overflow
    // <2> rx overflowf
    // <1> tx completed indication
    // <0> rx completed indication
//  return DeviceReadPort(DM9_ISR) & (MAKE_MASK4(3,2,1,0) | MAKE_MASK1(5));
    return (DeviceReadPort(DM9_ISR) & MAKE_MASK6(0, 1, 2, 3, 4, 5));
}

U32 C_DM9000::DeviceSetInterruptStatus(
    U32 uValue)
{
// return DeviceWritePort(DM9_ISR,(uValue & 0x3e));
    return DeviceWritePort(DM9_ISR, uValue);
}

U32 C_DM9000::DeviceGetReceiveStatus(void)
{
    DEBUG_PRINT((TEXT("[DM9ISA_d]: nTxPendings=%d, %d \r\n"), m_nTxPendings, m_pUpper->m_TQueue.Size()));
    
    U32 cr;
    
    cr = DeviceReadPort(DM9_ROCR);
    
    return ((cr >> 7) & 1) << 31 | (cr & 0x7F);
}

void C_DM9000::DeviceStart(void)
{
    DeviceWritePort(DM9_RXCR, m_szCurrentSettings[SID_OP_MODE]);
    
    // enable interrupt
    DeviceEnableInterrupt();
    
#if 0
	PRINT_REG_DATA(1);
#endif
    
    DeviceSetTimer(500);
}

void C_DM9000::DeviceReset(void)
{
//edit by fae
    DEBUG_PRINT((TEXT("[DM9ISA_d]: Chip DeviceReset \r\n")));
    // announce shutdown
    m_bShutdown = 1;
    
    // wait until all activities are fi.
    m_mutexRxValidate.Lock();
    m_mutexTxValidate.Lock();
    
    C_Exception *pexp;
    
    TRY
    {
    
        EDeviceInitialize(++m_nResetCounts);
        
        DeviceOnSetupFilter(0);
        
        FI;
    }
    CATCH(pexp)
    {
        // nothing to do
        CLEAN(pexp);
    } // of catch
    
    // dequeue for all objects in waiting and standby queue
    PCQUEUE_GEN_HEADER pcurr;
    
    for (;(pcurr = m_TQWaiting.Dequeue());)
        DeviceSendCompleted(pcurr);
        
    for (;(pcurr = m_TQStandby.Dequeue());)
        DeviceSendCompleted(pcurr);
        
    m_mutexRxValidate.Release();
    
    m_mutexTxValidate.Release();
    
    m_bShutdown = 0;
}

void C_DM9000::EDeviceInitialize(
    int  nResetCounts)
{
    U32  val;
    
    DEBUG_PRINT((TEXT("[DM9ISA_d]: Chip EDeviceInitialize \r\n")));
    
    // reset member varialbes
    m_uLastAddressPort = (U32) - 1;
    
    if (!(DeviceReadPort(DM9_NSR) & 0x40))
    {
        RETAILMSG(TRUE, (TEXT("[DM9ISA]: EDeviceInitialize! phy reset now \r\n")));
        
        DeviceWritePort(DM9_GPR, 0x00);
        DevicePolling(0x28, 0xff, 0x46, 500, 5);
        
        DeviceWritePort(DM9_GPR, 0x01);
        DevicePolling(0x28, 0xff, 0x46, 100, 10);
        
        DeviceWritePhy(1, 0, 0x8000);
        NdisStallExecution(50);
        DeviceWritePhy(1, 20, 0x0200);
        if (0x1a == DeviceReadPort(DM9_CHIPREV))
        {
            DeviceWritePhy(1, 0x1b, 0xe100);
        }
        
        if (FLOW_CONTROL_ENABLE == m_szConfigures[CID_FC_ENABLE])
        {
            DeviceWritePhy(1, 4, 0x05e1);
        }
        
        DeviceWritePort(DM9_GPR, 0x00);
        DevicePolling(0x28, 0xff, 0x46, 500, 5);
        
        SetConnectionStatus(0);
    }
    
    // software reset the device
    DeviceWritePort(DM9_NCR, 0x03);
    
    NdisStallExecution(20);
    
    DeviceWritePort(DM9_NCR, 0x03);
    
    NdisStallExecution(20);
    
    DeviceWritePort(DM9_NCR, 0x00);
    
    //DeviceWritePort(DM9_NCR, 0x00);
    //DeviceWritePort(DM9_NCR, 0x02); //loopback
    
    // Enable memory chain
    DeviceWritePort(DM9_IMR, MAKE_MASK1(7));
    
    // clear TX status
    DeviceWritePort(DM9_NSR, 0x00);
    
    //function setting
    DeviceWritePort(DM9_LED, 0x80);
    if (0x1a == DeviceReadPort(DM9_CHIPREV))
    	DeviceWritePort(DM9_SPECIAL, 0x00);
    else
    	DeviceWritePort(DM9_SPECIAL, 0x08);
    	
    
    if (FLOW_CONTROL_ENABLE == m_szConfigures[CID_FC_ENABLE])
    {
        DeviceWritePort(DM9_FLOW, MAKE_MASK2(5, 3));
    }
    else
    {
        DeviceWritePort(DM9_FLOW, MAKE_MASK1(3));
    }
    
    m_nTxPendings = 0;
    rec_buff_count = 0;
    
    if (nResetCounts)
        return;
    
    m_nMaxTxPending = 1;
    
    // read the io orgnization
    // ISR<7:6> == x1, dword
    // ISR<7:6> == 0x, word
    // ISR<7:6> == 10, byte mode
    val = (DeviceReadPort(DM9_ISR) >> 6) & 0x03 ;
    
    switch (val)
    {
    
        case 0x00 :
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: EDeviceInitialize! chip is 16bit mode .. \r\n")));
            m_nIoMode = WORD_MODE;
            m_nIoMaxPad = 1;
            break;
            
        case 0x01 :
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: EDeviceInitialize! chip is 32bit mode .. \r\n")));
            m_nIoMode = DWORD_MODE;
            m_nIoMaxPad = 3;
            break;
            
        case 0x02 :
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: EDeviceInitialize! chip is 8bit mode .. \r\n")));
            m_nIoMode = BYTE_MODE;
            m_nIoMaxPad = 0;
            break;
            
        default :
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: EDeviceInitialize! nic error hald .. \r\n")));
            while (1);
            break;
    }
    
    /* set indication timer */
    DeviceInitializeTimer();
    
#ifdef SOFT_FIRST_RESET
    soft_first_reset_delay = SOFT_FIRST_RESET;
#endif
	
#ifdef LINK_OFF_RESET
	link_off_reset = 0;
#endif
}

void C_DM9000::DeviceResetPHYceiver(void)
{
    return;
}

void C_DM9000::DeviceHalt(void)
{
    RETAILMSG(TRUE, (TEXT("[DM9ISA]: DeviceHalt! \r\n")));
    
    DeviceReset();
    
    DeviceDisableInterrupt();
    DeviceDisableReceive();
    DeviceSetInterruptStatus(0xff);
    
    DeviceWritePort(DM9_GPR, 0x01);
    DevicePolling(0x28, 0xff, 0x46, 100, 10);
}


/********************************************************************************
 *
 * Devcie handler routines
 *
 ********************************************************************************/
#ifdef IMPL_DEVICE_ISR
void C_DM9000::DeviceIsr(
    U32  uState)
{
}
#endif

void C_DM9000::DeviceOnTimer(void)
{
    REG_INDEX_SAVE();
    
    U32 soft_interrupt_count = 0;
	
//    RETAILMSG(TRUE, (TEXT("[DM9ISA]: DeviceOnTimer check \r\n")));
	
#ifdef SOFT_FIRST_RESET
	if(soft_first_reset_delay)
	{
		soft_first_reset_delay--;
		DeviceSetTimer(500);
		if(0x00 == soft_first_reset_delay) DeviceIndication(AID_ERROR);
		return;
	}
#endif
	
#if 0
	PRINT_REG_DATA(0);
#endif
    
    U32 reg_isr = DeviceReadPort(DM9_ISR);
    U32 reg_imr = DeviceReadPort(DM9_IMR);

#ifdef INT_Edge_Detect
    if (reg_imr != 0x80)
	{ 
	    if ((reg_imr & reg_isr) & 0x3f)
	    {
	        RETAILMSG(TRUE, (TEXT("[DM9ISA]: INT_Edge_Detect %x %x   \r\n"), reg_imr, reg_isr));
	        soft_interrupt_count++;
	    }
	}
#endif
    
#ifdef SOFT_INT_CHECK
    if (reg_isr & 0x03f)
    {
//        RETAILMSG(TRUE, (TEXT("[DM9ISA]: SOFT_INT_CHECK %x \r\n"), reg_isr));
        soft_interrupt_count++;
    }
#endif

    if(rec_buff_count >= MAX_ONE_TIME_REV_PACKET)
    {
        reg_isr |= 0x01;
        soft_interrupt_count++;
    }

    if(0 != soft_interrupt_count)
    {
        DeviceDisableInterrupt();

        DeviceWritePort(DM9_ISR, reg_isr);
        DeviceWritePort(DM9_IMR, MAKE_MASK1(7));
        DeviceInterruptEventHandler(reg_isr);
        DeviceWritePort(DM9_IMR, reg_imr);
        
        DeviceEnableInterrupt();
    }

#ifdef INT_Edge_Detect
    if(rec_buff_count >= MAX_ONE_TIME_REV_PACKET)
    {
        DeviceSetTimer(SOFT_INT_CHECK_Time);
    }
#else
    DeviceSetTimer(SOFT_INT_CHECK_Time);
#endif

    REG_INDEX_RESTORE();
}

int C_DM9000::DeviceOnSetupFilter(
    U32  uFilter)
{
    int n;
    U8 sz[8];
    U32 hashval;
    U32 newmode;
    
    // save old op mode
    newmode = m_szCurrentSettings[SID_OP_MODE];
    // clear filter related bits,
    // pass all multicast<3> and promiscuous<1>
    newmode &= ~MAKE_MASK2(3, 1);
    
    // zero input means one reset request
    
    if (!(m_szCurrentSettings[SID_GEN_CURRENT_PACKET_FILTER] = uFilter))
    {
    
        /* 1. set unicast */
        // retrive node address
        DeviceMacAddress(&sz[0]);
        // set node address
        
        for (n = 0;n < ETH_ADDRESS_LENGTH;n++)
            DeviceWritePort(DM9_PAR0 + n, (U32)sz[n]);
            
        /* 2. clear multicast list and count */
        m_nMulticasts = 0;
        
        memset((void*)&m_szMulticastList, 0, sizeof(m_szMulticastList));
        
        /* 3. clear hash table */
        // clear hash table
        memset((void*)(&sz[0]), 0, sizeof(sz));
        
        for (n = 0;n < sizeof(sz);n++)
            DeviceWritePort(DM9_MAR0 + n, (U32)sz[n]);
            
        return uFilter;
    }
    
    // if promiscuous mode<1> is requested,
    // just set this bit and return
    if ( (uFilter & NDIS_PACKET_TYPE_PROMISCUOUS) )
    {
        // add promiscuous<1>
        newmode |= MAKE_MASK1(1);
        DeviceWritePort(DM9_RXCR, m_szCurrentSettings[SID_OP_MODE] = newmode);
        return uFilter;
    }
    
    // if pass all multicast<3> is requested,
    if (uFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
        newmode |= MAKE_MASK1(3);
        
    // prepare new hash table
    memset((void*)(&sz[0]), 0, sizeof(sz));
    
    // if broadcast, its hash value is known as 63.
    if (uFilter & NDIS_PACKET_TYPE_BROADCAST)
        sz[7] |= 0x80;
        
    if (uFilter & NDIS_PACKET_TYPE_MULTICAST)
        for (n = 0;n < m_nMulticasts;n++)
        {
            hashval = DeviceCalculateCRC32(&m_szMulticastList[n][0], ETH_ADDRESS_LENGTH, FALSE) & 0x3f;
            sz[hashval/8] |= 1 << (hashval % 8);
        } // of calculate hash table
        
    // submit the new hash table
    for (n = 0;n < sizeof(sz);n++)
        DeviceWritePort(DM9_MAR0 + n, (U32)sz[n]);
        
//edit by fae
    for (n = 0x10; n < 0x1e; n++)
        DEBUG_PRINT(( TEXT("[DM9ISA_d]: Chip reg %x is %x \r\n"), n , DeviceReadPort(n)));
        
    DeviceWritePort(DM9_RXCR, m_szCurrentSettings[SID_OP_MODE] = newmode);
    
    return uFilter;
}


void C_DM9000::DeviceInterruptEventHandler(
    U32 uValue)
{
    // check RX activities
    if (uValue & 0x01)
        Dm9LookupRxBuffers();
        
    uValue |= DeviceReadPort(DM9_ISR);
    
    // return if not TX latch
    if (uValue & 0x02)
    {
        if (m_nTxPendings)
        {
            TX_SEND_CHECK();
        }
    }
    
    // check link chang  activities
    if (uValue & 0x20)
    {
        DeviceCheckLink();
        DEBUG_PRINT((TEXT("[DM9ISA_d]: Chip DeviceInterruptEventHandler link chang  %x \r\n"), uValue));
    }
    
#ifdef INT_Edge_Detect
    DeviceSetTimer(INT_Edge_Detect_Time);
#endif
}

U32 C_DM9000::DeviceHardwareStatus(void)
{
    return m_nTxPendings ? 0 : NIC_HW_TX_IDLE;
}

int C_DM9000::DeviceSend(
    PCQUEUE_GEN_HEADER pObject)
{
    PCQUEUE_GEN_HEADER pcurr;
    U32 save_mwr, calc_mwr, check_mwr;
    
    if (pObject)
        m_TQStandby.Enqueue(pObject);

    if(m_szCurrentSettings[SID_MEDIA_CONNECTION_STATUS] != NdisMediaStateConnected)
    {
         DEBUG_PRINT((TEXT("[DM9ISA_d]: DeviceSend not link \r\n")));
        return 0;
    }
        
    while (m_nTxPendings)
    {
        TX_SEND_CHECK();
        
        if (m_nTxPendings)
            NdisStallExecution(5);
    }
    
    /*
     if(m_nTxPendings)
     {
    //  DEBUG_PRINT((TEXT("[DM9ISA]:Chip DeviceSend err %x \r\n"),m_nTxPendings));
      RETAILMSG(TRUE,(TEXT("[DM9ISA]:Chip DeviceSend err %x \r\n"),m_nTxPendings));
      m_TQWaiting.Enqueue(pcurr=m_TQStandby.Dequeue());
      DeviceSendCompleted(m_TQWaiting.Dequeue());
    //  DeviceIndication(NIC_IND_TX_IDLE);
      return 0;
     }
    */
    
    /* increment counter */
    m_nTxPendings++;
    
    /* get first pkt in queue */
    m_TQWaiting.Enqueue(pcurr = m_TQStandby.Dequeue());
    
    save_mwr = MAKE_WORD(DeviceReadPort(DM9_MDWAH), DeviceReadPort(DM9_MDWAL));
    calc_mwr = save_mwr + pcurr->nLength;
    if(pcurr->nLength & m_nIoMaxPad)
    {
    	calc_mwr += m_nIoMaxPad;
    	calc_mwr &= ~(m_nIoMaxPad);
    }
    if (calc_mwr > 0x0bff) calc_mwr -= 0x0c00;
    
    DeviceWritePort(DM9_TXLENH, HIGH_BYTE(pcurr->nLength));
    DeviceWritePort(DM9_TXLENL, LOW_BYTE(pcurr->nLength));    
        
    /* fill data */
    DeviceWriteString((PU8)CQueueGetUserPointer(pcurr), pcurr->nLength);
    
    // TXCR<0>, issue TX request
    DeviceWritePort(DM9_TXCR, MAKE_MASK1(0));
    
    check_mwr = MAKE_WORD(DeviceReadPort(DM9_MDWAH), DeviceReadPort(DM9_MDWAL));
    if (check_mwr != calc_mwr)
    {
        RETAILMSG(TRUE, (TEXT("[DM9ISA]: tx point error %x , %x , %x , %x \r\n"),
                         save_mwr, pcurr->nLength, check_mwr, calc_mwr));
        DeviceWritePort(DM9_MDWAL, LOW_BYTE(calc_mwr));
        DeviceWritePort(DM9_MDWAH, HIGH_BYTE(calc_mwr));
    }
    
    return 0;
}

int C_DM9000::Dm9LookupRxBuffers(void)
{
//edit by fae
//DEBUG_PRINT((TEXT("[DM9ISA]: Chip Dm9LookupRxBuffers \r\n")));

    if (!m_mutexRxValidate.TryLock()) 
        return 0;
    
    int  errors = 0;
    
    U32 desc;
    
    PDM9_RX_DESCRIPTOR pdesc;
    
    U32 save_mrr, calc_mrr, check_mrr;
    
    U8 szbuffer[DRIVER_BUFFER_SIZE];
    
    pdesc = (PDM9_RX_DESCRIPTOR) & desc;
    
    rec_buff_count = 0;
    
    while(rec_buff_count < MAX_ONE_TIME_REV_PACKET)
    {
        CHECK_SHUTDOWN();
        
        // probe first byte
        desc = DeviceReadDataWithoutIncrement();
        
        // check if packet available, 01h means available, 00h means no data
        
        if (pdesc->bState != 0x01)
        {
            if (pdesc->bState != 0x00)
            {
                RETAILMSG(TRUE, (TEXT("[DM9ISA]: pdesc->bState error %x \r\n"), pdesc->bState));
                DeviceIndication(AID_STATUS_ERROR_PACKET);
            }
            else
            {
                DeviceWritePort(DM9_ISR, 0x01);
            }
            
            break;
        }
        
        save_mrr = MAKE_WORD(DeviceReadPort(DM9_MDRAH), DeviceReadPort(DM9_MDRAL));

        // get the data descriptor again
        desc = DeviceReadData();
        
        // read out the data to buffer
        // Performance issue: maybe we may discard the data
        // just add the rx address.
        // if the length is greater than buffer size, ...
        if (pdesc->nLength > DRIVER_BUFFER_SIZE)
        {
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: pdesc->nLength over %x error %x \r\n"), DRIVER_BUFFER_SIZE, pdesc->nLength));
            DeviceIndication(AID_LARGE_INCOME_PACKET);
            break;
        }

        calc_mrr = save_mrr + pdesc->nLength + 4;
	    if(pdesc->nLength & m_nIoMaxPad)
	    {
	    	calc_mrr += m_nIoMaxPad;
	    	calc_mrr &= ~(m_nIoMaxPad);
	    }        
        if (calc_mrr > 0x3fff) calc_mrr -= 0x3400;
            
        // check status, as specified in DM9_RXSR,
        // the following bits are error
        // <3> PLE
        // <2> AE
        // <1> CE
        // <0> FOE
        if (pdesc->bStatus & MAKE_MASK4(3, 2, 1, 0))
        {
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: pdesc->bStatus %02x!!! \r\n"), pdesc->bStatus));
            DeviceWritePort(DM9_MDRAL, LOW_BYTE(calc_mrr));
            DeviceWritePort(DM9_MDRAH, HIGH_BYTE(calc_mrr));
            errors++;
            continue;
        } // of error happens
        
        DeviceReadString((PU8)&szbuffer, pdesc->nLength);
        
        check_mrr = MAKE_WORD(DeviceReadPort(DM9_MDRAH), DeviceReadPort(DM9_MDRAL));
        if (check_mrr != calc_mrr)
        {
            RETAILMSG(TRUE, (TEXT("[DM9ISA]: rx point error %x , %x , %x , %x \r\n"),
                             save_mrr, pdesc->nLength, check_mrr, calc_mrr));
            DeviceWritePort(DM9_MDRAL, LOW_BYTE(calc_mrr));
            DeviceWritePort(DM9_MDRAH, HIGH_BYTE(calc_mrr));
        }
        
        rec_buff_count++;
        
        DeviceReceiveIndication(
            0,
            (PVOID)&szbuffer,
            pdesc->nLength);
            
//edit by fae
        DEBUG_PRINT((TEXT("[DM9ISA_d]: Chip Dm9LookupRxBuffers len is %d , now packet is %d \r\n"), pdesc->nLength, rec_buff_count));
        
    } // of forever read loop
    
    REPORT(TID_GEN_RCV_OK, rec_buff_count);
    
    REPORT(TID_GEN_RCV_ERROR, errors);
    
    m_mutexRxValidate.Release();
    
    return rec_buff_count;
}

U32 C_DM9000::DeviceCheckLink(void) //  Device-Check-For-Link() ;'BMSR'
{
    U8 re_try = 5;
    
    while (--re_try)
    {
        if (DeviceReadPort(DM9_NSR) & MAKE_MASK1(6))
            break;
    }
    
    if (re_try)
    {
    	if (m_szCurrentSettings[SID_MEDIA_CONNECTION_STATUS] != NdisMediaStateConnected) DeviceIndication(AID_ERROR);
        SetConnectionStatus(1);
#ifdef LINK_OFF_RESET
        link_off_reset = 0;
#endif
    }
    else
    {
        SetConnectionStatus(0);
        
        if (0x80 != DeviceReadPort(DM9_IMR))
        {
            DeviceWritePhy(1, 0, 0x8000);
            NdisStallExecution(50);
            DeviceWritePhy(1, 20, 0x0200);
            if (0x1a == DeviceReadPort(DM9_CHIPREV))
            {
                DeviceWritePhy(1, 0x1b, 0xe100);
            }
            
            if (FLOW_CONTROL_ENABLE == m_szConfigures[CID_FC_ENABLE])
            {
                DeviceWritePhy(1, 4, 0x05e1);
            }
            
            DeviceWritePhy(1, 0, 0x1200);
#ifdef LINK_OFF_RESET
	        link_off_reset++;
	        if(link_off_reset > LINK_OFF_RESET)
	        {
	        	RETAILMSG(TRUE, (TEXT("[DM9ISA]: DeviceCheckLink device long time not link , reset device !! \r\n")));
	        	link_off_reset = 0;
	        	DeviceIndication(AID_ERROR);
	        }
#endif	    
        }
    }
 
    return(re_try);
}


/*******************************************************************
 *
 * Hooked function
 *
 *******************************************************************/

/*******************************************************************
 *
 * Device Entry
 *
 *******************************************************************/

extern "C" NIC_DEVICE_OBJECT *DeviceEntry(
        NIC_DRIVER_OBJECT *pDriverObject,
        PVOID pVoid)
{
    RETAILMSG(TRUE, (TEXT("DM9000 NIC_DEVICE_OBJECT *DeviceEntry!!***\r\n")));
    return new C_DM9000(pDriverObject, pVoid);
}
