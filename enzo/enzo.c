#include "enzo.h"

DEFINE_ENZO;

extern inline sf_t  getSf    (rps_t params);
extern inline rps_t setSf    (rps_t params, sf_t sf);
extern inline bw_t  getBw    (rps_t params);
extern inline rps_t setBw    (rps_t params, bw_t cr);
extern inline cr_t  getCr    (rps_t params);
extern inline rps_t setCr    (rps_t params, cr_t cr);
extern inline int   getNocrc (rps_t params);
extern inline rps_t setNocrc (rps_t params, int nocrc);
extern inline int   getIh    (rps_t params);
extern inline rps_t setIh    (rps_t params, int ih);
extern inline rps_t makeRps  (sf_t sf, bw_t bw, cr_t cr, int ih, int nocrc);
extern inline int   sameSfBw (rps_t r1, rps_t r2);


void ENZO_init() {
  os_clearCallback(&ENZO.osjob);
  os_clearMem((xref2u1_t)&ENZO, SIZEOFEXPR(ENZO));
}

void ENZO_reset() {
  os_radio(RADIO_RST);
  os_clearCallback(&ENZO.osjob);
  os_clearMem((xref2u1_t)&ENZO, SIZEOFEXPR(ENZO));

  // SF7, 125 kHz, CR 4/5, explicit header, CRC
  ENZO.rps = makeRps(SF7, BW125, CR_4_5, 0, 0);
  ENZO.freq       = 868000000; // Hz
  ENZO.txpow      = 2;         // dBm
}
