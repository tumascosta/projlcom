#include <lcom/lcf.h>
#include <lcom/lab3.h>

#include <stdbool.h>
#include <stdint.h>

int (kbc_subscribe_int)(uint8_t *irq_set);
int (kbc_unsubscribe_int)();
void (kbc_ih)();
int (kbc_poll_byte)(uint8_t *data);
int (kbc_restore_kbd_int)();

int (timer_subscribe_int)(uint8_t *bit_no);
int (timer_unsubscribe_int)();
void (timer_int_handler)();

extern uint8_t scancode;
extern bool scancode_ready;
extern uint32_t cnt;
extern uint32_t timer_counter;

static int process_scancode_byte(uint8_t byte, uint8_t bytes[2], uint8_t *size, bool *done) {
  if (byte == 0xE0) {
    bytes[0] = byte;
    *size = 1;
    return 0;
  }

  if (*size == 1) {
    bytes[1] = byte;
    *size = 2;
  }
  else {
    bytes[0] = byte;
    *size = 1;
  }

  bool make = !(bytes[*size - 1] & BIT(7));

  if (kbd_print_scancode(make, *size, bytes) != 0)
    return 1;

  if (*size == 1 && bytes[0] == 0x81)
    *done = true;

  *size = 0;
  return 0;
}

int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");

  lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");
  lcf_log_output("/home/lcom/labs/lab3/output.txt");

  if (lcf_start(argc, argv))
    return 1;

  lcf_cleanup();
  return 0;
}

int(kbd_test_scan)() {
  int ipc_status, r;
  message msg;
  uint8_t irq_set;

  uint8_t bytes[2];
  uint8_t size = 0;
  bool done = false;

  if (kbc_subscribe_int(&irq_set) != 0)
    return 1;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & BIT(irq_set)) {
            kbc_ih();

            if (!scancode_ready)
              break;

            if (process_scancode_byte(scancode, bytes, &size, &done) != 0) {
              kbc_unsubscribe_int();
              return 1;
            }
          }
          break;
        default:
          break;
      }
    }
  }

  if (kbc_unsubscribe_int() != 0)
    return 1;

  kbd_print_no_sysinb(cnt);
  return 0;
}

int(kbd_test_poll)() {
  uint8_t bytes[2];
  uint8_t size = 0;
  uint8_t data;
  bool done = false;

  while (!done) {
    if (kbc_poll_byte(&data) != 0)
      return 1;

    if (process_scancode_byte(data, bytes, &size, &done) != 0)
      return 1;
  }

  if (kbc_restore_kbd_int() != 0)
    return 1;

  kbd_print_no_sysinb(cnt);
  return 0;
}

int(kbd_test_timed_scan)(uint8_t n) {
  int ipc_status, r;
  message msg;
  uint8_t kbd_irq_set, timer_irq_set;

  uint8_t bytes[2];
  uint8_t size = 0;
  bool done = false;

  if (kbc_subscribe_int(&kbd_irq_set) != 0)
    return 1;

  if (timer_subscribe_int(&timer_irq_set) != 0) {
    kbc_unsubscribe_int();
    return 1;
  }

  timer_counter = 0;

  while (!done && timer_counter < (uint32_t)n * 60) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed with: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & BIT(timer_irq_set)) {
            timer_int_handler();
          }

          if (msg.m_notify.interrupts & BIT(kbd_irq_set)) {
            kbc_ih();

            if (!scancode_ready)
              break;

            timer_counter = 0;

            if (process_scancode_byte(scancode, bytes, &size, &done) != 0) {
              timer_unsubscribe_int();
              kbc_unsubscribe_int();
              return 1;
            }
          }
          break;
        default:
          break;
      }
    }
  }

  if (timer_unsubscribe_int() != 0) {
    kbc_unsubscribe_int();
    return 1;
  }

  if (kbc_unsubscribe_int() != 0)
    return 1;

  kbd_print_no_sysinb(cnt);
  return 0;
}
