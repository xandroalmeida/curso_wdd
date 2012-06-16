#pragma once

#define IOCTL_COPY_BUFFERED CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                     0x800,                 \
                                     METHOD_BUFFERED,       \
                                     FILE_ANY_ACCESS)

#define IOCTL_COPY_DIRECT   CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                     0x801,                 \
                                     METHOD_OUT_DIRECT,     \
                                     FILE_ANY_ACCESS)

#define IOCTL_COPY_NEITHER  CTL_CODE(FILE_DEVICE_UNKNOWN,   \
                                     0x802,                 \
                                     METHOD_NEITHER,        \
                                     FILE_ANY_ACCESS)
