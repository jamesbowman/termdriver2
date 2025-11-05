#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "drawscreen.h"

extern screen_t screen;
extern uint32_t screen_y;

char *td2_boardname();
void td2_action(uint32_t argc, uint32_t *argv);
void send_to_host(uint8_t *s, size_t n);

#define MAXARGS 80

static uint32_t cx, cy;   // Cursor x, y
static uint32_t fg, bg;   // foreground, background as RGB-444
static enum {PLAIN, ESC, BODY} estate;
static uint32_t args[MAXARGS], nargs;

static const uint16_t c256[256] = {
#include "c256.i"
};

uint32_t my_id(void)
{
  return (uint32_t)strtol(td2_boardname(), NULL, 16);
}

void text_cls()
{
  uint32_t bl = (bg << 20) | (fg << 8) | ' ';
  for (size_t i = 0; i < (screen.cols * 32); i++)
    screen.s[i] = bl;
}

static void sgr_0(void)
{
  fg = 0xfff;
  bg = 0x000;
}

void text_init()
{
  screen.cols = 40;
  screen.rows = 22;
  cx = cy = 0;
  sgr_0();
  screen_y = 0;
  text_cls();
  estate = PLAIN;
  screen.cols = 40;
}

static inline uint32_t xf(uint32_t y)
{
  return ((screen_y + y) & 0x1f);
}

static inline void fill_u32(uint32_t * __restrict dst, uint32_t v, size_t n) {
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {             // unrolled
        dst[i+0]=v; dst[i+1]=v; dst[i+2]=v; dst[i+3]=v;
        dst[i+4]=v; dst[i+5]=v; dst[i+6]=v; dst[i+7]=v;
    }
    for (; i < n; ++i) dst[i]=v;
}

static void clrline(uint32_t y)
{
  uint32_t bl = (bg << 20) | (fg << 8) | 0x20;
  uint32_t *pd = &screen.s[xf(y) * screen.cols];

  // for (size_t i = 0; i < (1 * screen.cols); i++) *pd++ = bl;
  fill_u32(pd, bl, screen.cols);
}

void down1()
{
  if (cy != (screen.rows - 1)) {
    cy++;
  } else {
    clrline(cy + 1);
    screen_y++;
  }
}

// CSI handlers -----------------------------------------------

#define DEFAULT 0

static void arg_report(void) __attribute__((unused));

static void arg_report(void)
{
  printf("%d args: ", nargs);
  for (uint32_t i = 0; i < nargs; i++)
    printf("%d ", args[i]);
  printf("\n");
}

static void default_missing(uint32_t x)
{
  for (uint32_t i = 0; i < nargs; i++)
    if (args[i] == DEFAULT)
      args[i] = x;
}

void csi_m()
{
  // arg_report();
  if (nargs == 0) {
    sgr_0();
    return;
  }
  uint32_t na = nargs, *a = args;
  uint32_t consumed = 0;

  while (na > 0) {
    // printf("na=%d a[0]=%d\n", na, a[0]);
    switch (a[0]) {
    case 0:
      sgr_0();
      consumed = 1;
      break;

    case 1:
      // bold
      consumed = 1;
      break;

    case 8:
      fg = bg;
      consumed = 1;
      break;

    case 30 ... 37:
      fg = c256[a[0] - 30];
      consumed = 1;
      break;

    case 38:
      switch (a[1]) {
      case 5:
        fg = c256[a[2]];
        consumed = 3;
        break;
      case 2:
        fg = ((a[2] >> 4) << 8) | ((a[3] >> 4) << 4) | (a[4] >> 4);
        consumed = 5;
        break;
      }
      break;

    case 39:
      fg = 0xfff;
      consumed = 1;
      break;

    case 40 ... 47:
        bg = c256[a[0] - 40];
        consumed = 1;
        break;

    case 48:
      switch (a[1]) {
      case 5:
        bg = c256[a[2]];
        consumed = 3;
        break;
      case 2:
        bg = ((a[2] >> 4) << 8) | ((a[3] >> 4) << 4) | (a[4] >> 4);
        consumed = 5;
        break;
      }
      break;

    case 49:
      bg = 0x000;
      consumed = 1;
      break;

    case 90 ... 97:
      fg = c256[8 + (a[0] - 90)];
      consumed = 1;
      break;

    case 100 ... 107:
      bg = c256[8 + (a[0] - 100)];
      consumed = 1;
      break;

    default:
      printf("Unhandled SGR %d\n", a[0]);
      consumed = 1;
      break;
    }
    assert(consumed != 0);
    na -= consumed;
    a += consumed;
    assert(na >= 0);
  }
  // assert(0);
}

void csi_n()
{
  char msg[160] = "";

  if (nargs == 1) {
    switch (args[0]) {
    case 6:
      snprintf(msg, sizeof(msg), "\033[%u;%uR", cy + 1, cx + 1);
      break;
    case 7:
      strcpy(msg, td2_boardname());
      break;
    }
  }

  if (strlen(msg) != 0)
    send_to_host((uint8_t*)msg, strlen(msg));
}

