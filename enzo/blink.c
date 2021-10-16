/*
 * blink
 * Simple protocol prototype for data collection in a LoRa sensor network
 *
 * @author: Martin Bor <m.bor@lancaster.ac.uk>
 * @date: 2015-11-05  
 *
 */

#include <stdlib.h>
#include "enzo.h"
#include "blink.h"
#include "blink-common.h"
#include "debug.h"
// #include "queue.h"

struct blink_t BLINK;

/* fwd decl */
static void _sync_cb(osjob_t *job);
static void _wakeup(osjob_t *job);
static void _wakeup_root(osjob_t *job);
static void _beacon_tx(osjob_t *job);
static void _beacon_rx(osjob_t *job);
static void _data_tx(osjob_t *job);
static void _data_rx(osjob_t *job);
static void _rx_done(osjob_t *job);
static void _rx_root_done(osjob_t *job);
static void _rx_beacon_done(osjob_t *job);
static void _rx_data_done(osjob_t *job);
static void _tx_done(osjob_t *job);
static void _tx_beacon_done(osjob_t *job);
static void _tx_data_done(osjob_t *job);
static void _cad_done(osjob_t *job);

// utils
static inline void _next_slot(void);
static inline u1_t _is_beacon_slot(void);
static inline u1_t _is_data_slot(void);
static void        _report_event(event_t ev);
static void        _missed_beacon(void);
static void        _rebroadcast_beacon(beacon_msg_t *b);
static void        _set_radio_callback(osjobcb_t callback);

// job decl
static osjob_t _root_job;
static osjob_t _sync_job;
static osjob_t _wakeup_job;
static osjob_t _transmit_job;
static osjob_t _receive_job;

// messages queues
static beacon_msg_t beacon_tx;
static data_msg_t   data_msg_tx;
static data_msg_t   data_msg_rx;

static u1_t cad_counter = CAD_CHECKS;

#define debug_fun() do {\
  debug_char('>'); \
  debug_char(' '); \
  debug_str((const u1_t*)__func__); \
  debug_char('\r'); \
  debug_char('\n');\
} while(0)
#define debug(x) do { debug_str(x "\r\n"); } while(0)
void debug_opmode() {
  debug_char('[');
  for(u1_t i = 0; i < SIZEOFEXPR(BLINK.opmode) * 8; i++) {
    switch(BLINK.opmode & (1 << i)) {
       case OP_NONE:
         debug_char('.');
         break;
       case OP_READY:
         debug_char('r');
         break;
       case OP_SCAN:   
         debug_char('s');
         break;
       case OP_TRACK:  
         debug_char('t');
         break;
       case OP_TXBCN:  
         debug_char('B');
         break;
       case OP_TXDATA: 
         debug_char('D');
         break;
       case OP_RXBCN:  
         debug_char('b');
         break;
       case OP_RXDATA: 
         debug_char('d');
         break;
      case OP_ROOT:
         debug_char('0');
         break;
      case OP_NODE:
         debug_char('n');
         break;
       default:
         debug_char('?');
    }
  }
  debug_char(']');
  debug_char('\r');
  debug_char('\n');
}

void blink_init(void) {
  debug_fun();
  os_clearMem((xref2u1_t)&BLINK, SIZEOFEXPR(BLINK));
  os_clearMem((xref2u1_t)&beacon_tx, SIZEOFEXPR(beacon_msg_t));
  os_clearMem((xref2u1_t)&data_msg_tx, SIZEOFEXPR(data_msg_t));
  os_clearMem((xref2u1_t)&data_msg_rx, SIZEOFEXPR(data_msg_t));
}

void blink_reset(void) {
  debug_fun();
  os_clearCallback(&ENZO.osjob);
  os_radio(RADIO_RST);

  // Init ENZO struct
  os_clearCallback(&ENZO.osjob);
  os_clearMem((xref2u1_t)&ENZO, SIZEOFEXPR(ENZO));

  ENZO.rps   = DEFAULT_RPS;
  ENZO.freq  = DEFAULT_FREQ;
  ENZO.txpow = DEFAULT_TXPOWER;

  if(BLINK.nodeid == ROOT_ID) {
    // we're special
    BLINK.hop    = 0;
    BLINK.opmode |= OP_ROOT;
  } else {
    // max hop distance
    BLINK.hop    = 0xff;
    BLINK.opmode |= OP_NODE;
  }
  BLINK.slot = TIME_SLOTS;
  BLINK.opmode |= OP_READY;
}

