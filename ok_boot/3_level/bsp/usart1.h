#ifndef _USART1_H_
#define _USART1_H_

#include "stm32f10x_conf.h"

void usart1_init(u32 bound);
void usart1_send_char(u8 ch);
void usart1_rx_enable(void);
void usart1_rx_disable(void);

#endif
