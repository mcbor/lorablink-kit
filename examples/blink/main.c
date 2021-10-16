/*
 * blink
 * Simple protocol prototype for data collection in a LoRa sensor network
 *
 * @author: Martin Bor <m.bor@lancaster.ac.uk>
 * @date: 2015-11-05
 *
 */

#include "enzo.h"
#include "debug.h"
#include "blink.h"

#if !defined(NODE_ID)
#define NODE_ID 0x1
#endif

static void reportfunc(osjob_t *job);
static void initfunc(osjob_t *job);

static osjob_t _report_job;

static u4_t _counter;
static u1_t tx;

static const u1_t* eventnames[] = {
  [EVENT_SYNC]      = (u1_t*)"SYNC",
  [EVENT_LOST_SYNC] = (u1_t*)"SYNC_LOST",
  [EVENT_RXCOMPLETE]= (u1_t*)"RXCOMPLETE",
  [EVENT_TXCOMPLETE]= (u1_t*)"TXCOMPLETE",
};

ostime_t next_report_time() {
  // calculate when the next epoch starts
  ostime_t time_till_next_epoch = (TIME_SLOTS - BLINK.slot) * TIME_SLOT_ticks;
  // calculate time offset of the data slots relative to the epoch
  ostime_t data_slot_time_offset = BEACON_SLOTS * TIME_SLOT_ticks;
  // we want to transmit once per epoch in a random data time slot
  u1_t tx_time_slot = radio_rand1() % DATA_SLOTS;
  // calculate time slot in time offset
  ostime_t tx_time = tx_time_slot * TIME_SLOT_ticks;
  // next report is start of the epoch + time offset
  return time_till_next_epoch + data_slot_time_offset + tx_time;
}

void on_event(event_t ev) {
  debug_str(eventnames[ev]);
  debug_char('\r');
  debug_char('\n');

  if(NODE_ID > 0) {
  switch(ev) {
    case EVENT_SYNC:
      debug_str("start report\r\n");
      os_setTimedCallback(&_report_job, os_getTime() + (NODE_ID * TIME_SLOT_ticks), FUNC_ADDR(reportfunc));
      break;
    case EVENT_LOST_SYNC:
      debug_str("stop report\r\n");
      os_clearCallback(&_report_job);
      break;
    case EVENT_RXCOMPLETE:
      // nop
      break;
    case EVENT_TXCOMPLETE:
      if(tx == 1) {
        debug_str("set next report\r\n");
        os_setTimedCallback(&_report_job, os_getTime() + next_report_time(), FUNC_ADDR(reportfunc));
        tx = 0;
      } else {
        debug_str("forwarding\r\n");
      }
      break;
    default:
      // nop
      break;
  }}
}

static void reportfunc(osjob_t *job) {
  debug_str("reportfunc\r\n");
  u1_t data[5];
  _counter++;
  data[0] = NODE_ID;
  data[1] = (u1_t)(0xff & (_counter >> 24));
  data[2] = (u1_t)(0xff & (_counter >> 16));
  data[3] = (u1_t)(0xff & (_counter >> 8));
  data[4] = (u1_t)(0xff & (_counter >> 0));
  blink_tx((u1_t*)&data, SIZEOFEXPR(data));
  tx = 1;
}

static void initfunc(osjob_t* job) {
  debug_str("> initfunc\r\n");
  debug_str("node ");
  debug_hex(NODE_ID);
  debug_char('\r');
  debug_char('\n');
  BLINK.nodeid = NODE_ID;
  blink_reset();
  blink_start_sync();
}

int main(void) {
  osjob_t initjob;

  // init runtime
  os_init();
  // init debug lib
  debug_init();
  // init blink
  blink_init();
  // setup initial job
  os_setCallback(&initjob, initfunc);
  // execute scheduled jobs and events
  os_runloop();
  // (not reached)
  return 0;
}


