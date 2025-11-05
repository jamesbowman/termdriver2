#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "tusb.h"

#include "hardware/spi.h"
#include "hardware/dma.h"

#define MANUFACTURING 0

#include "../gen/guide240.i"

#include "drawscreen.h"
#include "text.h"
#include "state.h"

#define spi spi1

#define USE_DMA 1

// #define PIN_MISO 13
#define PIN_SCK  14
#define PIN_MOSI 15
#define PIN_RES  11    // panel reset
#define PIN_DC   10    // panel Command/Data

#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID 0x04
#define ST7789_RDDST 0x09

#define ST7789_SLPIN 0x10
#define ST7789_SLPOUT 0x11
#define ST7789_PTLON 0x12
#define ST7789_NORON 0x13

#define ST7789_INVOFF 0x20
#define ST7789_INVON 0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON 0x29

#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_RAMRD 0x2E

#define ST7789_PTLAR 0x30
#define ST7789_SCRLAR 0x33
#define ST7789_MADCTL 0x36
#define ST7789_VSCSAD 0x37
#define ST7789_COLMOD 0x3A

#define ST7789_GSCAN 0x45
#define ST7789_WRDISBV 0x51

#define ST7789_FRMCTR1 0xB1
#define ST7789_FRMCTR2 0xB2
#define ST7789_FRMCTR3 0xB3
#define ST7789_INVCTR 0xB4
#define ST7789_DISSET5 0xB6

#define ST7789_GCTRL 0xB7
#define ST7789_GTADJ 0xB8
#define ST7789_VCOMS 0xBB

#define ST7789_LCMCTRL 0xC0
#define ST7789_IDSET 0xC1
#define ST7789_VDVVRHEN 0xC2
#define ST7789_VRHS 0xC3
#define ST7789_VDVS 0xC4
#define ST7789_VMCTR1 0xC5
#define ST7789_FRCTRL2 0xC6
#define ST7789_CABCCTRL 0xC7

#define ST7789_RDID1 0xDA
#define ST7789_RDID2 0xDB
#define ST7789_RDID3 0xDC
#define ST7789_RDID4 0xDD

#define ST7789_GMCTRP1 0xE0
#define ST7789_GMCTRN1 0xE1

#define ST7789_PWCTR6 0xFC

static void command(uint8_t x)
{
  gpio_put(PIN_DC, 0);
  spi_write_blocking(spi, &x, 1);
}

static void data1(uint8_t x)
{
  gpio_put(PIN_DC, 1);
  spi_write_blocking(spi, &x, 1);
}

static void datan(size_t n, const uint8_t *p)
{
  gpio_put(PIN_DC, 1);
  spi_write_blocking(spi, p, n);
}

static void set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  command(ST7789_CASET);
  {
    uint8_t d[4] = {
      0, x0, 0, x1
    };
    datan(sizeof(d), d);
  }
  command(ST7789_RASET);
  {
    uint8_t d[4] = {
      0, y0, 0, y1
    };
    datan(sizeof(d), d);
  }
  command(ST7789_RAMWR);
}

static void cls()
{
  set_window(0, 0, 239, 239);
  for (int i = 0; i < (240 * 240 * 12 / 8); i++)
    data1(0);
}

static void drawch(int x, int y, uint8_t ch)
{
  extern const uint8_t mainfont[], mainfont_decode[];
  uint32_t w = mainfont[0], h = mainfont[1];
  uint32_t bpl = (w * 3) / 2;
  uint32_t bpc = bpl * h;
  size_t ix = mainfont_decode[ch];

  set_window(x, y, x + w - 1, y + h - 1);
  const uint8_t *src = mainfont + 2 + (bpc * ix);
  for (uint32_t i = 0; i < (h * bpl); i++)
    data1(*src++);
}

static void show_code()
{
  extern char *td2_boardname();
  const int w = 16, n = 6;
  char *nm = td2_boardname();
  for (int i = 0; i < n; i++) {
    int x = (240 - w * n) / 2 + (w * i);
    drawch(x, 5, nm[i]);
  }
}

static void show_splash()
{
  int y = 180;
  set_window(0, y, GUIDE_W - 1, y + GUIDE_H - 1);
  for (int i = 0; i < sizeof(guide_240); i++)
    data1(guide_240[i]);

  show_code();
}

volatile screen_t screen;
uint32_t screen_y;

static uint8_t linebuf[2][240 * 12 / 8];

#if USE_DMA
uint dma_tx;
dma_channel_config c;
#endif

static bool actual_dtr, actual_rts;

static void lamps_off(void)
{
  screen.lamps[LAMP_TX] = 0;
  screen.lamps[LAMP_RX] = 0;
  screen.lamps[LAMP_DTR] = 0;
  screen.lamps[LAMP_RTS] = 0;
}

