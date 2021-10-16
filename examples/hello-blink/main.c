#include "enzo.h"
#include "debug.h"
#include "blink.h"

#define NODEID 1

// fwd decl
static void ping(osjob_t *job);
static void initfunc(osjob_t *job);

static u1_t counter;
static osjob_t pingjob;

void on_event(event_t ev) {
  switch(ev) {
    case EVENT_SYNC:
      debug_str("got sync\r\n");
      os_setCallback(&pingjob, FUNC_ADDR(ping));
      break;
    case EVENT_LOST_SYNC:
      debug_str("lost sync\r\n");
      os_clearCallback(&pingjob);
      break;
    case EVENT_RXCOMPLETE:
      debug_str("rx complete\r\n");
      u1_t payload[MAX_LEN_PAYLOAD];
      blink_rx(payload, MAX_LEN_PAYLOAD);
      debug_buf(payload, MAX_LEN_PAYLOAD);
      break;
    case EVENT_TXCOMPLETE:
      debug_str("tx complete\r\n");
      break;
    default:
      // nop
      break;
  }
}


static void ping(osjob_t *job) {
  debug_val("ping ", counter);
  u1_t payload[6] =  {'P', 'i', 'n', 'g', ' ', counter++};
  blink_tx(payload, SIZEOFEXPR(payload));
  os_setTimedCallback(job, os_getTime() + sec2osticks(5), FUNC_ADDR(ping));
}

static void initfunc(osjob_t* job) {
  BLINK.nodeid = NODEID;
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

