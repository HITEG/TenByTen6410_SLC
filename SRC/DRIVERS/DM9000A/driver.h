/********************************************************************************
 *
 * $Id: driver.h,v 1.1.1.1 2007/04/16 03:45:52 bill Exp $
 *
 * File: Driver.h
 *
 * Optimizations:
 * When build in RETAIL mode, the compiler options is set to /Oxs,
 *  which means maximum opts.(x) and favor for code space(s).
 *  Option Ox stands for Ogityb1 and Gs, for more detail information,
 *  type CL/? at command your DOS command prompt.
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/
#ifndef __WINCE_DRIVER_H__
#define __WINCE_DRIVER_H__

#include "types.h"
#include "common.h"

#define PRJ_NDIS_MAJOR_VERSION          4
#define PRJ_NDIS_MINOR_VERSION          0

#ifndef VENDOR_DESC
#define VENDOR_DESC                     "Davicom Semiconductor, Inc"
#endif


/********************************************************************************
 *
 * Driver behaviors
 *
 ********************************************************************************/
//#define IMPL_SEND_INDICATION
#define IMPL_STATISTICS
#define IMPL_RESET
//#define IMPL_DEVICE_ISR

/******************************************************************************************
 *
 * Ethernet definitions
 *
 *******************************************************************************************/
#define ETH_MAX_FRAME_SIZE              1514
#define ETH_HEADER_SIZE                 14
#define ETH_ADDRESS_LENGTH              6
#define ETH_CRC_SIZE                    4
#define MAX_MULTICAST_LIST              64
#define DRIVER_BUFFER_SIZE              0x5F0

/******************************************************************************************
 *
 * NIC_DRIVER_OBJECT definition
 *
 *******************************************************************************************/
typedef struct _CONFIG_PARAMETER
{
    U32 uId;
    U32 uDefValue;
    NDIS_STRING  szName;
} CONFIG_PARAMETER, *PCONFIG_PARAMETER;

typedef enum
{
    CID_CONNECTION_TYPE = 0,
    CID_SLOT_NUMBER,
    CID_BUFFER_PHYSICAL_ADDRESS,
    CID_TXBUFFER_NUMBER,
    CID_RXBUFFER_NUMBER,
    CID_ADAPTER_NUMBER,
    CID_IO_RANGE,
    CID_IRQ_LEVEL,  // to raise irql level
    CID_IRQ_GEN_TYPE, // level sensitive(pci) or latched(isa)
    CID_IRQ_SHARED,  // shared or not
    CID_INTERFACE_TYPE, // isa or pci device
    CID_BUS_MASTER,  // is a bus master or not
    CID_INTERMEDIATE, // is a intermediate miniport
    CID_CHECK_FOR_HANG_PERIOD, // in seconds
    CID_CHIP_STEPPING,
    CID_NEED_IO_SPACE,
    CID_NEED_INTERRUPT,
    
    CID_IO_BASE_ADDRESS,
    CID_IRQ_NUMBER,  // irq pin or line number
    
    CID_INT_ACTIVE,
    CID_FC_ENABLE,
    
    CID_MAC_LOAD_MODE,
    
    CID_MAC_ADDR_0,
    CID_MAC_ADDR_1,
    CID_MAC_ADDR_2,
    CID_MAC_ADDR_3,
    CID_MAC_ADDR_4,
    CID_MAC_ADDR_5,
    
    CID_SIZE
} DEVICE_CID_TYPE;

typedef enum
{
    SID_HW_STATUS,
    SID_OP_MODE,
    SID_INT_MASK,
    SID_INT_GEN_MASK,
    SID_PORT_BASE_ADDRESS,
    SID_PORT_REG_INDEX_ADDRESS,
    SID_PORT_REG_DATA_ADDRESS,
    SID_PHY_NUMBER,
    SID_MEDIA_SUPPORTED,
    SID_MEDIA_IN_USE,
    SID_MEDIA_CONNECTION_STATUS,
    
    SID_MAXIMUM_LOOKAHEAD,
    SID_MAXIMUM_FRAME_SIZE,
    SID_MAXIMUM_TOTAL_SIZE,
    SID_BUFFER_SIZE,
    SID_MAXIMUM_SEND_PACKETS,
    SID_LINK_SPEED,
    
    SID_GEN_MAC_OPTIONS,
    SID_802_3_PERMANENT_ADDRESS,
    SID_802_3_CURRENT_ADDRESS,
    SID_802_3_MAXIMUM_LIST_SIZE,
    SID_802_3_MULTICAST_LIST,
    SID_GEN_CURRENT_PACKET_FILTER,
    SID_GEN_TRANSMIT_BUFFER_SPACE,
    SID_GEN_RECEIVE_BUFFER_SPACE,
    SID_GEN_TRANSMIT_BLOCK_SIZE,
    SID_GEN_RECEIVE_BLOCK_SIZE,
    SID_GEN_VENDOR_ID,
    SID_GEN_VENDOR_DESCRIPTION,
    SID_GEN_CURRENT_LOOKAHEAD,
    SID_GEN_DRIVER_VERSION,
    SID_GEN_VENDOR_DRIVER_VERSION,
    SID_GEN_PROTOCOL_OPTIONS,
    
    SID_SIZE
} DEVICE_SID_TYPE;

