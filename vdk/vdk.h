#ifndef _VDK_H_
#define _VDK_H_

#define declare_klass(klass)    extern const void *klass

enum
{
    VDK_FRAME_SINGLE = 0x01,
    VDK_FRAME_DOUBLE,
    VDK_FRAME_SOLID,
};

#endif
