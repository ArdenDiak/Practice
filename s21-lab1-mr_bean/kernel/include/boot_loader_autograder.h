#ifndef _BOOT_LOADER_AUTOGRADER_
#define _BOOT_LOADER_AUTOGRADER_

#include <kernel.h>
#include <unistd.h>

int boot_loader_test();
int test_correctness(uint16_t array_unoptimized[OPTIMIZATION_ARRAY_SIZE],
                     uint16_t array_optimized[OPTIMIZATION_ARRAY_SIZE]);


#endif /* _BOOT_LOADER_AUTOGRADER_ */
