#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <ntddk.h>

typedef struct _String_Entry
{
	UNICODE_STRING usString;
	LIST_ENTRY entry;
} String_Entry, *PString_Entry;

typedef struct _Device_Extension
{
	LIST_ENTRY listHead;
	KSPIN_LOCK slock;
	LONG Users;
} Device_Extension, *PDevice_Extension;

#endif