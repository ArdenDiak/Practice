#include <boot_loader_autograder.h>
#include <unistd.h>
#include <printk.h>

int fib[10] = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55};
int fib_temp[10];
int uninitialized;

/**
 * @brief      Checks to see if the bootloader has been setup correctly.
 *             Global variables must have the correct values and the bss must
 *             be cleared. This test will not conclusively catch the bss being
 *             cleared that will be manually graded.
 *
 * @return     Returns 0 if succesful, -1 otherwise.
 */
int boot_loader_test() {
  fib_temp[0] = 1;
  fib_temp[1] = 1;

  for ( int i = 2; i < 10; ++i ) {
    fib_temp[i] = fib_temp[i - 1] + fib_temp[i - 2];
  }

  if ( uninitialized != 0 ) {
    printk( "Bss not cleared" );
    return -1;
  }

  for ( int i = 0; i < 10; ++i ) {
    if( fib[i] != fib_temp[i] ) {
      printk( "Failed to initialize global var\n" );
      printk( "Make sure you copy all the global vars from flash to sram\n" );
      return -1;
    }
  }

  printk( "Boot Loader Successful !\n" );
  return 0;
}


/**
 * @brief      This compares two arrays for equality.
 *
 * @param      array_unoptimized  The array unoptimized
 * @param      array_optimized    The array optimized
 *
 * @return     Returns 0 if succesful, -1 otherwise.
 */
int test_correctness( uint16_t array_unoptimized[OPTIMIZATION_ARRAY_SIZE],
                      uint16_t array_optimized[OPTIMIZATION_ARRAY_SIZE] ) {

  for ( int i = 0; i < OPTIMIZATION_ARRAY_SIZE; ++i ) {
    if ( array_unoptimized[i] != array_optimized[i] ) {
      return -1;
    }
  }

  return 0;
}
