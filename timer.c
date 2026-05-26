#include <lcom/lcf.h>
#include <stdint.h>

#include "i8254.h"

int hook_id_timer = 0;
uint32_t timer_counter = 0;

int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  if (timer > 2 || freq == 0 || freq > TIMER_FREQ)
    return 1;

  uint8_t st;
  if (timer_get_conf(timer, &st) != 0)
    return 1;

  uint8_t control = (st & 0x0F);
  switch (timer) {
    case 0: control |= TIMER_SEL0; break;
    case 1: control |= TIMER_SEL1; break;
    case 2: control |= TIMER_SEL2; break;
    default: return 1;
  }

  control |= TIMER_LSB_MSB;

  uint16_t initial = TIMER_FREQ / freq;
  uint8_t lsb, msb;

  if (util_get_LSB(initial, &lsb) != 0)
    return 1;
  if (util_get_MSB(initial, &msb) != 0)
    return 1;

  uint8_t port;
  switch (timer) {
    case 0: port = TIMER_0; break;
    case 1: port = TIMER_1; break;
    case 2: port = TIMER_2; break;
    default: return 1;
  }

  if (sys_outb(TIMER_CTRL, control) != 0)
    return 1;
  if (sys_outb(port, lsb) != 0)
    return 1;
  if (sys_outb(port, msb) != 0)
    return 1;

  return 0;
}

int (timer_subscribe_int)(uint8_t *bit_no) {
  if (bit_no == NULL)
    return 1;

  *bit_no = hook_id_timer;

  if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id_timer) != 0)
    return 1;

  return 0;
}

int (timer_unsubscribe_int)() {
  if (sys_irqrmpolicy(&hook_id_timer) != 0)
    return 1;

  return 0;
}

void (timer_int_handler)() {
  timer_counter++;
}

int (timer_get_conf)(uint8_t timer, uint8_t *st) {
  uint8_t temp;
  uint8_t rb_cmd;
  uint8_t port;

  if (st == NULL || timer > 2)
    return 1;

  rb_cmd = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);

  if (sys_outb(TIMER_CTRL, rb_cmd) != 0)
    return 1;

  switch (timer) {
    case 0:
      port = TIMER_0;
      break;
    case 1:
      port = TIMER_1;
      break;
    case 2:
      port = TIMER_2;
      break;
    default:
      return 1;
  }

  if (util_sys_inb(port, &temp) != 0)
    return 1;

  *st = temp;
  return 0;
}

int (timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {
  union timer_status_field_val val;

  switch (field) {
    case tsf_all:
      val.byte = st;
      break;

    case tsf_initial:
      val.in_mode = (enum timer_init)((st >> 4) & 0x03);
      break;

    case tsf_mode: {
      uint8_t mode = (st >> 1) & 0x07;
      if (mode == 6) mode = 2;
      else if (mode == 7) mode = 3;
      val.count_mode = mode;
      break;
    }

    case tsf_base:
      val.bcd = (st & TIMER_BCD) != 0;
      break;

    default:
      return 1;
  }

  if (timer_print_config(timer, field, val) != 0)
    return 1;

  return 0;
}
