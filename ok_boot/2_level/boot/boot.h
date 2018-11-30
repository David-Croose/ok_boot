#ifndef _BOOT_H_
#define _BOOT_H_

#include "usart1.h"
#include "stmflash.h"

typedef enum
{
	BOOT_SIGNAL_RESET,
	BOOT_SIGNAL_SEND,
	BOOT_SIGNAL_GET,
}
boot_signal_t;

typedef enum
{
	BOOT_BUFOP_RESET,
	BOOT_BUFOP_SEND,
	BOOT_BUFOP_GET,
}
boot_bufop_t;    // boot buffer opration

#define BOOT_CMD_CONNECT    'A'
#define BOOT_CMD_TRANSMIT   'M'
#define BOOT_CMD_RUN        'R'
#define BOOT_CMD_RESET      'S'
#define BOOT_CMD_ACK        'C'
#define BOOT_ACK_OK          1
#define BOOT_ACK_ERROR       0

// here are the items you configure
//==============================================================================================================
#define BOOT_VERSION_1     1
#define BOOT_VERSION_2     0
#define BOOT_VERSION_3     0
#define BOOT_VERSION_4     0

#define BOOT_HW_INIT()         do { NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); usart1_init(115200); } while(0)
#define BOOT_SEND_CHAR(x)      do { usart1_send_char(x); } while(0)
#define BOOT_RECV_CHAR()
#define BOOT_RECV_ENABLE()     do { usart1_rx_enable(); } while(0)
#define BOOT_RECV_DISABLE()    do { usart1_rx_disable(); } while(0)

#define BOOT_RECV_BUF_SIZE    1100
#define BOOT_SEND_BUF_SIZE    30
#define BOOT_ADDR             0
#define BOOT_MULTICAST_ADDR   0xFF

#define BOOT_WRITE_BYTE_PER_TIME    1024
#define BOOT_APP_ADDR_BEGIN         (0x8000000 + 0x8000)
#define BOOT_APP_ADDR_END           (BOOT_APP_ADDR_BEGIN + 120 * 1024 - 1)
#define BOOT_JUMP_TO_BOOT()         NVIC_SystemReset()
#define BOOT_JUMP_TO_APP()                                                        \
do                                                                                \
{                                                                                 \
    typedef void (*jump_fun_t)(void);                                             \
	register jump_fun_t jump = (jump_fun_t)(*(vu32 *)(BOOT_APP_ADDR_BEGIN + 4));  \
	__set_MSP(*(vu32 *)BOOT_APP_ADDR_BEGIN);                                      \
	jump();                                                                       \
}                                                                                 \
while(0)

#define BOOT_WRITE_CODE(addr, buf, size)    stmflash_write(addr, buf, size)
//--------------------------------------------------------------------------------------------------------------

void boot_init(void);
void boot_recv_in_irq(u8 byte);
void boot_proc(void);

#endif
