#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "pico/bootrom.h"
#include "hardware/spi.h"
#include "hardware/watchdog.h"
#include "hardware/flash.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"

#include "tusb.h"

#include "hardware/uart.h"

#include "panel.h"
#include "text.h"
#include "drawscreen.h"

extern volatile screen_t screen;

#define MAGIC 0x5445524d  // Magic number to access admin features

extern const uint8_t __td2_config_start[], __td2_config_end[];
#define TD2_CONFIG_SIZE ((size_t)(__td2_config_end - __td2_config_start))

#define uart uart1

#define UART_TX_PIN 8
#define UART_RX_PIN 9
#define DTR_PIN     12
#define RTS_PIN     13

extern void lamps_age(void);

void debug(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    panel_text(buf, strlen(buf), 't');
    // uart_puts(uart, buf);
}

// ------------------------------------------------------------

#define FIFO_SIZE 32768

#define MOD_FIFO(x) ((x) & (FIFO_SIZE - 1))

typedef struct {
  uint8_t buf[FIFO_SIZE];
  size_t rd, wr;
} fifo_t;

fifo_t rxfifo;
static uint32_t last_rxflush = 0;

void fifo_wipe(fifo_t *f)
{
  f->wr = 0;
  f->rd = 0;
}

void fifo_add1(fifo_t *f, uint8_t c)
{
  f->buf[f->wr] = c;
  f->wr = MOD_FIFO(f->wr + 1);
}

ssize_t fifo_fullness(fifo_t *f)
{
  return MOD_FIFO(f->wr - f->rd);
}

uint8_t *fifo_get(fifo_t *f, ssize_t req, ssize_t *len)
{
  ssize_t rem = FIFO_SIZE - f->rd;
  *len = MIN(rem, MIN(fifo_fullness(f), req));
  uint8_t *r = f->buf + f->rd;
  f->rd  = MOD_FIFO(f->rd + *len);
  return r;
}

// ------------------------------------------------------------

static state_t current_state;

static void set_state(const state_t *ps)
{
  uint32_t baud = ps->bit_rate;

  if (baud > 1200) {
    clock_configure(clk_peri, 0,
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
        clock_get_hz(clk_sys),      // source freq
        clock_get_hz(clk_sys));     // target freq (must match)
  } else {
    clock_configure(clk_peri, 0,
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
        48 * MHZ, 48 * MHZ);
  }

  static const unsigned stops[] = {1, 1, 2};
  static const unsigned parities[] = {
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
    UART_PARITY_NONE,
    UART_PARITY_NONE,
  };

  uart_set_baudrate(uart, baud);
  uart_set_format(uart,
    ps->data_bits,
    stops[ps->stop_bits],
    parities[ps->parity]);
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* line_coding)
{
  current_state.version = 1000;
  current_state.bit_rate = line_coding->bit_rate;
  current_state.data_bits = line_coding->data_bits;
  current_state.parity = line_coding->parity;
  current_state.stop_bits = line_coding->stop_bits;

  set_state(&current_state);
  panel_line_coding(&current_state);
}

void tud_cdc_rx_cb(uint8_t itf)
{
  uint8_t tx_buf[64];
  size_t n;

  while ((n = MIN(sizeof(tx_buf), tud_cdc_available())) != 0) {
    tud_cdc_read(tx_buf, n);
    panel_text(tx_buf, n, 't');
    for (size_t i = 0; i < n; i++) {
      uart_putc_raw(uart, tx_buf[i]);
      while (uart_is_readable(uart)) {
        uint8_t ch = uart_getc(uart);
        fifo_add1(&rxfifo, ch);
        panel_text(&ch, 1, 'r');
      }
    }
  }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  bool m_dtr, m_rts;
  remap(dtr, rts, &m_dtr, &m_rts);
  panel_dtr_rts(m_dtr, m_rts);
  gpio_put(DTR_PIN, m_dtr);
  gpio_put(RTS_PIN, m_rts);
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t ms)
{
  debug("[[BREAK]]");
  uart_set_break(uart, 1);
}


void send_to_host(uint8_t *s, size_t n)
{
  ssize_t cdca = tud_cdc_write_available();
  if (cdca != 0) {
    tud_cdc_write(s, n);
    if (n == cdca) {
      tud_cdc_write_flush();
    }
  }
}

