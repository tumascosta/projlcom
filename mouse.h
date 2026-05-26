#ifndef _LCOM_MOUSE_H_
#define _LCOM_MOUSE_H_

#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdint.h>

int (mouse_subscribe_int)(uint8_t *bit_no);
int (mouse_unsubscribe_int)();
void (mouse_ih)();

int (mouse_enable_stream_reporting)();
int (mouse_disable_stream_reporting)();

bool (mouse_packet_ready)();
void (mouse_build_packet)(struct packet *pp);
void (mouse_reset_packet_state)();

extern uint8_t mouse_byte;
extern bool mouse_byte_ready;

#endif
