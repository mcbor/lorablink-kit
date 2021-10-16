#include "enzo.h"
#include "blink.h"
#include "debug.h"

// sensor functions
extern void initsensor(osjobcb_t callback);
extern u2_t readsensor(void);


// report sensor value when change was detected
static void sensorfunc (osjob_t* j) {
    // read sensor
    u2_t val = readsensor();
    debug_val("val = ", val);
    // if we're synced, prepare and schedule data for transmission
    if(BLINK.opmode & OP_TRACK) {
      blink_tx((u1_t*)&val, sizeof(val));
    }
}


// initial job
static void initfunc (osjob_t* j) {
    // intialize sensor hardware
    initsensor(sensorfunc);
    // reset blink state
    blink_reset();
    // start sync
    blink_start_sync();
    // init done - onEvent() callback will be invoked...
}


// application entry point
int main () {
    osjob_t initjob;

    // initialize runtime env
    os_init();
    // initialize debug library
    debug_init();
    // setup initial job
    os_setCallback(&initjob, initfunc);
    // execute scheduled jobs and events
    os_runloop();
    // (not reached)
    return 0;
}


void on_event(event_t ev) {
    switch(ev) {
      case EVENT_SYNC:
          // switch on LED
          debug_led(1);
          // (further actions will be interrupt-driven)
          break;
      case EVENT_LOST_SYNC:
          // switch of fLED
          debug_led(0);
          break;
      default:
          // nop
          break;
    }
}
