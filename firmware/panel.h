#include "state.h"

void panel_init();
void panel_line_coding(state_t const* line_coding);
void panel_dtr_rts(bool dtr, bool rts);
void panel_text(const uint8_t *tx_buf, size_t n, int dir);
void panel_freeze(void);
void panel_tick(void);
void panel_lamp_tx(void);
void panel_lamp_rx(void);
void panel_hide_cursor(void);
