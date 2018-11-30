#include <string.h>
#include "boot.h"
#include "crc.h"
#include "core_cm3.h"

u8 *boot_recvbuf(boot_bufop_t op)
{
	static u8 recv_buf[BOOT_RECV_BUF_SIZE];

	if(op == BOOT_BUFOP_RESET)
	{
		memset(recv_buf, 0, sizeof(recv_buf));
		return NULL;
	}
	else if(op == BOOT_BUFOP_GET)
	{
		return recv_buf;
	}

	return NULL;
}

u8 *boot_sendbuf(boot_bufop_t op)
{
	static u8 send_buf[BOOT_SEND_BUF_SIZE];
	u16 i, total_len;

	if(op == BOOT_BUFOP_RESET)
	{
		memset(send_buf, 0, sizeof(send_buf));
		return NULL;
	}
	else if(op == BOOT_BUFOP_GET)
	{
		return send_buf;
	}
	else if(op == BOOT_BUFOP_SEND)
	{
		total_len = send_buf[5];
		total_len <<= 8;
		total_len |= send_buf[4];
		total_len += 6;
		for(i = 0; i < total_len; i ++)
		{
			BOOT_SEND_CHAR(send_buf[i]);
		}

		return NULL;
	}

	return NULL;
}

void boot_init(void)
{
	BOOT_HW_INIT();
}

void boot_message_handle_start(void)
{
	BOOT_RECV_DISABLE();
}

void boot_message_handle_end(void)
{
	BOOT_RECV_ENABLE();
}

bool boot_signal(boot_signal_t signal)
{
	static bool boot_recv_flag;

	if(signal == BOOT_SIGNAL_SEND)
	{
		boot_recv_flag = TRUE;
		return TRUE;
	}
	else if(signal == BOOT_SIGNAL_GET)
	{
		if(boot_recv_flag == TRUE)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		boot_recv_flag = FALSE;
		return TRUE;
	}

	/// return FALSE;
}

/**
 * check the frame's format
 * @param frame : the frame's first byte address
 * @return TRUE : the frame's format is right
 *         FALSE : the frame's format is wrong
 */
bool boot_is_frame_right(u8 *frame)
{
	u16 body_len;
	u16 crc16_calc, crc16_recv;
	u16 crc16_pos_h, crc16_pos_l;

	if(frame[0] != 0xFF
	   || frame[1] != 0xFF
	   || frame[2] != 0xFF
	   || frame[3] != 0xFF)
	{
		return FALSE;
	}

	body_len = frame[5];
	body_len <<= 8;
	body_len |= frame[4];
	if(body_len + 6 > BOOT_RECV_BUF_SIZE)
	{
		return FALSE;
	}

	crc16_calc = crc16(0, &frame[6], body_len - 2);

	crc16_pos_h = body_len + 6 - 1;
	crc16_pos_l = crc16_pos_h - 1;
	crc16_recv = frame[crc16_pos_h];
	crc16_recv <<= 8;
	crc16_recv |= frame[crc16_pos_l];
	if(crc16_calc != crc16_recv)
	{
		return FALSE;
	}

	return TRUE;
}

/**
 * receive byte one by one in receiving IRQ, and collect a complete frame finally
 * @param byte : the received byte
 * @return TRUE : received a right frame
 *         FALSE : received error or receive not complete
 */
bool boot_recv_byte(u8 byte)
{
    static u8 addr;
	static u32 recv_seq;
	static u16 body_len;
	u16 tmp;
	u8 *buf;

    buf = boot_recvbuf(BOOT_BUFOP_GET);

	if(recv_seq < 4)
	{
		if(byte == 0xFF)
		{
			recv_seq ++;
		}
		else
		{
			goto error;
		}
	}
	else if(recv_seq == 4)
	{
		body_len = byte;
		recv_seq ++;
	}
	else if(recv_seq == 5)
	{
		tmp = byte;
		tmp <<= 8;
		body_len |= tmp;
		if(body_len + 6 <= BOOT_RECV_BUF_SIZE)
		{
			recv_seq ++;
		}
		else
		{
			goto error;
		}
	}
	else if(recv_seq == 6)
	{
		if(byte == BOOT_ADDR || byte == BOOT_MULTICAST_ADDR)
		{
            addr = byte;
			recv_seq ++;
		}
		else
		{
			goto error;
		}
	}
	else if(recv_seq == 7)    // the commant word
	{
		buf[0] = 0xFF;
		buf[1] = 0xFF;
		buf[2] = 0xFF;
		buf[3] = 0xFF;
		buf[4] = body_len;
		buf[5] = body_len >> 8;
		buf[6] = addr;
		buf[7] = byte;
		recv_seq ++;
	}
	else if(recv_seq > 7 && recv_seq < BOOT_RECV_BUF_SIZE)
	{
		buf[recv_seq] = byte;
		if(recv_seq >= body_len + 6 - 1)
		{
            recv_seq = 0;
            body_len = 0;
			return TRUE;
		}
		recv_seq ++;
	}
	else
	{
		goto error;
	}

	return FALSE;

error:
	boot_recvbuf(BOOT_BUFOP_RESET);
	recv_seq = 0;
	body_len = 0;
	return FALSE;
}

