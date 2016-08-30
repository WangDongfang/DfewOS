
#include <linux/interrupt.h>

#ifndef _PLATFORM_DEVICE_H_
#define _PLATFORM_DEVICE_H_


struct device {
    irq_handler_t handler;
    unsigned long irqflags;
};

struct platform_device {
    const char      *name;
    struct device    dev;
};

#endif                                                                  /*  _PLATFORM_DEVICE_H_         */
