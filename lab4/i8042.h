#ifndef _LCOM_I8042_H_
#define _LCOM_I8042_H_

#include <lcom/lcf.h>

#define KBC_OUT_BUF 0x60
#define KBC_IN_BUF  0x60
#define KBC_CMD_REG 0x64
#define KBC_ST_REG  0x64

#define KBC_IRQ     1
#define MOUSE_IRQ   12

#define READ_CMD_BYTE  0x20
#define WRITE_CMD_BYTE 0x60
#define WRITE_TO_MOUSE 0xD4

#define OBF     BIT(0)
#define IBF     BIT(1)
#define AUX     BIT(5)
#define TO_ERR  BIT(6)
#define PAR_ERR BIT(7)

#define DELAY_US 20000
#define RETRIES  10

#define MOUSE_ACK    0xFA
#define MOUSE_NACK   0xFE
#define MOUSE_ERROR  0xFC

#define ENABLE_DATA_REPORTING  0xF4
#define DISABLE_DATA_REPORTING 0xF5

#endif
