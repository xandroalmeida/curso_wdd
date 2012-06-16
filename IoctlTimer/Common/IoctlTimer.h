#pragma once

#define IOCTL_TIMER_SET_TIMER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _TIMER_SET_TIMER
{
    HANDLE  hEvent;
    ULONG   ulSeconds;

} TIMER_SET_TIMER, *PTIMER_SET_TIMER;