void blink_start_sync(void) {
  debug_fun(); debug_opmode();
  ASSERT(BLINK.opmode & OP_READY);

  if(BLINK.opmode & OP_ROOT) {
    // we're root, start beaconing
    os_setCallback(&_root_job, FUNC_ADDR(_wakeup_root));
  } else {
    BLINK.opmode |= OP_SCAN;
    os_clearCallback(&ENZO.osjob);
    ENZO.osjob.func = FUNC_ADDR(_sync_cb);
    os_radio(RADIO_RXON);
    debug_led(1);
  }
}

void blink_tx(u1_t *buffer, size_t n) {
  debug_fun(); debug_opmode();
  ASSERT(BLINK.opmode & (OP_READY|OP_TRACK));

  // TODO check if we aren't transmitting in the meantime
  if(n > MAX_LEN_PAYLOAD) {
    // can't transmit anything that's too big
    return;
  }
  // copy payload in data tx buffer (possibly overwriting a pending message)
  // TODO replace with tx queue
  os_clearMem((xref2u1_t)&data_msg_tx, SIZEOFEXPR(data_msg_t));
  data_msg_tx.header.type = DATA;
  data_msg_tx.header.dest = DEST_ROOT;
  data_msg_tx.header.hop  = BLINK.hop;
  data_msg_tx.footer.trace = (TRACE_MASK & BLINK.nodeid);
  os_copyMem(&data_msg_tx.payload, buffer, n);
  BLINK.pending |= PEND_DATA_TX;
}

void blink_rx(u1_t *buffer, size_t n) {
  debug_fun(); debug_opmode();
  // copy at most max len payload
  n = n > MAX_LEN_PAYLOAD ? MAX_LEN_PAYLOAD : n;
  os_copyMem(buffer, &data_msg_rx.payload, n);
  BLINK.pending &= ~(PEND_DATA_RX);
}

static void _sync_cb(osjob_t *job) {
  debug_fun(); debug_opmode();
  // lets assume we got a beacon
  beacon_msg_t *b = (beacon_msg_t*)ENZO.frame;
  if(ENZO.dataLen == SIZEOFEXPR(beacon_msg_t) && b->header.type == BEACON) {
    // got a beacon!
    BLINK.missed_beacons = 0;
    // we are hop + 1 away from the sink
    BLINK.hop = b->header.hop + 1;
    // sink starts the beacon in slot 0, so hop count is equal to current (beacon) slot
    BLINK.slot = b->header.hop;
    // set our next wakeup slot
    os_setTimedCallback(&_wakeup_job, ENZO.rxtime + TIME_SLOT_ticks - AIRTIME_BEACON_ticks, FUNC_ADDR(_wakeup));
    // update our opmode
    BLINK.opmode &= ~(OP_SCAN);
    BLINK.opmode |= OP_TRACK;
    // rebroadcast the beacon (if possible)
    _rebroadcast_beacon(b);
    // tell the upper layers
    _report_event(EVENT_SYNC);
    debug_led(0);
  } else {
    debug_char('.');
    // doesn't seem to be a beacon, keep listening
    os_radio(RADIO_RXON);
  }
}

static void _wakeup(osjob_t *job) {
  ostime_t now = os_getTime();
  debug_led(1);
  debug_fun(); debug_opmode();
  ASSERT(BLINK.opmode & OP_READY);

  // increment slot
  _next_slot();

  if(_is_beacon_slot()) {
    /* beacon slot */
    if(BLINK.pending & PEND_BEACON_TX) {
      // retransmit beacon
      os_setCallback(&_transmit_job, FUNC_ADDR(_beacon_tx));
    } else {
      if(BLINK.slot == 0) {
        // we accept any hop
        BLINK.hop_updated = 0;
      }
      // look for beacon
      os_setCallback(&_receive_job, FUNC_ADDR(_beacon_rx));
    }
  } else if(_is_data_slot()) {
    /* data slot */
    if(BLINK.pending & PEND_DATA_TX) {
      // transmit
      os_setCallback(&_transmit_job, FUNC_ADDR(_data_tx));
    } else {
      // listen
      os_setCallback(&_receive_job, FUNC_ADDR(_data_rx));
    }
  } else {
    // TODO no beacon or data slot, err?
	  ASSERT(0);
  }

  // schedule next wakeup
  os_setTimedCallback(&_wakeup_job, now + TIME_SLOT_ticks, FUNC_ADDR(_wakeup));
}

