#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "drawscreen.h"
#include "../gen/microfont.i"

extern screen_t screen;

//          R G B
// 0  g0      0
// 4  r0    0
// 8  r1    1
// 12 b0        0
// 16 b1        1
// 20 g1      1
// 24 g2      2
// 28 r2    2
// 32 r3    3
// 36 b2        2
// 40 b3        3
// 44 g3      3
//

void line_text_4x6(uint8_t *dst, int y)
{
  uint32_t ymod = (y & 7);
  uint32_t *psrc = screen.s + (COLS * (((y >> 3) + screen.y) & 0x1f));
  uint16_t *dp = (uint16_t *)dst;

#define N(c, i)  ((c >> (4 * i)) & 0xf)
#define CH(c, i)  ((c >> (4 + 8 * i)) & 0xf)
#define PACK16(a, b, c, d) (a | (b << 4) | (c << 8) | (d << 12))

  switch (ymod) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6: {
      uint32_t coff = ((ymod - 1) * 4);
      for (int x = 0; x < COLS; x++) {
        uint32_t el = psrc[x];
        uint32_t fg = (el >> 8);
        uint32_t bg = (el >> 20);
        uint32_t m = *(uint32_t*)(microfont[el & 0xff] + coff);
        uint32_t mi = 0x0f0f0f0f - m;

        uint32_t r, g, b;
        r = m * N(fg, 2) + mi * N(bg, 2);
        g = m * N(fg, 1) + mi * N(bg, 1);
        b = m * N(fg, 0) + mi * N(bg, 0);

        *dp++ = PACK16(CH(g, 0), CH(r, 0), CH(r, 1), CH(b, 0));
        *dp++ = PACK16(CH(b, 1), CH(g, 1), CH(g, 2), CH(r, 2));
        *dp++ = PACK16(CH(r, 3), CH(b, 2), CH(b, 3), CH(g, 3));
      }
    }
    break;

  default:

    for (int x = 0; x < COLS; x++) {
      uint32_t el = psrc[x];
      uint32_t bg = (el >> 20);

      uint32_t r, g, b;
      const uint32_t mi = 0x0f0f0f0f;
      r = mi * N(bg, 2);
      g = mi * N(bg, 1);
      b = mi * N(bg, 0);

      *dp++ = PACK16(CH(g, 0), CH(r, 0), CH(r, 1), CH(b, 0));
      *dp++ = PACK16(CH(b, 1), CH(g, 1), CH(g, 2), CH(r, 2));
      *dp++ = PACK16(CH(r, 3), CH(b, 2), CH(b, 3), CH(g, 3));
    }
  }
}