static void lamps_age(void)
{
  inclamp(LAMP_TX, -15);
  inclamp(LAMP_RX, -15);
  inclamp(LAMP_DTR, actual_dtr ? 50 : -25);
  inclamp(LAMP_RTS, actual_rts ? 50 : -25);
}


static void render()
{
#if USE_DMA
  dma_channel_configure(dma_tx, &c,
                      &spi_get_hw(spi)->dr, // write address
                      linebuf[0], // read address
                      sizeof(linebuf[0]), // element count (each element is of size transfer_data_size)
                      false); // don't start yet
#endif

  set_window(0, 0, 239, 239);
  gpio_put(PIN_DC, 1);
  screen.y = screen_y;

  if (1) {
    for (int y = 0; y < 240; y++) {
      int w = (y & 1);
      line1(linebuf[w], y);
#if !USE_DMA
      datan(sizeof(linebuf[0]), linebuf[w]);
#else
      dma_channel_wait_for_finish_blocking(dma_tx);
      dma_channel_transfer_from_buffer_now(dma_tx, linebuf[w], sizeof(linebuf[0]));
#endif
    }
  }
#if USE_DMA
  dma_channel_wait_for_finish_blocking(dma_tx);
#endif
}

static void spi_in()
{
  // gpio_set_dir(PIN_MISO, GPIO_IN);
  // gpio_set_dir(PIN_MISO, GPIO_IN);
  gpio_set_dir(PIN_SCK, GPIO_OUT);
  // gpio_set_function(PIN_MISO, GPIO_FUNC_SIO);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SIO);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SIO);
}

static void spi_out()
{
  // gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
}

static uint32_t gscan()
{
  command(ST7789_GSCAN);
  gpio_put(PIN_DC, 1);

  spi_in();

#if 0
  uint8_t dd[256];
  memset(dd, 0x55, sizeof(dd));
  spi_read_blocking(spi, 0xff, dd, 10);
  for (int i = 0; i < 20; i++)
    printf("%02x\n", dd[i]);
#else
  static uint32_t mx;
  uint32_t v = 0;
  for (int i = 0; i < 16; i++) {
    gpio_put(PIN_SCK, 1);
    gpio_put(PIN_SCK, 0);
    // printf("%2d: %d %d\n", i, gpio_get(PIN_MISO), gpio_get(PIN_MOSI));
    sleep_us(1);
    asm volatile("nop \n nop \n nop");
    asm volatile("nop \n nop \n nop");
    asm volatile("nop \n nop \n nop");
    // printf("%d ", gpio_get(PIN_MOSI));
    v = (v << 1) | gpio_get(PIN_MOSI);
  }
  if (v > mx)
    mx = v;
  // printf("%10d: %6x %6x\n", time_us_32(), v, mx);
#endif
  spi_out();
  // printf("[%d] ", v);
  return v;
}

static void panel_refresh(void)
{
  while (screen.traffic == 0)
    ;
  while (1) {
    uint32_t t0 = time_us_32();

    render();
    if (!screen.freeze) {
      lamps_age();
    }

    uint32_t t1 = time_us_32();
    // printf("Took %u us\n", t1 - t0);

    screen.paused = 0;
    while (screen.pause)
      screen.paused = 1;
    continue;
    sleep_us(1);

    while (gscan() != 332)
      ;
    while (gscan() != 333)
      ;

  }
}

