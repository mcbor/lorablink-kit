#ifndef _enzo_h_
#define _enzo_h_

#include "osenzo.h"
#include "enzobase.h"

// ENZO version
#define ENZO_VERSION_MAJOR 1
#define ENZO_VERSION_MINOR 0
#define ENZO_VERSION_BUILD 20151128

// Radio states
enum { RADIO_RST=0, RADIO_TX=1, RADIO_RX=2, RADIO_RXON=3, RADIO_CAD=4 };

struct enzo_t {
  // Radio settings TX/RX (also accessed by HAL)
  ostime_t   txend;                       // ticks - transmission completed
  ostime_t   rxtime;                      // ticks - reception completed
  u4_t       freq;                        // Operating frequency
  s1_t       rssi;                        // RSSI register value of received frame
  s1_t       snr;                         // SNR register value of received frame
  u1_t       irq_flags;                   // IRQ flags
  u1_t       crcerr;                      // CRC error (0=no error, 1=CRC error)
  u1_t       validHeader;                 // Valid header received (0 = no error, 1 = invalid header)
  u1_t       cad;                         // CAD detected (0=no carrier detected, 1=carrier detected)
  rps_t      rps;                         // Radio Parameter Set of Bandwidth/Spreading Factor/Coding Rate
  u1_t       rxsyms;                      // RX timeout in symbols
  s1_t       txpow;                       // TX output power in dBm
  osjob_t    osjob;                       // callback for handled IRQs
  u1_t       dataLen;                     // 0 no data or zero length data, >0 byte cout of data
  u1_t       frame[MAX_LEN_FRAME];        // frame (RX or TX)
};
DECLARE_ENZO;

void ENZO_init      (void);
void ENZO_reset     (void);

#endif // _enzo_h_
