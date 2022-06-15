#ifndef _KERNEL_H_
#define _KERNEL_H_

#define OPTIMIZATION_ARRAY_SIZE 500

#ifdef DEBUG
#define ASSERT(X) if(!X) {__asm volatile("bkpt");} else do {} while(0) 
#define DEBUG_PRINT(...) printk(__VA_ARGS__)
#else
#define ASSERT(...) do {} while(0)
#define DEBUG_PRINT(...) do {} while(0)
#endif /* DEBUG */

#endif /* _KERNEL_H_ */
