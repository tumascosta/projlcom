#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdint.h>

#include "i8042.h"
#include "mouse.h"

extern int (util_sys_inb)(int port, uint8_t *value);

static int hook_id_mouse = 2;

uint8_t mouse_byte = 0;
bool mouse_byte_ready = false;

static uint8_t packet_bytes[3];
static uint8_t packet_index = 0;
static bool packet_complete = false;

static int (kbc_read_mouse_byte)(uint8_t *data) {
  uint8_t status;
  uint8_t value;

  if (data == NULL)
    return 1;

  if (util_sys_inb(KBC_ST_REG, &status) != 0)
    return 1;

  if ((status & OBF) == 0)
    return 1;

  if ((status & AUX) == 0)
    return 1;

  if (util_sys_inb(KBC_OUT_BUF, &value) != 0)
    return 1;

  if (status & (PAR_ERR | TO_ERR))
    return 1;

  *data = value;
  return 0;
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

static int (mouse_read_response)(uint8_t *resp) {
  for (int i = 0; i < RETRIES; i++) {
    if (kbc_read_mouse_byte(resp) == 0)
      return 0;

    tickdelay(micros_to_ticks(DELAY_US));
  }

  return 1;
}

static int (mouse_send_cmd)(uint8_t cmd) {
  uint8_t resp;

  for (int i = 0; i < RETRIES; i++) {
    if (kbc_write_cmd(WRITE_TO_MOUSE) != 0)
      return 1;

    if (kbc_write_arg(cmd) != 0)
      return 1;

    if (mouse_read_response(&resp) != 0)
      return 1;

    if (resp == MOUSE_ACK)
      return 0;

    if (resp == MOUSE_NACK || resp == MOUSE_ERROR)
      continue;

    return 1;
  }

  return 1;
}

int (mouse_subscribe_int)(uint8_t *bit_no) {
  if (bit_no == NULL)
    return 1;

  *bit_no = BIT(hook_id_mouse);

  if (sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id_mouse) != 0)
    return 1;

  return 0;
}

int (mouse_unsubscribe_int)() {
  if (sys_irqrmpolicy(&hook_id_mouse) != 0)
    return 1;

  return 0;
}

void (mouse_ih)() {
  uint8_t data;

  mouse_byte_ready = false;

  if (kbc_read_mouse_byte(&data) != 0)
    return;

  mouse_byte = data;
  mouse_byte_ready = true;

  if (packet_index == 0 && ((data & BIT(3)) == 0))
    return;

  packet_bytes[packet_index] = data;
  packet_index++;

  if (packet_index == 3) {
    packet_index = 0;
    packet_complete = true;
  }
}

int (mouse_enable_stream_reporting)() {
  return mouse_send_cmd(ENABLE_DATA_REPORTING);
}

int (mouse_disable_stream_reporting)() {
  return mouse_send_cmd(DISABLE_DATA_REPORTING);
}

bool (mouse_packet_ready)() {
  return packet_complete;
}

void (mouse_reset_packet_state)() {
  packet_complete = false;
  packet_index = 0;
}

void (mouse_build_packet)(struct packet *pp) {
  if (pp == NULL)
    return;

  pp->bytes[0] = packet_bytes[0];
  pp->bytes[1] = packet_bytes[1];
  pp->bytes[2] = packet_bytes[2];

  pp->lb = (packet_bytes[0] & BIT(0)) != 0;
  pp->rb = (packet_bytes[0] & BIT(1)) != 0;
  pp->mb = (packet_bytes[0] & BIT(2)) != 0;

  pp->x_ov = (packet_bytes[0] & BIT(6)) != 0;
  pp->y_ov = (packet_bytes[0] & BIT(7)) != 0;

  pp->delta_x = (packet_bytes[0] & BIT(4)) ? (int16_t)(0xFF00 | packet_bytes[1]) : (int16_t)packet_bytes[1];
  pp->delta_y = (packet_bytes[0] & BIT(5)) ? (int16_t)(0xFF00 | packet_bytes[2]) : (int16_t)packet_bytes[2];

  packet_complete = false;
}