static void _wakeup_root(osjob_t *job) {
  ostime_t now = os_getTime();
  debug_led(1);
  debug_fun(); debug_opmode();
  // increment slot
  _next_slot();
  if(_is_beacon_slot()) {
    if(BLINK.slot == 0) {
      beacon_tx.header.type = BEACON;
      beacon_tx.header.hop  = 0;
      beacon_tx.header.dest = DEST_BROADCAST;
      // radio may be in RXON mode, set in SLEEP mode before we can do anything
      // and clear any pending callbacks
      os_clearCallback(&ENZO.osjob);
      os_radio(RADIO_RST);
      os_setCallback(&_transmit_job, FUNC_ADDR(_beacon_tx));
    } else {
      BLINK.opmode |= (OP_RXBCN);
      os_clearCallback(&ENZO.osjob);
      ENZO.osjob.func = FUNC_ADDR(_rx_root_done);
      os_radio(RADIO_RST);
      os_radio(RADIO_RXON);
      debug_led(0);
    }
  } else if(_is_data_slot()) {
    if(BLINK.opmode & OP_RXDATA) {
      // we're already set up to listen
    } else {
      // adjust the opmode
      BLINK.opmode &= ~(OP_RXBCN);
      BLINK.opmode |= OP_RXDATA;
      // we're already listening
    }
    debug_led(0);
  }
  os_setTimedCallback(&_root_job, now + TIME_SLOT_ticks, _wakeup_root);
}

static void _beacon_tx(osjob_t *job) {
  debug_fun(); debug_opmode();
  ASSERT( ((BLINK.opmode & OP_ROOT) && (BLINK.opmode & OP_READY)) ||
          ((BLINK.opmode & OP_NODE) && (BLINK.opmode & (OP_READY|OP_TRACK))) ||
            0);

  // prepare packet for transmit
  os_copyMem(ENZO.frame, &beacon_tx, SIZEOFEXPR(beacon_msg_t));
  ENZO.dataLen = SIZEOFEXPR(beacon_msg_t);

  // set up tx callback
  ENZO.osjob.func = FUNC_ADDR(_tx_done);

  // set opmode
  BLINK.opmode |= OP_TXBCN;

  // tx
  os_radio(RADIO_TX);
}

static void _beacon_rx(osjob_t *job) {
  debug_fun(); debug_opmode();
  
  // preprare receive
  // set op mode
  BLINK.opmode |= OP_RXBCN;

#if (TRUE == BLINK_USE_CAD)
  ENZO.osjob.func = FUNC_ADDR(_cad_done);
  os_radio(RADIO_CAD);
#else /* TRUE == BLINK_USE_CAD */
  os_clearCallback(&ENZO.osjob);
  ENZO.osjob.func = FUNC_ADDR(_rx_done);
  ENZO.rxsyms = 50; // TODO less symbols is probably also sufficient?
  ENZO.rxtime = 0; // start now
  os_radio(RADIO_RX);
#endif
}

static void _data_tx(osjob_t *job) {
  debug_fun(); debug_opmode();
  ASSERT(BLINK.opmode & (OP_READY|OP_TRACK));

  // prepare packet for transmit
  os_copyMem(ENZO.frame, &data_msg_tx, SIZEOFEXPR(data_msg_t));
  ENZO.dataLen = SIZEOFEXPR(data_msg_t);

  // set opmode
  BLINK.opmode |= OP_TXDATA;

  // set up tx callback
  ENZO.osjob.func = FUNC_ADDR(_tx_done);

  // tx!
  os_radio(RADIO_TX);
}

static void _data_rx(osjob_t *job) {
  debug_fun(); debug_opmode();
  ASSERT(BLINK.opmode & (OP_READY|OP_TRACK));

  // set opmode
  BLINK.opmode |= OP_RXDATA;

#if (TRUE == BLINK_USE_CAD)
  ENZO.osjob.func = FUNC_ADDR(_cad_done);
  os_radio(RADIO_CAD);
#else /* TRUE == BLINK_USE_CAD */
  ENZO.osjob.func = FUNC_ADDR(_rx_done);
  ENZO.rxsyms = 50; // TODO less symbols is probably also sufficient?
  ENZO.rxtime = 0; // start now
  os_radio(RADIO_RX);
#endif
}

static void _cad_done(osjob_t *job) {
  debug_fun(); debug_opmode();
  if(ENZO.cad) {
    // cad deteced
    // TODO do we care about what we want to receive?
    // maybe to set up a timeout on the receive, symbols? 

    // set up rx done callback
    ENZO.osjob.func = FUNC_ADDR(_rx_done);
    // TODO symbols?
    // ENZO.rxsyms = 20;
    // start single rx
    ENZO.rxtime = 0; // start now
    os_radio(RADIO_RX);
  } else {
    if(BLINK.opmode & OP_SCAN) {
      // we're scanning for beacons, don't care about time outs
      os_radio(RADIO_CAD);
    } else if(cad_counter > 0) {
      // retry
      cad_counter--;
      os_radio(RADIO_CAD);
    } else {
      if(BLINK.opmode & OP_RXBCN) {
        // might missed a beacon period
        _missed_beacon();
        BLINK.opmode &= ~(OP_RXBCN);
      } else if (BLINK.opmode & OP_RXDATA) {
        // nothing useful in this data time slot
        BLINK.opmode &= ~(OP_RXDATA);
      }
      // reset cad counter
      cad_counter = CAD_CHECKS;
      debug_led(0);
    }
  }
}

