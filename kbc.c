#include <lcom/lcf.h>

#include <stdbool.h>
#include <stdint.h>

#include "kbc.h"

#define KBC_IRQ 1

#define KBC_OUT_BUF 0x60
#define KBC_ST_REG  0x64

#define OBF     BIT(0)
#define AUX     BIT(5)
#define TO_ERR  BIT(6)
#define PAR_ERR BIT(7)

static int hook_id_kbd = 1;

uint8_t kbd_scancode = 0;
bool kbd_scancode_ready = false;

int my_kbc_subscribe_int(uint8_t *irq_set) {
  if (irq_set == NULL)
    return 1;

  *irq_set = BIT(hook_id_kbd);

  if (sys_irqsetpolicy(KBC_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id_kbd) != 0)
    return 1;

  return 0;
}

int my_kbc_unsubscribe_int(void) {
  if (sys_irqrmpolicy(&hook_id_kbd) != 0)
    return 1;

  return 0;
}

void my_kbc_ih(void) {
  uint8_t status;
  uint8_t data;

  kbd_scancode_ready = false;

  if (util_sys_inb(KBC_ST_REG, &status) != 0)
    return;

  if ((status & OBF) == 0)
    return;

  if (status & AUX)
    return;

  if (status & (PAR_ERR | TO_ERR))
    return;

  if (util_sys_inb(KBC_OUT_BUF, &data) != 0)
    return;

  kbd_scancode = data;
  kbd_scancode_ready = true;
}
