#ifndef __COMMON_H__
#define __COMMON_H__

enum { EU868_FREQ_MIN = 863000000,
       EU868_FREQ_MAX = 870000000};

/* Default settings are SF12, BW125, CR 4/5, explicit header, crc */
#define DEFAULT_RPS MAKERPS(SF12, BW125, CR_4_5, 0, 0)
/* transmit at 17 dBm by default */
#define DEFAULT_TXPOWER 17
/* default transmit/receive frequency is 868.000 MHz */
#define DEFAULT_FREQ 868000000u

static const u1_t* sf_names[] = {
  [FSK]   = (const u1_t*)"FSK",
  [SF12]  = (const u1_t*)"SF12",
  [SF11]  = (const u1_t*)"SF11",
  [SF10]  = (const u1_t*)"SF10",
  [SF9]   = (const u1_t*)"SF9",
  [SF8]   = (const u1_t*)"SF8",
  [SF7]   = (const u1_t*)"SF7",
  [SFrfu] = (const u1_t*)"SFrfu",
};

static const u1_t* bw_names[] = {
  [BW125] = (const u1_t*)"BW125",
  [BW250] = (const u1_t*)"BW250",
  [BW500] = (const u1_t*)"BW500",
  [BWrfu] = (const u1_t*)"BWrfu",
};

// Precalculated symbol times in osticks
static const ostime_t SYMBOLTIME[7][3] = {
  // ------------bw----------             
  // 125kHz    250kHz    500kHz           
  {       0,        0,        0 },  // FSK 
  {      34,       17,        8 },  // SF7 
  {      67,       34,       17 },  // SF8 
  {     134,       67,       34 },  // SF9 
  {     268,      134,       67 },  // SF10
  {     537,      268,      134 },  // SF11
  {    1074,      537,      268 },  // SF12
};

/* Array of SF/BW settings to go through */
typedef struct _hop_t {
  sf_t sf;
  bw_t bw;
} hop_t;

/* R_b = SF * 1/[2^SF/BW] bits/sec
   higher SF decreases bitrate, increases sensitivity
   smaller BW decreases bitrate, increases sensitivity
   hops are sorted on raw bitrate, from small to large, decreasing sensitivity with each hop (and range)
 */
hop_t hops[] = {
  {SF12, BW125}, // [0]  366 b/s

  {SF11, BW125}, // [1]  671 b/s
  {SF12, BW250}, // [2]  732 b/s

  {SF10, BW125}, // [3]  1221 b/s
  {SF11, BW250}, // [4]  1343 b/s
  {SF12, BW500}, // [5]  1465 b/s

  {SF9 , BW125}, // [6]  2197 b/s
  {SF10, BW250}, // [7]  2441 b/s
  {SF11, BW500}, // [8]  2686 b/s

  {SF8 , BW125}, // [9]  3906 b/s
  {SF9 , BW250}, // [10] 4395 b/s
  {SF10, BW500}, // [11] 4883 b/s

  {SF7 , BW125}, // [12] 6836 b/s
  {SF8 , BW250}, // [13] 7813 b/s
  {SF9 , BW500}, // [14] 8789 b/s

  {SF7 , BW250}, // [15] 13672 b/s
  {SF8 , BW500}, // [16] 15625 b/s

  {SF7 , BW500}, // [17] 27344 b/s
};

#endif /* end of include guard: __COMMON_H__ */