static void _rx_done(osjob_t *job) {
  debug_fun(); debug_opmode();

  if(ENZO.dataLen == 0 || ENZO.crcerr == 1) {
    if(ENZO.crcerr == 1) {
      debug("garbage");
    }
    // nothing received, or received garbage
    if(BLINK.opmode & OP_RXBCN) {
      // we were expecting a beacon, and we missed it
      _missed_beacon();
    }
  } else if(BLINK.opmode & OP_RXBCN) {
    debug("beacon");
    _rx_beacon_done(job);
  } else if(BLINK.opmode & OP_RXDATA) {
    debug("data");
    _rx_data_done(job);
  } else {
    // TODO received when we didn't expect it, err?
    ASSERT(0);
  }

  // clear modes
  BLINK.opmode &= ~(OP_RXBCN|OP_RXDATA);
  debug_led(0);
}

static void _rx_beacon_done(osjob_t *job) {
  debug_fun(); debug_opmode();
  ASSERT(BLINK.opmode & OP_RXBCN);

  // did we receive a beacon?
  beacon_msg_t *b = (beacon_msg_t*)ENZO.frame;
  if(ENZO.dataLen == SIZEOFEXPR(beacon_msg_t) && b->header.type == BEACON) {
    if(BLINK.hop_updated == 0) {
      debug_str("hop ");
      debug_hex(BLINK.hop);
      BLINK.hop = b->header.hop + 1;
      BLINK.hop_updated = 1;
      debug_str(" -> ");
      debug_hex(BLINK.hop);
      debug_char('\r');
      debug_char('\n');
    }
    // update our slot
    if( b->header.hop != BLINK.slot ) {
      debug_str("slot ");
      debug_hex(BLINK.slot);
      debug_str(" -> ");
      debug_hex(b->header.hop);
      debug_char('\r');
      debug_char('\n');
      BLINK.slot = b->header.hop;
    }
    if(abs(_wakeup_job.deadline - (ENZO.rxtime - AIRTIME_BEACON_ticks)) > ms2osticks(MAX_DRIFT_ms)) {
      // reschedule wake slot based on the beacon time as we've drifed too much
      os_setTimedCallback(&_wakeup_job, ENZO.rxtime + TIME_SLOT_ticks - AIRTIME_BEACON_ticks, FUNC_ADDR(_wakeup));
    }
    // reset missed beacons
    BLINK.missed_beacons = 0;

    // rebroadcast the beacon (if possible)
    _rebroadcast_beacon(b);
  } else {
    // expected beacon, got something else, count as a missed beacon
    _missed_beacon();
  }

  BLINK.opmode &= ~(OP_RXBCN);
}

static void _rx_data_done(osjob_t *job) {
  debug_fun(); debug_opmode();
  ASSERT(BLINK.opmode & OP_RXDATA);

  // check if we actually received something data-like
  data_msg_t *d = (data_msg_t*)ENZO.frame;
  if(ENZO.dataLen == SIZEOFEXPR(data_msg_t) && d->header.type == DATA) {
    if(d->header.dest == BLINK.nodeid) {
      // it's for us, notify the upper layer
      os_copyMem(&data_msg_rx, ENZO.frame, SIZEOFEXPR(data_msg_t));
      BLINK.pending |= PEND_DATA_RX;
      _report_event(EVENT_RXCOMPLETE);
    } else {
      // not for us, check if we can rebroadcast it to bring it closer to the sink
      // XXX for now just add the message to the transmit buffer, overwriting any
	  // pending message and falsley trigger the upper layer with EV_TXCOMPLETE
		if(d->header.hop > BLINK.hop) {
        os_copyMem(&data_msg_tx, ENZO.frame, SIZEOFEXPR(data_msg_t));
        data_msg_tx.header.hop--;
        // add our node id to the trace if there's room
        if(data_msg_tx.header.hop < TRACE_MAX) {
          data_msg_tx.footer.trace |= ((TRACE_MASK & BLINK.nodeid) << (TRACE_SHIFT * data_msg_tx.header.hop));
        }
        BLINK.pending |= PEND_DATA_TX;
      }
    }
  } else {
    // expected data, got someting else, may be a beacon?
    if(ENZO.dataLen == SIZEOFEXPR(beacon_msg_t) && d->header.type == BEACON) {
      debug_str("beacon in data slot\r\n");
      // process as beacon
      BLINK.opmode |= OP_RXBCN;
      _rx_beacon_done(job);
    }
  }

  BLINK.opmode &= ~(OP_RXDATA);
}

