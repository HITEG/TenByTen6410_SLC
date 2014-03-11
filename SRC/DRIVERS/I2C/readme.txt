//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation
//
// Module:	I2C Bus Driver (I2C.DLL)
//
//------------------------------------------------------------------------------

o) Supports Master Tx/Rx Modes only.

o) Supports Multi-byte Tx/Rx (slave must support auto increment for multi-byte).

o) Uses UTLDRV for mapping in register space. I2C.DLL must load after UTLDRV.DLL.
   See %CATFISH%\files\platform.reg for load order setup.

o) Exports I2C_FASTCALL dispatch table for direct Driver-To-Driver calling in device.exe 
   process space. Callers outside of device.exe will fail and MUST use the IOCTLs provided.
   To use these routines an I2C client driver must call CreateFile on the I2C bus driver,
   and call IOCTL_I2C_GET_FASTCALL to get the entrypoints. CreateFile calls the 
   bus driver's Xxx_Open routine. The client driver will call CloseHandle when 
   it is completely done with the bus driver, which calls the bus driver's Xxx_Close.

o) PCF50606.DLL is the main I2C client on Catfish. Load order is important.
   The I2C bus driver must load before any I2C client driver.
   See %CATFISH%\files\platform.reg for load order setup.

o) The battery driver sits on top of the PCF50606 driver, using it's services 
   for A/D and battery charger, across the I2C.

