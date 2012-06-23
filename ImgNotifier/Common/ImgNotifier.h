#pragma once

#define IOCTL_IMG_START_NOTIFYING   CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                             0x800,                 \
                                             METHOD_BUFFERED,       \
                                             FILE_ANY_ACCESS)

typedef struct _IMG_START_NOTIFYING
{
    HANDLE  hEvent;

} IMG_START_NOTIFYING, *PIMG_START_NOTIFYING;


#define IOCTL_IMG_GET_IMAGE_DETAIL  CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                             0x801,                 \
                                             METHOD_BUFFERED,       \
                                             FILE_ANY_ACCESS)


#define IMG_MAX_IMAGE_NAME  260


typedef struct _IMG_IMAGE_DETAIL
{
    CHAR    ImageName[IMG_MAX_IMAGE_NAME];

} IMG_IMAGE_DETAIL, *PIMG_IMAGE_DETAIL;


#define IOCTL_IMG_STOP_NOTIFYING    CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                             0x802,                 \
                                             METHOD_BUFFERED,       \
                                             FILE_ANY_ACCESS)