static void _rx_root_done(osjob_t *job) {
  debug_fun(); debug_opmode();

  header_t *h = (header_t*)ENZO.frame;
  switch(h->type) {
    case BEACON:
      debug_str("beacon ");
      break;
    case DATA:
      debug_str("data ");
      break;
    default:
      debug_str("unknown ");
      break;
  }
  debug_buf(ENZO.frame, ENZO.dataLen);

  if(h->type == DATA) {
      data_msg_t *d = (data_msg_t*)ENZO.frame;
      debug_str("hop ");
      debug_hex(d->header.hop); debug_char('\r'); debug_char('\n');
      debug_str("dest ");
      debug_hex(d->header.dest); debug_char('\r'); debug_char('\n');
      debug_str("trace ");
      for(u1_t i = 0; i < TRACE_MAX; i++) {
        debug_hex(i);
        debug_char(':');
        debug_hex(TRACE_MASK & (d->footer.trace >> (i * TRACE_SHIFT)));
        debug_char(' ');
      }
      debug_char('\r'); debug_char('\n');
  }
  // keep listening
  os_radio(RADIO_RXON);
}

static void _tx_done(osjob_t *job) {
  debug_fun(); debug_opmode();
  if(BLINK.opmode & OP_TXBCN) {
    // clear TXBCN mode and PEND_BEACON_TX bits
    BLINK.opmode &= ~(OP_TXBCN);
    BLINK.pending &= ~(PEND_BEACON_TX);
  } else if(BLINK.opmode & OP_TXDATA) {
    // clear the TXDATA and PEND_DATA_TX bits
    BLINK.opmode &= ~(OP_TXDATA);
    BLINK.pending &= ~(PEND_DATA_TX);
    // report to the upper layer
    _report_event(EVENT_TXCOMPLETE);
  } else {
    // TODO transmission done when we didn't expect it, err?
    ASSERT(0);
  }
  debug_led(0);
}

/* util */
static void _set_radio_callback(osjobcb_t callback) {
  os_clearCallback(&ENZO.osjob);
  ENZO.osjob.func = callback;
}

// rebroadcast a beacon if it hasn't reached it maximum hops yet
static void _rebroadcast_beacon(beacon_msg_t *b) {
  // setup the beacon for rebroadcast if it hasn't reached its max yet
  if(b->header.hop < MAX_BEACON_HOPS) {
    // schedule beacon for rebroadcast
    os_copyMem(&beacon_tx, b, SIZEOFEXPR(beacon_msg_t));
    // increment the hop
    beacon_tx.header.hop++;
    BLINK.pending |= PEND_BEACON_TX;
  }
}

// count missing beacons, restart sync when lost too many
static void _missed_beacon() {
  BLINK.missed_beacons++;

  if(BLINK.missed_beacons > MAX_MISSED_BEACONS) {
    // lost sync
    BLINK.opmode &= ~(OP_TRACK);
    BLINK.opmode |= OP_SCAN;
    // cancel wakeup
    os_clearCallback(&_wakeup_job);
    // report and schedule resync
    _report_event(EVENT_LOST_SYNC);
    blink_start_sync();
  }
}

// report an event to the upper layers
static void _report_event(event_t ev) {
  debug_fun(); debug_opmode();
  // TODO do we need to do more?
  on_event(ev);
}

// go to the next slot
static inline void _next_slot() {
  BLINK.slot++;
  if(BLINK.slot >= TIME_SLOTS) {
    BLINK.slot = 0;
  }
  debug_str("slot ");
  debug_hex(BLINK.slot);
  debug_char(' ');
  if(_is_beacon_slot()) {
    debug_char('B');
  } else if(_is_data_slot()) {
    debug_char('D');
  } else {
    debug_char('?');
  }
  debug_char('\r');
  debug_char('\n');
}

// return true iff the current slot is a beacon slot, false otherwise
static inline u1_t _is_beacon_slot() {
  return BLINK.slot < BEACON_SLOTS;
}

// return true iff the current slot is a data slot, false otherwise
static inline u1_t _is_data_slot() {
  return (BLINK.slot >= BEACON_SLOTS) && (BLINK.slot < TIME_SLOTS);
}