void boot_recv_in_irq(u8 byte)
{
	if(boot_recv_byte(byte) == TRUE)
	{
		boot_message_handle_start();
		boot_signal(BOOT_SIGNAL_SEND);
	}
}

bool boot_get_code_from_remote(u8 *frame)
{
    static u32 last_addr;
	u32 addr;

	addr = frame[13];
	addr <<= 8;
	addr |= frame[12];
	addr <<= 8;
	addr |= frame[11];
	addr <<= 8;
	addr |= frame[10];
	if(addr < BOOT_APP_ADDR_BEGIN || addr + BOOT_WRITE_BYTE_PER_TIME - 1 > BOOT_APP_ADDR_END)
	{
		return FALSE;
	}
//    if(addr != BOOT_APP_ADDR_BEGIN && addr != last_addr + BOOT_WRITE_BYTE_PER_TIME)
//    {
//        return FALSE;
//    }
    last_addr = addr;

	if(BOOT_WRITE_CODE(addr, (u16 *)&frame[18], BOOT_WRITE_BYTE_PER_TIME / 2))
	{
		return FALSE;
	}
	return TRUE;
}

void boot_ack_connect(u16 seq, bool res)
{
	u8 *buf;
	u16 body_len, crc16_calc;

	(void)res;

	body_len = 19;
	boot_sendbuf(BOOT_BUFOP_RESET);
	buf = boot_sendbuf(BOOT_BUFOP_GET);

	buf[0] = 0xEE;
	buf[1] = 0xEE;
	buf[2] = 0xEE;
	buf[3] = 0xEE;

	buf[4] = body_len;
	buf[5] = body_len >> 8;

	buf[6] = BOOT_ADDR;
	buf[7] = BOOT_CMD_ACK;

	buf[8] = seq;
	buf[9] = seq >> 8;

	buf[10] = BOOT_CMD_CONNECT;

	buf[11] = BOOT_VERSION_1;
	buf[12] = BOOT_VERSION_2;
	buf[13] = BOOT_VERSION_3;
	buf[14] = BOOT_VERSION_4;
	buf[15] = 0;
	buf[16] = 0;
	buf[17] = 0;
	buf[18] = 0;

	buf[19] = 0;
	buf[20] = 0;
	buf[21] = 0;
	buf[22] = 0;

	crc16_calc = crc16(0, &buf[6], body_len - 2);
	buf[23] = crc16_calc;
	buf[24] = crc16_calc >> 8;

	boot_sendbuf(BOOT_BUFOP_SEND);
}

void boot_ack_transmit(u16 seq, bool res)
{
	u8 *buf;
	u16 body_len, crc16_calc;

	body_len = 19;
	boot_sendbuf(BOOT_BUFOP_RESET);
	buf = boot_sendbuf(BOOT_BUFOP_GET);

	buf[0] = 0xEE;
	buf[1] = 0xEE;
	buf[2] = 0xEE;
	buf[3] = 0xEE;

	buf[4] = body_len;
	buf[5] = body_len >> 8;

	buf[6] = BOOT_ADDR;
	buf[7] = BOOT_CMD_ACK;

	buf[8] = seq;
	buf[9] = seq >> 8;

	buf[10] = BOOT_CMD_TRANSMIT;

	buf[11] = res;
	buf[12] = 0;
	buf[13] = 0;
	buf[14] = 0;
	buf[15] = 0;
	buf[16] = 0;
	buf[17] = 0;
	buf[18] = 0;

	buf[19] = 0;
	buf[20] = 0;
	buf[21] = 0;
	buf[22] = 0;

	crc16_calc = crc16(0, &buf[6], body_len - 2);
	buf[23] = crc16_calc;
	buf[24] = crc16_calc >> 8;

	boot_sendbuf(BOOT_BUFOP_SEND);
}