void csi_G()
{
  default_missing(1);
  switch (nargs) {
  case 0:
    cx = 0;
    break;
  default:
    cx = args[0] - 1;
    break;
  }
}

void csi_H()
{
  default_missing(1);
  switch (nargs) {
  case 0:
    cx = cy = 0;
    break;
  case 1:
    cy = args[0] - 1;
    cx = 0;
    break;
  default:
    cy = args[0] - 1;
    cx = args[1] - 1;
    break;
  }
}

void csi_J()
{
  if (nargs == 0)
    return;
  switch (args[0]) {
  case 2:
    text_cls();
    cx = cy = 0;  // match behavior of DOS ANSI.SYS
    break;
  default:
    return;
  }
}

void csi_l()
{
  td2_action(nargs, args);
}

static uint32_t distance(void)
{
  uint32_t n;
  if (nargs == 0)
    n = 1;
  else if (args[0] == DEFAULT)
    n = 1;
  else
    n = args[0];
  return n;
}

void csi_A()
{
  cy -= distance();
  if (cy > screen.rows)
    cy = 0;
}

void csi_B()
{
  cy += distance();
  if (cy > (screen.rows - 1))
    cy = screen.rows - 1;
}

void csi_C()
{
  cx += distance();
  if (cx > (screen.cols - 1))
    cx = screen.cols - 1;
}

void csi_D()
{
  cx -= distance();
  if (cx > screen.cols)
    cx = 0;
}

void csi_K()
{
  default_missing(0);
  uint32_t bl = (bg << 20) | (fg << 8) | ' ';
  if ((nargs == 0) || (args[0] == 0)) {
    for (size_t i = cx; i < screen.cols; i++)
      screen.s[((cy + screen_y) & 0x1f) * screen.cols + i] = bl;
  }
  return;
}

void text_ch(uint8_t ch)
{
  // printf("text_ch(%02x) estate=%d\n", ch, (int)estate);
  if (estate == PLAIN) {
    switch (ch) {
    case '\r':
      cx = 0;
      break;

    case 0x7:
      break;

    case '\b':
      if (cx > 0)
        --cx;
      break;

    case '\n':
      down1();
      break;

    case 27:
      estate = ESC;
      break;

    default:
      if ((ch < 32) || (127 <= ch)) {
        // printf("Illegal %#x\n", ch);
        ch = '?';
      }
      screen.s[((cy + screen_y) & 0x1f) * screen.cols + cx] = (bg << 20) | (fg << 8) | ch;
      if (++cx == screen.cols) {
        cx = 0;
        down1();
      }
    }
    return;
  }

  if (estate == ESC) {
    if (ch == '[') {
      estate = BODY;
      nargs = 0;
      memset(args, 0, sizeof(args));
    } else
      estate = PLAIN;
    return;
  }

  if (estate == BODY) {
    switch (ch) {
    case 0x20 ... 0x3f:
      switch (ch) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          args[nargs] = (args[nargs] * 10) + (ch - '0');
          break;
        case ';':
          if (nargs < (MAXARGS - 1))
            nargs++;
          break;
        default:
          break;
      }
      break;

#define CSI(f) /*printf("Handle %c\n", ch); arg_report();*/ nargs++; f(); estate = PLAIN; break;
    case 'A': CSI(csi_A);
    case 'B': CSI(csi_B);
    case 'C': CSI(csi_C);
    case 'D': CSI(csi_D);
    case 'G': CSI(csi_G);
    case 'H': CSI(csi_H);
    case 'J': CSI(csi_J);
    case 'K': CSI(csi_K);
    case 'l': CSI(csi_l);
    case 'm': CSI(csi_m);
    case 'n': CSI(csi_n);
    default:
      // printf("Unhandled CSI code '%c'\n", ch);
      nargs++;
      estate = PLAIN;
    }
  }
}

void text_str(const uint8_t *s)
{
  // printf("text_str(%s)\n", s);
  const uint8_t *pc;
  for (pc = s; *pc; pc++)
    text_ch(*pc);
  // printf("cursor (%d,%d) screen_y %x\n\n", cx, cy, screen_y);
}

static uint32_t under_cursor;

void hide_cursor(void)
{
  if (screen.cursor) {
    uint32_t *pc = &screen.s[((cy + screen_y) & 0x1f) * screen.cols + cx];
    *pc = under_cursor;
  }
}

void show_cursor(void)
{
  if (screen.cursor) {
    uint32_t *pc = &screen.s[((cy + screen_y) & 0x1f) * screen.cols + cx];
    under_cursor = *pc;
    *pc &= 0x000fffff;
    *pc |= 0xf0000000;
  }
}

void remap(bool dtr, bool rts, bool *m_dtr, bool *m_rts)
{
  unsigned idx = (dtr << 1) | rts;
  *m_dtr = (screen.l_dtr >> idx) & 1;
  *m_rts = (screen.l_rts >> idx) & 1;
}
