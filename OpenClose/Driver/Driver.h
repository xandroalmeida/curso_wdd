#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <ntddk.h>

typedef struct _String_List
{
	UNICODE_STRING usString;
	LIST_ENTRY entry;
} String_List, *PString_List;

#endif