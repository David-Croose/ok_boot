#include "boot.h"

int main(void)
{
	boot_init();

	while(1)
	{
		boot_proc();
	}
}