typedef enum
{
    TID_GEN_XMIT_OK,
    TID_GEN_RCV_OK,
    TID_GEN_XMIT_ERROR,
    TID_GEN_RCV_ERROR,
    TID_GEN_RCV_NO_BUFFER,
    TID_GEN_RCV_CRC_ERROR,
    
    TID_802_3_RCV_ERROR_ALIGNMENT,
    TID_802_3_RCV_OVERRUN,
    TID_802_3_XMIT_ONE_COLLISION,
    TID_802_3_XMIT_MORE_COLLISIONS,
    TID_802_3_XMIT_DEFERRED,
    TID_802_3_XMIT_MAX_COLLISIONS,
    TID_802_3_XMIT_UNDERRUN,
    TID_802_3_XMIT_HEARTBEAT_FAILURE,
    TID_802_3_XMIT_TIMES_CRS_LOST,
    TID_802_3_XMIT_LATE_COLLISIONS,
    
    TID_NIC_RXPS,
    TID_NIC_RXCI,
    
    TID_RX_FIFO_OVERFLOW,
    TID_RX_FIFO_OVERFLOW_OVERFLOW,
    TID_SIZE
} DEVICE_TID_TYPE;

typedef enum
{
    EID_MAC_ADDRESS,
    EID_VENDOR_ID,
    EID_PRODUCT_ID,
    EID_SIZE
} DEVICE_EID_TYPE;

typedef enum
{
    NIC_IND_TX_IDLE = 0,
    AID_ERROR = 0x8000,
    AID_STATUS_ERROR_PACKET,
    AID_LARGE_INCOME_PACKET,
} NIC_IND_TYPE;

typedef struct _DATA_BLOCK
{
    CQUEUE_GEN_HEADER Header;
    U8 Buffer[DRIVER_BUFFER_SIZE];
} DATA_BLOCK, *PDATA_BLOCK;

/******************************************************************************************
 *
 * NIC_DRIVER_OBJECT definition
 *
 *******************************************************************************************/

class NIC_DEVICE_OBJECT;

class NIC_DRIVER_OBJECT
{
    public:
        NIC_DRIVER_OBJECT(NDIS_HANDLE pHandle, NDIS_HANDLE pWrapper)
        {
            m_NdisHandle = pHandle;
            m_NdisWrapper = pWrapper;
            m_bSystemHang = 0;
            m_bOutofResources = 0;
            m_pLower = NULL;
        };
        
        ~NIC_DRIVER_OBJECT() {};
        
    public:
        // routines that may throw exceptions
        virtual void EDriverInitialize(
            OUT PNDIS_STATUS OpenErrorStatus,
            OUT PUINT SelectedMediaIndex,
            IN PNDIS_MEDIUM MediaArray,
            IN UINT MediaArraySize);
            
        // routines that return void or error code
        virtual void DriverStart(void);
        virtual void DriverIndication(U32);
        virtual void DriverSendCompleted(PCQUEUE_GEN_HEADER);
        
        virtual void DriverReceiveIndication(int, PVOID, int);
        
        NDIS_HANDLE GetNdisHandle()
        {
            return m_NdisHandle;
        };
        
        NDIS_HANDLE GetNdisWrapper()
        {
            return m_NdisWrapper;
        };
        
        U32 GetRecentInterruptStatus()
        {
            return m_uRecentInterruptStatus;
        };
        
        PVOID DriverBindAddress(U32, U32);
        
        // miniport driver callback
        virtual void DriverIsr(
            OUT PBOOLEAN InterruptRecognized,
            OUT PBOOLEAN QueueInterrupt);
            
        virtual VOID DriverInterruptHandler(void);
        
        virtual NDIS_STATUS DriverQueryInformation(
            IN NDIS_OID  Oid,
            IN PVOID InfoBuffer,
            IN ULONG InfoBufferLength,
            OUT PULONG BytesWritten,
            OUT PULONG BytesNeeded);
            
        virtual NDIS_STATUS DriverSetInformation(
            IN NDIS_OID  Oid,
            IN PVOID InfoBuffer,
            IN ULONG InfoBufferLength,
            OUT PULONG BytesRead,
            OUT PULONG BytesNeeded);
            
        virtual VOID DriverEnableInterrupt(void);
        
        virtual VOID DriverDisableInterrupt(void);
        
        virtual BOOL DriverCheckForHang(void);
        
        virtual VOID DriverHalt(void);
        
        virtual NDIS_STATUS DriverReset(OUT PBOOLEAN);
        
        virtual NDIS_STATUS DriverSend(IN PNDIS_PACKET, IN UINT);
        
        virtual int DriverIsOutOfResource(void)
        {
            return m_bOutofResources;
        };
        
    public:
        int m_bSystemHang;
        int m_bOutofResources;
        
        NIC_DEVICE_OBJECT *m_pLower;
        
        U32 m_uRecentInterruptStatus;
        NDIS_HANDLE m_NdisHandle;
        NDIS_HANDLE m_NdisWrapper;
        
        CQueue m_TQueue;       
};

/*********************************************************************************
 *
 * External functions
 *
 *********************************************************************************/
extern "C"
    BOOL VirtualCopy (
        LPVOID lpvDest,
        LPVOID lpvSrc,
        DWORD cbSize,
        DWORD fdwProtect );

extern "C" NIC_DEVICE_OBJECT *DeviceEntry(
        NIC_DRIVER_OBJECT *pDriverObject,
        PVOID pVoid);

#endif // of __PROJECT_H__
