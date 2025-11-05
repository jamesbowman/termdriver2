typedef struct {
  // subset of fields from cdc_line_coding_t
  uint32_t version;
  uint32_t bit_rate;
  uint32_t data_bits;
  uint32_t parity;
  uint32_t stop_bits;
} state_t;

