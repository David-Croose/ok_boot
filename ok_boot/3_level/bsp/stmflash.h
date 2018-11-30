#ifndef _STMFLASH_H_
#define _STMFLASH_H_

#include "stm32f10x_conf.h"

#define STM32_FLASH_SIZE    (512 * 1024)
#define STM32_FLASH_BASE    0x08000000

#if (STM32_FLASH_SIZE < 256 * 1024)
#define STM_SECTOR_SIZE    1024
#else
#define STM_SECTOR_SIZE	   2048
#endif

u8 stmflash_write(u32 addr, u16 *buf, u16 write_halfwords);
void stmflash_read(u32 addr, u16 *buf, u16 read_halfwords);

#endif
