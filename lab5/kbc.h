#ifndef KBC_H
#define KBC_H

#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdint.h>

extern uint8_t kbd_scancode;
extern bool kbd_scancode_ready;

int my_kbc_subscribe_int(uint8_t *irq_set);
int my_kbc_unsubscribe_int(void);
void my_kbc_ih(void);

#endif
