#include "stmflash.h"

/**
 * write the stm32 flash without check
 * @param addr : the address you want to write into
 * @param buf : the pointer stores the data
 * @param read_halfwords : how many halfwords to write
 */
void stm32flash_write_no_check(u32 addr, u16 *buf, u16 write_halfwords)
{
	u32 i;

	for(i = 0; i < write_halfwords; i ++)
	{
		FLASH_ProgramHalfWord(addr, buf[i]);
		addr += 2;
	}
}

/**
 * write the stm32 flash with check, you can treat the stm32 flash as a normal ram
 * @param addr : the address you want to write into, must be a multiple of 2!
 * @param buf : the pointer stores the data
 * @param read_halfwords : how many halfwords to write
 * @return 0 : sucess
 *         other : fail
 */
u8 stmflash_write(u32 addr, u16 *buf, u16 write_halfwords)
{
	static u16 tmp_buf[STM_SECTOR_SIZE / 2];
	u32 secpos;
	u16 secoff;
	u16 secremain;
	u16 i;
	u32 offaddr;

	if(addr < STM32_FLASH_BASE || (addr + write_halfwords * 2 > (STM32_FLASH_BASE + STM32_FLASH_SIZE)))
	{
		return 1;
	}
	FLASH_Unlock();
	offaddr = addr - STM32_FLASH_BASE;
	secpos = offaddr / STM_SECTOR_SIZE;
	secoff = (offaddr % STM_SECTOR_SIZE) / 2;
	secremain = STM_SECTOR_SIZE / 2 - secoff;
	if(write_halfwords <= secremain)
	{
		secremain = write_halfwords;
	}
	while(1)
	{
		stmflash_read(secpos * STM_SECTOR_SIZE + STM32_FLASH_BASE, tmp_buf, STM_SECTOR_SIZE / 2);
		for(i = 0; i < secremain; i ++)
		{
			if(tmp_buf[secoff + i] != 0XFFFF)
			{
				break;
			}
		}
		if(i < secremain)
		{
			FLASH_ErasePage(secpos * STM_SECTOR_SIZE + STM32_FLASH_BASE);
			for(i = 0; i < secremain; i ++)
			{
				tmp_buf[i + secoff] = buf[i];
			}
			stm32flash_write_no_check(secpos * STM_SECTOR_SIZE + STM32_FLASH_BASE, tmp_buf, STM_SECTOR_SIZE / 2);
		}
		else
		{
			stm32flash_write_no_check(addr, buf, secremain);
		}
		if(write_halfwords == secremain)
		{
			break;
		}
		else
		{
			secpos ++;
			secoff = 0;
			buf += secremain;
			addr += secremain;
			write_halfwords -= secremain;
			if(write_halfwords > (STM_SECTOR_SIZE / 2))
			{
				secremain = STM_SECTOR_SIZE / 2;
			}
			else
			{
				secremain = write_halfwords;
			}
		}
	}
	FLASH_Lock();
	return 0;
}

/**
 * read data from stm32 flash
 * @param addr : the address you want to read out
 * @param buf : the pointer stores the data
 * @param read_halfwords : how many halfwords to read
 */
void stmflash_read(u32 addr, u16 *buf, u16 read_halfwords)
{
	u32 i;

	for(i = 0; i < read_halfwords; i ++)
	{
		buf[i] = *(vu16 *)addr;	//读取2个字节.
		addr += 2;	//偏移2个字节.
	}
}


