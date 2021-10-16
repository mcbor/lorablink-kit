#include "enzo.h"
#include "hw.h"

// use PA7
#define INP_PORT 0
#define INP_PIN  7

static osjob_t irqjob;

// use PA7 as sensor value
void initsensor (osjobcb_t callback) {
    // configure input
    RCC->AHBENR  |= RCC_AHBENR_GPIOBEN; // clock enable port B
    hw_cfg_pin(GPIOx(INP_PORT), INP_PIN, GPIOCFG_MODE_INP | GPIOCFG_OSPEED_40MHz | GPIOCFG_OTYPE_OPEN);
    hw_cfg_extirq(INP_PORT, INP_PIN, GPIO_IRQ_CHANGE);
    // save application callback
    irqjob.func = callback;
}

// read PA7
u2_t readsensor () {
    return ((GPIOB->IDR & (1 << INP_PIN)) != 0);
}

// called by EXTI_IRQHandler
// (set preprocessor option CFG_EXTI_IRQ_HANDLER=sensorirq)
void sensorirq () {
    if((EXTI->PR & (1<<INP_PIN)) != 0) { // pending
	EXTI->PR = (1<<INP_PIN); // clear irq
        // run application callback function in 50ms (debounce)
        os_setTimedCallback(&irqjob, os_getTime()+ms2osticks(50), irqjob.func);
    }
}
