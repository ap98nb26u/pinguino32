// cdc module for Pinguino32X
// Jean-Pierre Mandon 2010
// Based on the Microchip USB stack

#ifndef __USBCDC
#define __USBCDC

#include <system.c>
#include <interrupt.c>
#include <delay.c>
#include <stdio.c>
#include <stdarg.h>

typedef enum
{
	EVENT_NONE = 0,
	EVENT_DEVICE_STACK_BASE = 1,
	EVENT_HOST_STACK_BASE = 100,
	EVENT_HUB_ATTACH,           
	EVENT_STALL,                  
	EVENT_VBUS_SES_REQUEST,     
	EVENT_VBUS_OVERCURRENT,     
	EVENT_VBUS_REQUEST_POWER,   
	EVENT_VBUS_RELEASE_POWER,   
	EVENT_VBUS_POWER_AVAILABLE, 
	EVENT_UNSUPPORTED_DEVICE,   
	EVENT_CANNOT_ENUMERATE,     
	EVENT_CLIENT_INIT_ERROR,    
	EVENT_OUT_OF_MEMORY,        
	EVENT_UNSPECIFIED_ERROR,     
	EVENT_DETACH, 
	EVENT_TRANSFER,
	EVENT_SOF,                  
	EVENT_RESUME,
	EVENT_SUSPEND,
	EVENT_RESET,  
	EVENT_DATA_ISOC_READ,
	EVENT_DATA_ISOC_WRITE,
	EVENT_GENERIC_BASE  = 400,
	EVENT_MSD_BASE      = 500,
	EVENT_HID_BASE      = 600,
	EVENT_PRINTER_BASE  = 700,
	EVENT_CDC_BASE      = 800,
	EVENT_CHARGER_BASE  = 900,
	EVENT_AUDIO_BASE    = 1000,
	EVENT_USER_BASE     = 10000,
	EVENT_BUS_ERROR     = 2^31-1
} USB_EVENT;

typedef enum
{
	EVENT_CONFIGURED = EVENT_DEVICE_STACK_BASE,
	EVENT_SET_DESCRIPTOR,
	EVENT_EP0_REQUEST,
	EVENT_ATTACH,
	EVENT_TRANSFER_TERMINATED
} USB_DEVICE_STACK_EVENTS;

// this function is here to insure compatibility between USB microchip
// stack and Pinguino interrupt code
 
void INTEnableSystemMultiVectoredInt()
{
	IntConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
}

// this function is here to insure compatibility between USB microchip
// stack and Pinguino interrupt code

void INTEnableInterrupts()
{
	IntEnable(INT_USB, INT_ENABLED);
}

// this is the Set Line coding CallBack function
 
void mySetLineCodingHandler()
{
}

// this is the USB Event CallBack function

void USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event_usb)
{
	switch(event_usb)
	{
		case EVENT_EP0_REQUEST:
			USBCheckCDCRequest();
			break;
		case EVENT_CONFIGURED: 
			CDCInitEP();
			break;
	}
}

// this function is called by the main32.c file
// CDC.init
void CDC_init()
{
	USBDeviceInit();		// Initializes USB module SFRs and firmware
	USBDeviceAttach();
	Delayms(2000);
}

// CDC.puts
void CDCputs(u8 *buffer, u8 length)
{
	putUSBUSART(buffer,length);
	CDCTxService();
}
	
// CDC.read
u8 CDCgets(u8 *buffer, u8 length)
{
	u8 receivedchar;
	
	receivedchar=getsUSBUSART(buffer, length);
	return receivedchar;
}

// added by regis blanchot 29/05/2011

// CDC.write
void CDCwrite(char c)
{
	putUSBUSART(c, 1);
	CDCTxService();
}

// CDC.printf
void CDCprintf(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	pprintf(CDCwrite, fmt, args);
	va_end(args);
}

//CDC.println
void CDCprintln(char *fmt,...)
{
	CDCprintf(fmt);
	CDCprintf("\n\r");
}

// CDC.print
void CDCprint(char *fmt,...)
{
	va_list ap;
	unsigned char *s;							// must be unsigned char !!!

	va_start(ap,fmt);
	s = va_start(ap,fmt);

	switch (*s)
	{
		case DEC:
			CDCprintf("%d",fmt);
			break;
		case HEX:
			CDCprintf("%x",fmt);
			break;
		case BYTE:
			CDCprintf("%d",fmt);
			break;
		case OCT:
			CDCprintf("%o",fmt);
			break;
		case BIN:
			CDCprintf("%b",fmt);
			break;           
		default:
			CDCprintf(fmt);
			break;
	}

	va_end(ap);
}

// CDC.getKey
char * CDCgetkey()
{
	char c;
	static char buffer[1];

	getsUSBUSART(buffer, 1);
	return (buffer);
}

// CDC.getString
char * CDCgetstring()
{
	u8 i = 0;
	char c;
	static char buffer[80];
	
	do {
		c = CDCgetkey();
		CDCprintf("%c", c);
		buffer[i++] = c;
	} while (c != '\r');
	buffer[i] = '\0';
	return (buffer);
}

#endif	/* __USBCDC */