void panel_init()
{
#if USE_DMA
  // Grab some unused dma channels
  dma_tx = dma_claim_unused_channel(true);
  c = dma_channel_get_default_config(dma_tx);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_dreq(&c, spi_get_dreq(spi, true));
#endif

  gpio_init(PIN_RES);
  gpio_set_dir(PIN_RES, GPIO_OUT);
  gpio_init(PIN_DC);
  gpio_set_dir(PIN_DC, GPIO_OUT);

  // Enable SPI 0 at 1 MHz and connect to GPIOs
  spi_init(spi, 62500000);

  spi_set_format(spi, 8, SPI_CPHA_1, SPI_CPOL_1, SPI_MSB_FIRST);
  // gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

  gpio_put(PIN_RES, 1);
  gpio_put(PIN_RES, 0);
  sleep_ms(10);
  gpio_put(PIN_RES, 1);
  sleep_ms(50);

  // command(ST7789_SWRESET);
  // sleep_ms(150);

  command(ST7789_MADCTL);
  data1(0x0);
  command(ST7789_FRMCTR2);
  data1(0xc);
  data1(0xc);
  data1(0x0);
  data1(0x33);
  data1(0x33);
  command(ST7789_COLMOD);
  data1(0x3);
  command(ST7789_GCTRL);
  data1(0x14);
  command(ST7789_VCOMS);
  data1(0x37);
  command(ST7789_LCMCTRL);
  data1(0x2c);
  command(ST7789_VDVVRHEN);
  data1(0x1);
  command(ST7789_VRHS);
  data1(0x12);
  command(ST7789_VDVS);
  data1(0x20);
  command(0xD0);
  data1(0xa4);
  data1(0xa1);
  command(ST7789_FRCTRL2);
  data1(0xf);
  command(ST7789_GMCTRP1);
  {
    static const uint8_t d[14] = {
  0xd0,0x4,0xd,0x11,0x13,0x2b,0x3f,0x54,0x4c,0x18,0xd,0xb,0x1f,0x23
    };
    datan(sizeof(d), d);
  }
  command(ST7789_GMCTRN1);
  {
    static const uint8_t d[14] = {
  0xd0,0x4,0xc,0x11,0x13,0x2c,0x3f,0x44,0x51,0x2f,0x1f,0x1f,0x20,0x23
    };
    datan(sizeof(d), d);
  }
  command(ST7789_INVON);
  command(ST7789_SLPOUT);
  cls();
  show_splash();
  command(ST7789_DISPON);
  // sleep_ms(100);

  screen.freeze = 0;
  screen.cursor = 1;
  screen.l_dtr = 0xc;
  screen.l_rts = 0xa;

  lamps_off();
  strcpy((char*)screen.mode, "ready");
  text_init();
  show_cursor();

#if !MANUFACTURING
  multicore_launch_core1(panel_refresh);
#endif
}

void panel_line_coding(state_t const* line_coding)
{
  printf("CALLBACK tud_cdc_line_coding_cb\n");
  if (1 || tud_cdc_connected()) {
    // uint32_t bit_rate;
    // uint8_t  stop_bits; ///< 0: 1 stop bit - 1: 1.5 stop bits - 2: 2 stop bits
    // uint8_t  parity;    ///< 0: None - 1: Odd - 2: Even - 3: Mark - 4: Space
    // uint8_t  data_bits; ///< can be 5, 6, 7, 8 or 16

    uint32_t baud = line_coding->bit_rate;

    // Kludge.
    // Want to activate monitor on connection, but Linux sets up all terminals
    // on connection to 9600. So don't count a 9600 baud setting as "traffic"
    if (baud != 9600)
      screen.traffic = 1;

    if (baud < 10000)
      sprintf((char*)screen.mode, "%d", baud);
    else {
      uint32_t
        u = baud % 1000,
        t = (baud / 1000) % 1000,
        m = (baud / 1000000);
      if (baud < 1000000)
        sprintf((char*)screen.mode, "%u,%03u", t, u);
      else if ((u | t) == 0)
        sprintf((char*)screen.mode, "%uM", m);
      else
        sprintf((char*)screen.mode, "%u,%03u,%03u", m, t, u);
    }

    static const char* stops[] = {"1", "1%", "2"};

    char buf[5];
    sprintf(buf, " %d%c%s",
      line_coding->data_bits,
      "NOEMS"[line_coding->parity],
      stops[line_coding->stop_bits]);
    strcat((char*)screen.mode, buf);

    printf("%d %d%c%s\n",
      line_coding->bit_rate,
      line_coding->data_bits,
      "NOEMS"[line_coding->parity],
      stops[line_coding->stop_bits]);
 }
}

// draw text on the panel
// dir:'t' LAMP_TX, 'r' LAMP_RX
void panel_text(const uint8_t *tx_buf, size_t n, int dir)
{
  screen.traffic = 1;
  hide_cursor();
  for (size_t i = 0; i < n; i++) {
    text_ch(tx_buf[i]);
  }
  show_cursor();

  if (!screen.freeze) {
    int lamp;
    if (dir == 't')
      lamp = LAMP_TX;
    else
      lamp = LAMP_RX;
    inclamp(lamp, n * LAMP_ATTACK);
  }
}

////////////////////////////////////////////////////////

void panel_dtr_rts(bool dtr, bool rts)
{
  actual_dtr = dtr;
  actual_rts = rts;
}

void panel_freeze(void)
{
  screen.freeze = 1;
  lamps_off();
}

void panel_tick(void)
{
  lamps_age();
}

void panel_lamp_tx(void)
{
  inclamp(LAMP_TX, LAMP_ATTACK);
}

void panel_lamp_rx(void)
{
  inclamp(LAMP_RX, LAMP_ATTACK);
}

void panel_hide_cursor(void)
{
  screen.cursor = 0;
}
