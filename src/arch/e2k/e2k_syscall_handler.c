/**
 * @file
 * @brief
 *
 * @date Mar 13, 2018
 * @author Anton Bondarev
 */

#include <stdint.h>

#include <kernel/printk.h>

unsigned long cpuSyscall(int number, uint64_t arg1, uint64_t arg2,
			 uint64_t arg3, uint64_t arg4)
{
	printk("syscall handler %d (%x, %x, %x, %x)\n", number, arg1, arg2, arg3, arg4);

	return 0;
}
