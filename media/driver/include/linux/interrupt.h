
#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#include <linux/kernel.h>
#include <linux/sched.h>
#include <plat/irqs.h>


#define IRQF_DISABLED       0x00000020

typedef int irqreturn_t;
#define IRQ_NONE    (0)
#define IRQ_HANDLED (1)
#define IRQ_RETVAL(x)   ((x) != 0)

typedef irqreturn_t (*irq_handler_t)(int, void *);

extern int request_irq(unsigned int, irq_handler_t handler,
               unsigned long, const char *, void *);

extern void free_irq(unsigned int, void *);

struct pt_regs;

#endif                                                                  /*  _LINUX_INTERRUPT_H          */
