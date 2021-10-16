#include "enzo.h"
#include "debug.h"

// counter
static u2_t counter = 0;

// log text to USART and toggle LED
static void initfunc (osjob_t* job) {
    // say hello
    debug_str("Hello World!\r\n");
    // log counter
    debug_val("counter = ", counter);
    // toggle LED
    debug_led(++counter & 1);
    // reschedule job every second
    os_setTimedCallback(job, os_getTime()+sec2osticks(1), initfunc);
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
