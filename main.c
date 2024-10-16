#include <msp430.h>
#include <stdio.h>


/*
 *
 * Resources:
 * WDTCTL: Sectoin 12.3.1
 * TB0CTL: Section 14.3.1 (FUG)
 *
 * Macros such as TBCLR, TBSSEL__ACLK are defined in the msp430fr2355.h file
 * located in includes > /Applications/ti/ccs1280/ccs/ccs_base/msp430/include
 *
 * MSP430 Family User Guide (FUG): https://www.ti.com/lit/ug/slau445/slau445.pdf
 * MSP430FR235x Device Specific Datasheet (DSD): https://www.ti.com/lit/ds/symlink/msp430fr2355.pdf
 */


typedef struct task {
    unsigned long period;       // rate at which task should tick
    unsigned long elapsedTime;  // time since task's last tick
    void (*TickFct)(void);      // function to call for task's tick
} task;

task tasks[2];
const unsigned int taskNum = 2;

// long = 4 bytes
// why are they using long?
const unsigned long tasksPeriodGCD = 200;
const unsigned long periodToggle = 1000;
const unsigned long periodSequence = 200;

// declare tick functions
void TickFct_Toggle(void);
void TickFct_Sequence(void);


unsigned char TimerFlag =  0;

#pragma vector = TIMER0_B0_VECTOR
__interrupt void isr_timer_b0_ccr0(void) {
    if (TimerFlag) {
        printf("Timer ticked before task processing done.\n");
    } else {
        TimerFlag = 1;
    }
    TB0CCTL0 &= ~CCIFG; // clear interrupt flag
    return;
}



int main(void)
{
    // the watchdog timer is for error recovery
    // if it is not reset before it expires, a system reset
    // is generated (FUG Chapter 12)
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    TB0CTL |= TBCLR;           // clear timer and dividers

    // the following two operations work to divide timer 8 * 4 = 32
    // 1MHZ/32 = 1,000,000/32 = 31,250
    // so clock will tick 31,250/second
    // or 31.25/millisecond
    TB0CTL |= TBSSEL__SMCLK;     // source clock = SMCLK (1 MHz)
    TB0CTL |= ID__8;            // prescaler/divider = 8
    TB0EX0 |= TBIDEX__4;        // input divider expansion = 4

    // timer will count up to value in CCR0, then reset to 0
    // when timer count reaches value in CCR0
    TB0CTL |= MC__UP;           // select UP count mode

    TB0CCR0 = 31.25 * tasksPeriodGCD;
    TB0CCTL0 |= CCIE;            // enable interrupt
    TB0CCTL0 &= ~CCIFG;          // clear interrupt flag

    // initialize tasks
    unsigned char i = 0;
    tasks[i].period = periodSequence;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Sequence;
    i++; // no difference in pre-increment (++i) or post-increment (i++) for this use case
    tasks[i].period = periodToggle;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Toggle;



    __enable_interrupt();        // enable maskable interrupts

    while(1) {
        for (i=0; i < taskNum; i++) {
            if (tasks[i].elapsedTime >= tasks[i].period) {
                tasks[i].TickFct(); // execute task
                tasks[i].elapsedTime = 0;
            }
            tasks[i].elapsedTime += tasksPeriodGCD;
        }
        TimerFlag = 0;
        while(!TimerFlag) {
            __low_power_mode_1();
        }
    }

    return 0;
}

void TickFct_Toggle(void) {
    __delay_cycles(10);
}

void TickFct_Sequence(void) {
    __delay_cycles(50);
}
