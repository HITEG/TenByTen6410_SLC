#if !defined(EXTPOWERCTL_DRV_H)
#define EXTPOWERCTL_DRV_H

typedef enum
{
	LCD_ON_SET,
    USB_ON_SET,
    CAMERA_ON_SET,
    DAC0_ON_SET,
    DAC1_ON_SET,
	ETH_ON_SET,
	WIFI_ON_SET,
	LCD_OFF_SET,
    USB_OFF_SET,
    CAMERA_OFF_SET,
    DAC0_OFF_SET,
    DAC1_OFF_SET,
	ETH_OFF_SET,
	WIFI_OFF_SET,
	ALL_ON_SET,
	ALL_OFF_SET,
	AWAKE_SET,
	SLEEP_SET,
    EPCTL_MAX
} EPCTL_SET;

typedef enum
{
    EPCTL_SUCCESS,  
    EPCTL_ERROR_XXX
} EPCTL_ERROR;


#ifdef DEBUG
#ifdef ZONE_ERROR
	#undef ZONE_ERROR
#endif
#define ZONE_ERROR            DEBUGZONE(0)
#ifdef ZONE_WARNING
#undef ZONE_WARNING
#endif
#define ZONE_WARNING            DEBUGZONE(1)
#define ZONE_FUNCTION           DEBUGZONE(2)
#ifdef ZONE_INIT
#undef ZONE_INIT
#endif
#define ZONE_INIT           DEBUGZONE(3)
#define ZONE_INFO           DEBUGZONE(4)
#define ZONE_IST          DEBUGZONE(5)

#define ZONE_TRACE          DEBUGZONE(15)

//
// these should be removed in the code if you can 'g' past these successfully
//
#define TEST_TRAP { \
   NKDbgPrintfW( TEXT("%s: Code Coverage Trap in: WDG, Line: %d\n"), TEXT(__FILE__), __LINE__); \
   DebugBreak();  \
}
#else
#define TEST_TRAP
#endif


#endif