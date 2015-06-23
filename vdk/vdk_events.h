#ifndef _VDK_EVENTS_H_
#define _VDK_EVENTS_H_

#include <inttypes.h>

#include <vdk_object.h>

/*
    The high-order bits on the signal and events are reserved.  The 
    low-order 16-bits are for general use.  As a result, the system limit
    is 65k unique signal definitions and 65k event definitions.
*/
#define VDK_EVENT_MASK(event)           (event & 0x00FFFFUL)

/*
void            vdk_event_dispatch(vdk_object_t *object,
                    uint32_t event,void *anything);
*/


#endif
