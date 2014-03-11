#ifndef __LED_H__
#define __LED_H__



//
// I2C Bus Driver IOCTLS
//
#define FILE_DEVICE_LED     FILE_DEVICE_CONTROLLER


#define IOCTL_LED_ON \
    CTL_CODE(FILE_DEVICE_LED, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define IOCTL_LED_OFF \
    CTL_CODE(FILE_DEVICE_LED, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)



#ifdef DEBUG
#define ZONE_ERR            DEBUGZONE(0)
#define ZONE_WRN            DEBUGZONE(1)
#define ZONE_INIT           DEBUGZONE(2)
#define ZONE_OPEN           DEBUGZONE(3)
#define ZONE_READ           DEBUGZONE(4)
#define ZONE_WRITE          DEBUGZONE(5)
#define ZONE_IOCTL          DEBUGZONE(6)
//...
#define ZONE_TRACE          DEBUGZONE(15)

//
// these should be removed in the code if you can 'g' past these successfully
//
#define TEST_TRAP { \
   NKDbgPrintfW( TEXT("%s: Code Coverage Trap in: Led, Line: %d\n"), TEXT(__FILE__), __LINE__); \
   DebugBreak();  \
}
#else
#define TEST_TRAP
#endif



#endif