#include <lcom/lcf.h>
#include <stdint.h>
#include <stdbool.h>

#include "video.h"
#include "mouse.h"

int (kbc_subscribe_int)(uint8_t *irq_set);
int (kbc_unsubscribe_int)();

// --- DECLARAÇÕES EXTERNAS DE VARIÁVEIS DOS TEUS LABS ---
extern uint8_t scancode;
extern bool scancode_ready;
extern uint32_t timer_counter;

int main(int argc, char *argv[]) {
  lcf_start(argc, argv);
  return 0;
}

int (proj_main_loop)(int argc, char *argv[]) {
  
  // 1. INICIALIZAR O MODO GRÁFICO (Usa o modo estável do teu Lab 5)
  uint16_t modo_grafico = 0x14C; 
  if (my_vg_init(modo_grafico) == NULL) {
    return 1;
  }

  // 2. SUBSCREVER AS INTERRUPÇÕES DO HARDWARE
  uint8_t timer_bit, kbc_bit, mouse_bit;
  uint32_t timer_irq, kbc_irq, mouse_irq;

  if (timer_subscribe_int(&timer_bit) != 0) { vg_exit(); return 1; }
  if (kbc_subscribe_int(&kbc_bit) != 0) { timer_unsubscribe_int(); vg_exit(); return 1; }
  if (mouse_subscribe_int(&mouse_bit) != 0) { kbc_unsubscribe_int(); timer_unsubscribe_int(); vg_exit(); return 1; }
  
  mouse_enable_stream_reporting();

  timer_irq = BIT(timer_bit);
  kbc_irq = BIT(kbc_bit);
  mouse_irq = BIT(mouse_bit);

  int ipc_status;
  message msg;
  bool jogo_a_correr = true;
  struct packet mouse_pkt;

  // Variáveis para a posição do quadrado de teste
  uint16_t player_x = 200;
  uint16_t player_y = 200;

  // 3. O LOOP PRINCIPAL DO JOGO
  while (jogo_a_correr) {
    
    if (driver_receive(ANY, &msg, &ipc_status) != 0) {
      continue;
    }
    
    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE: {
          
          // 3A. INTERRUPÇÃO DO TECLADO
          if (msg.m_notify.interrupts & kbc_irq) {
            kbc_ih(); 
            if (scancode_ready) {
              if (scancode == 0x81) { jogo_a_correr = false; } // ESC para sair
              
              switch (scancode) {
                case 0x48: case 0x11: player_y -= 10; break; // Up / W
                case 0x50: case 0x1F: player_y += 10; break; // Down / S
                case 0x4B: case 0x1E: player_x -= 10; break; // Left / A
                case 0x4D: case 0x20: player_x += 10; break; // Right / D
              }
            }
          }

          // 3B. INTERRUPÇÃO DO RATO
          if (msg.m_notify.interrupts & mouse_irq) {
            mouse_ih(); 
            if (mouse_packet_ready()) {
              mouse_build_packet(&mouse_pkt);
            }
          }

          // 3C. INTERRUPÇÃO DO TIMER
          if (msg.m_notify.interrupts & timer_irq) {
            timer_int_handler();
            
            vg_clear(); // Limpa ecrã virtual
            
            // Desenhos de teste
            my_vg_draw_rectangle(100, 100, 60, 60, 0x0000FF);  // Obstáculo azul
            my_vg_draw_rectangle(player_x, player_y, 20, 20, 0xFFFF00); // Boneco amarelo

            vg_flush(); // Copia para o ecrã real
          }
          break;
        }
        default:
          break; 
      }
    }
  }

  // 4. LIMPEZA FINAL
  mouse_disable_stream_reporting();
  mouse_unsubscribe_int();
  kbc_unsubscribe_int();
  timer_unsubscribe_int();
  vg_exit(); 

  return 0;
}