void rx_poll(void)
{
  while (uart_is_readable(uart)) {
    uint8_t ch = uart_getc(uart);
    fifo_add1(&rxfifo, ch);
    panel_text(&ch, 1, 'r');
  }
  ssize_t cdca = tud_cdc_write_available();
  if ((cdca != 0) && (fifo_fullness(&rxfifo) != 0)) {
    ssize_t n;
    uint8_t *rxd = fifo_get(&rxfifo, cdca, &n);
    tud_cdc_write(rxd, n);
    if (n == cdca) {
      printf("EMERGENCY FLUSH\n");
      tud_cdc_write_flush();
    }
  }
  uint32_t t = time_us_32();
  uint32_t elapsed = (t - last_rxflush);
  if (elapsed > (10 * 1000)) {
    tud_cdc_write_flush();
    last_rxflush = t;
  }
}

// char *td2_boardname()
// {
//   static char r[7];
//   
//   if (r[0] == 0) {
//     union {
//       pico_unique_board_id_t id;
//       uint64_t n;
//     } u;
//     pico_get_unique_board_id(&u.id);
//     uint8_t a = 'A' + (u.n % 26);
//     u.n /= 26;
//     uint8_t b = 'A' + (u.n % 26);
//     u.n /= 26;
//     uint8_t c = 'A' + (u.n % 26);
//     u.n /= 26;
//     uint16_t ddd = (u.n % 1000);
//     sprintf(r, "%c%c%c%03d", a, b, c, ddd);
//   }
//   return r;
// }

static uint32_t id24() {
  union {
    pico_unique_board_id_t id;
    uint64_t n;
  } u;
  pico_get_unique_board_id(&u.id);
  uint32_t rem = u.n % 0xfffffd;  // 16777213 is largest prime below 2^24; use all the bits
  return rem;
}

char *td2_boardname()
{
  static char r[7];
  
  if (r[0] == 0) {
    sprintf(r, "%06x", id24());
  }
  return r;
}

static void fail_hard(void){
    // Minimal: endless loop or watchdog reset; avoid verbose clues
    for(;;){ __asm volatile("wfi"); }
}

static void save_state()
{
  uint32_t offset = (uint32_t)__td2_config_start - XIP_BASE;
  screen.pause = 1;
  while (!screen.paused)
    ;
  if (1) {

    uint8_t *buffer = (uint8_t*)malloc(4096);
    memset(buffer, 0, 4096);
    memcpy(buffer, (uint8_t*)&current_state, sizeof(current_state));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
    // flash_range_program(offset, buffer, 4096);
    restore_interrupts(ints);
    free(buffer);
  }
  screen.pause = 0;
}

static void locked_action(uint32_t action)
{
  switch (action) {

  case 0:
    reset_usb_boot(1<<PICO_DEFAULT_LED_PIN,0); //invokes reset into bootloader mode
    break;

  case 1:
    watchdog_reboot (0, SRAM_END, 10);
    break;

  case 2:
    save_state();
    break;

  case 200:
    panel_freeze();
    break;

  case 201:
    panel_tick();
    break;

  case 202:
    panel_lamp_tx();
    break;

  case 203:
    panel_lamp_rx();
    break;

  case 204:
    panel_hide_cursor();
    break;
  }
}

void td2_action(uint32_t argc, uint32_t *argv)
{
  if ((argc == 2) && (argv[0] == 79218))
    locked_action(argv[1]);
  else if ((argc == 3) && (argv[0] == MAGIC) && (argv[1] == id24()))
    locked_action(argv[2]);

  if ((argc >= 3) && (argv[0] == 1337)) {
    switch (argv[1]) {
      case 4:
        screen.l_dtr = argv[2];
        screen.l_rts = argv[2];
        break;
    }
  }
}

static void load_defaults()
{
  state_t *ps = (state_t*)__td2_config_start;

  if (ps->version == 1000) {
    set_state(ps);
    return;
  }
  static const state_t default_state = {
  1000    , // version;
  115200  , // bit_rate;
  8       , // data_bits;
  0       , // parity;
  0       , // stop_bits;
  };
  set_state(&default_state);
}

int main()
{
  uart_init(uart, 115200);
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
  gpio_init(DTR_PIN);
  gpio_set_dir(DTR_PIN, GPIO_OUT);
  gpio_init(RTS_PIN);
  gpio_set_dir(RTS_PIN, GPIO_OUT);

  gpio_put(DTR_PIN, 0);
  gpio_put(RTS_PIN, 1);

  panel_init();

  stdio_init_all();

  load_defaults();

  tusb_init();

  if (uart_is_readable(uart)) {
    uart_getc(uart);
  }

  while (1) {
    tud_task();
    rx_poll();
    // CDC drivers use linestate as a bodge to activate/deactivate the interface
    // if (tud_cdc_get_line_state() == 0) {
    //   // strcpy(screen.mode, "ready");
    //   lamps_off();
    //   actual_dtr = false;
    //   actual_rts = false;
    // }
  }

  return 0;
}
