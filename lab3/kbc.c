#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdint.h>

#define KBC_IRQ       1
#define KBC_OUT_BUF   0x60
#define KBC_IN_BUF    0x60
#define KBC_CMD_REG   0x64
#define KBC_ST_REG    0x64

#define READ_CMD_BYTE  0x20
#define WRITE_CMD_BYTE 0x60

#define OBF     BIT(0)
#define IBF     BIT(1)
#define AUX     BIT(5)
#define TO_ERR  BIT(6)
#define PAR_ERR BIT(7)

#define KBD_INT_EN BIT(0)
#define KBD_DIS    BIT(4)

#define DELAY_US 20000
#define RETRIES  10

int hook_id_kbd = 1;
uint8_t scancode = 0;
bool scancode_ready = false;

int (kbc_subscribe_int)(uint8_t *irq_set) {
  if (irq_set == NULL)
    return 1;

  *irq_set = hook_id_kbd;

  if (sys_irqsetpolicy(KBC_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id_kbd) != 0)
    return 1;

  return 0;
}

int (kbc_unsubscribe_int)() {
  if (sys_irqrmpolicy(&hook_id_kbd) != 0)
    return 1;

  return 0;
}

static int (kbc_read_out_buf_once)(uint8_t *data, bool mouse) {
  uint8_t status;
  uint8_t value;

  if (data == NULL)
    return 1;

  if (util_sys_inb(KBC_ST_REG, &status) != 0)
    return 1;

  if ((status & OBF) == 0)
    return 1;

  if (util_sys_inb(KBC_OUT_BUF, &value) != 0)
    return 1;

  if (status & (PAR_ERR | TO_ERR))
    return 1;

  if (mouse) {
    if ((status & AUX) == 0)
      return 1;
  }
  else {
    if (status & AUX)
      return 1;
  }

  *data = value;
  return 0;
}

int (kbc_poll_byte)(uint8_t *data) {
  if (data == NULL)
    return 1;

  while (true) {
    if (kbc_read_out_buf_once(data, false) == 0)
      return 0;

    tickdelay(micros_to_ticks(DELAY_US));
  }
}

static int (kbc_write_cmd)(uint8_t cmd) {
  uint8_t status;

  for (int i = 0; i < RETRIES; i++) {
    if (util_sys_inb(KBC_ST_REG, &status) != 0)
      return 1;

    if ((status & IBF) == 0) {
      if (sys_outb(KBC_CMD_REG, cmd) != 0)
        return 1;
      return 0;
    }

    tickdelay(micros_to_ticks(DELAY_US));
  }

  return 1;
}

static int (kbc_write_arg)(uint8_t arg) {
  uint8_t status;

  for (int i = 0; i < RETRIES; i++) {
    if (util_sys_inb(KBC_ST_REG, &status) != 0)
      return 1;

    if ((status & IBF) == 0) {
      if (sys_outb(KBC_IN_BUF, arg) != 0)
        return 1;
      return 0;
    }

    tickdelay(micros_to_ticks(DELAY_US));
  }

  return 1;
}

int (kbc_read_command_byte)(uint8_t *cmd_byte) {
  if (cmd_byte == NULL)
    return 1;

  if (kbc_write_cmd(READ_CMD_BYTE) != 0)
    return 1;

  for (int i = 0; i < RETRIES; i++) {
    if (kbc_read_out_buf_once(cmd_byte, false) == 0)
      return 0;

    tickdelay(micros_to_ticks(DELAY_US));
  }

  return 1;
}

int (kbc_write_command_byte)(uint8_t cmd_byte) {
  if (kbc_write_cmd(WRITE_CMD_BYTE) != 0)
    return 1;

  if (kbc_write_arg(cmd_byte) != 0)
    return 1;

  return 0;
}

int (kbc_restore_kbd_int)() {
  uint8_t cmd_byte;

  if (kbc_read_command_byte(&cmd_byte) != 0)
    return 1;

  cmd_byte |= KBD_INT_EN;
  cmd_byte &= ~KBD_DIS;

  if (kbc_write_command_byte(cmd_byte) != 0)
    return 1;

  return 0;
}

void (kbc_ih)() {
  uint8_t data;

  scancode_ready = false;

  if (kbc_read_out_buf_once(&data, false) != 0)
    return;

  scancode = data;
  scancode_ready = true;
}