void boot_ack_run(u16 seq, bool res)
{
	u8 *buf;
	u16 body_len, crc16_calc;

	(void)res;

	body_len = 19;
	boot_sendbuf(BOOT_BUFOP_RESET);
	buf = boot_sendbuf(BOOT_BUFOP_GET);

	buf[0] = 0xEE;
	buf[1] = 0xEE;
	buf[2] = 0xEE;
	buf[3] = 0xEE;

	buf[4] = body_len;
	buf[5] = body_len >> 8;

	buf[6] = BOOT_ADDR;
	buf[7] = BOOT_CMD_ACK;

	buf[8] = seq;
	buf[9] = seq >> 8;

	buf[10] = BOOT_CMD_RUN;

	buf[11] = res;
	buf[12] = 0;
	buf[13] = 0;
	buf[14] = 0;
	buf[15] = 0;
	buf[16] = 0;
	buf[17] = 0;
	buf[18] = 0;

	buf[19] = 0;
	buf[20] = 0;
	buf[21] = 0;
	buf[22] = 0;

	crc16_calc = crc16(0, &buf[6], body_len - 2);
	buf[23] = crc16_calc;
	buf[24] = crc16_calc >> 8;

	boot_sendbuf(BOOT_BUFOP_SEND);
}

void boot_ack_reset(u16 seq, bool res)
{
	u8 *buf;
	u16 body_len, crc16_calc;

	(void)res;

	body_len = 19;
	boot_sendbuf(BOOT_BUFOP_RESET);
	buf = boot_sendbuf(BOOT_BUFOP_GET);

	buf[0] = 0xEE;
	buf[1] = 0xEE;
	buf[2] = 0xEE;
	buf[3] = 0xEE;

	buf[4] = body_len;
	buf[5] = body_len >> 8;

	buf[6] = BOOT_ADDR;
	buf[7] = BOOT_CMD_ACK;

	buf[8] = seq;
	buf[9] = seq >> 8;

	buf[10] = BOOT_CMD_RESET;

	buf[11] = res;
	buf[12] = 0;
	buf[13] = 0;
	buf[14] = 0;
	buf[15] = 0;
	buf[16] = 0;
	buf[17] = 0;
	buf[18] = 0;

	buf[19] = 0;
	buf[20] = 0;
	buf[21] = 0;
	buf[22] = 0;

	crc16_calc = crc16(0, &buf[6], body_len - 2);
	buf[23] = crc16_calc;
	buf[24] = crc16_calc >> 8;

	boot_sendbuf(BOOT_BUFOP_SEND);
}

void boot_proc(void)
{
	u8 *buf;
	u8 cmd;
	u16 seq;

	if(boot_signal(BOOT_SIGNAL_GET) != TRUE)
	{
		return;
	}

	buf = boot_recvbuf(BOOT_BUFOP_GET);
	if(boot_is_frame_right(buf) != TRUE)
	{
		goto end;
	}

	seq = buf[9];
	seq <<= 8;
	seq |= buf[8];
	cmd = buf[7];
	switch(cmd)
	{
	case BOOT_CMD_CONNECT:
		boot_ack_connect(seq, TRUE);
		break;

	case BOOT_CMD_TRANSMIT:
		if(boot_get_code_from_remote(buf) == TRUE)
		{
			boot_ack_transmit(seq, TRUE);
		}
		else
		{
			boot_ack_transmit(seq, FALSE);
		}
		break;

	case BOOT_CMD_RUN:
		boot_ack_run(seq, TRUE);
        BOOT_RECV_DISABLE();
		BOOT_JUMP_TO_APP();
		break;

	case BOOT_CMD_RESET:
		boot_ack_reset(seq, TRUE);
		BOOT_JUMP_TO_BOOT();
		break;

	default:
		break;
	}

end:
	boot_recvbuf(BOOT_BUFOP_RESET);
	boot_signal(BOOT_SIGNAL_RESET);
	boot_message_handle_end();
}
