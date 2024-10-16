//#include "RIMS.h"

/*
    Copyright (c) 2013 Frank Vahid, Tony Givargis, and
    Bailey Miller. Univ. of California, Riverside and Irvine.
    RIOS version 1.2
*/

// -------------------------------------------------------------
// Make sure to EXCLUDE the parent folder of this file in CCS
// In CCS 12.8, you can right-click "source-implementation" and
// select "Exclude From Build"


/*
    This code targets RMIS, a micro-controller simulator.
    It has built-in functions TimerOn & TimerSet()

    The author's provide an implementation to achieve the same
    functionality on AVR ATMega324P. I've pasted the two functions
    below and explained every line/operation with references to
    the corresponding sections in the Datasheet.

    I'm including this file with my own comments as I work
    to understand and port the implementation to MSP430.
*/

typedef struct task {
   unsigned long period;      // Rate at which the task should tick
   unsigned long elapsedTime; // Time since task's last tick
   void (*TickFct)(void);     // Function to call for task's tick
} task;

task tasks[2];
const unsigned char tasksNum = 2;
// timer periods are in milliseconds
/*
 * tasksPeriodGCD is the greatest common division between tasks
 * and it's used to set the time between ticks; 200 milliseconds.
 * I suppose it doesn't NEED to be a GCD, since the if condition
 * checks for task[i].elapsedTime >= task[i].period
 *
 * I was initially confused by periodToggle and periodSequence
 * but, I just realized they simply correspond to the TickFcts
 * periodToggle corresponds to TickFct_Toggle and represents the
 * time between execution of the corresponding task. In the main func
 * task 1 has TickFct_Toggle, so periodToggle = 1000 means that task
 * should run every 1000 milliseconds or 1 second.
 *
 * The same goes for periodSequence, Task 0 in main, should
 * run every 200 milliseconds.
 */
const unsigned long tasksPeriodGCD = 200; // Timer tick rate
const unsigned long periodToggle   = 1000;
const unsigned long periodSequence = 200;

void TickFct_Toggle(void);
void TickFct_Sequence(void);


/*
 * for RIMS simply creating a function of the following format
 * is sufficient. It's not necessary to register it in the interrupt
 * vector:
 *
 * TimerISR() {
 *  // ISR code
 * }
 * Source: https://www.cs.ucr.edu/~vahid/pes/RITools/help/
 *
 *
 * so what is this ISR doing?
 * when it's called for the first time, TimerFlag is set
 * then it's called again, which is when it prints the message.
 *
 * However, TimerFlag = 0 within the while loop in main if
 * all tasks finish processing.
 *
 * output compare a match flag is automatically cleared when interrupt
 * vector is executed (This is for AVR, but assuming same applies to RIMS)
*/
unsigned char TimerFlag = 0;
void TimerISR(void) {
   if (TimerFlag) {
      printf("Timer ticked before task processing done.\n");
   }
   else {
      TimerFlag = 1;
   }
   return;
}

int main(void) {
   // Priority assigned to lower position tasks in array
   unsigned char i = 0;
   tasks[i].period      = periodSequence;
   tasks[i].elapsedTime = tasks[i].period;
   tasks[i].TickFct     = &TickFct_Sequence;
   ++i;
   tasks[i].period      = periodToggle;
   tasks[i].elapsedTime = tasks[i].period;
   tasks[i].TickFct     = &TickFct_Toggle;

   TimerSet(tasksPeriodGCD);
   TimerOn();

   while(1) {
      // Heart of the scheduler code
      for (i=0; i < tasksNum; ++i) {
         if (tasks[i].elapsedTime >= tasks[i].period) { // Ready
            tasks[i].TickFct(); //execute task tick
            tasks[i].elapsedTime = 0;
         }
         // if we're not executing task[i] then let's
         // updated it's elapsedTime
         tasks[i].elapsedTime += tasksPeriodGCD;
      }
      // why do we clear TimerFlag?
      // why does the program sleep?
      TimerFlag = 0;
      while (!TimerFlag) {
         Sleep();
      }
   }
}

/*
 * So B0 is just a "switch" or a bit in RIMS.
 * so these function are toggling and rotating (sequence)
 * we can do the same for using registers or just
 * call a __delay_cycles(n) function to delay
 * for n cycles.
 */


// Task: Toggle an output
void TickFct_Toggle(void)   {
   static unsigned char init = 1;
   if (init) { // Initialization behavior
      B0 = 0;
      init = 0;
   }
   else { // Normal behavior
      B0 = !B0;
   }
}

 // Task: Sequence a 1 across 3 outputs
void TickFct_Sequence(void) {
   static unsigned char init = 1;
   unsigned char tmp = 0;
   if (init) { // Initialization behavior
      init = 0;
      B2   = 1;
      B3   = 0;
      B4   = 0;
   }
   else { // Normal behavior
      tmp = B4;
      B4  = B3;
      B3  = B2;
      B2  = tmp;
   }
}



void TimerOn(void){
    /*
     *  TCCR1B = Timer/Counter 1 control register B
     *  Section 17.14.2
     *
     *  CS = clock select; 1 corresponds to timer/counter 1
     *  Section 17.14.2
     *
     *  WMG = Waveform generation mode; 1 corresponds to timer/counter 1
     *  Section 16.19.1 PG 142
     *  1<<WMG12: this means shift 1 to WMG1 bit position 2
     *  meaning: enable  Clear Timer on Compare Match (CTC) Mode
     *  Section 17.12.2
     *
     *
     *  1<CS12: means shift 1 to CS1 bit position 2
     *  meaning: select internal clock (8GHz) / 256 as source
     *  Section 17.14.2
     *
     *  TIMSK1: means timer/counter interrupt mask register
     *  1<<OCIE1A means enable interrupt for output compare register A
     *  Section 17.14.8
     *
     *  SREG: Status Register
     *  SREG |= 0x80 means set bit position 7 to 1
     */
   TCCR1B = (1<<WGM12)|(1<<CS12); //Clear timer on compare. Prescaler = 256
   TIMSK1 = (1<<OCIE1A); //Enables compare match interrupt
   SREG |= 0x80; //Enable global interrupts
}

/*
 *  Related Notes:
 *
 *  ICR = Input Capture Register
 *
 *  Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42743-ATmega324P_Datasheet.pdf
 *
 */

/*
 * Okay, so from what I understand, the setup here
 * is to have the interrupt trigger after X milliseconds
 * where x in the input to TimerSet()
 * we can achieve a similar clock in MSP430 by choosing
 * SMCLK, a 1 GHz clock, and dividing by 32.
 * 8,000,000/256 = 1,000,000/32 = 31250
 * that is 31250 ticks, clock cycles, per second.
 * divide that by a thousand, we get 31.25 ticks per ms(millisecond)
 * now, if we take whatever input X we get and multiply by 31.25,
 * we will get a period of X milliseconds.
 */
void TimerSet(int milliseconds){
    //  TCNT1 = Timer/Counter register; the 1 represents the timer/counter number
    //  ATmega324P has three timers
    //  this is the register which holds count value of the clock
    //  so as the clock starts counting from 0, 1, 2, 3, .... the value is stored in this register.
    //  thus, to set the timer. This value is cleared and set to 0.
    //
    //  this functionality will be achieved in MSP430 by using TBxCLR which will
    //  clear the related timer
    TCNT1 = 0;
    //  OCR1A = output compare register; A represents the output compare unit
    //  ATmega324P timer 1 has 2 (A & B) output compare registers
    //  holds the value which will TCNT1 will be matched against
    //  when the values match, the OC1A interrupt will be triggered
    //
    //  This functionality will be achieved in the MSP 430 by using the UP count mode
    //  then assigning the value of ms*31.25 to CCR0.
    OCR1A = milliseconds*31.25; // 8 MHz AVR, prescalar == 256 -> 31.25 ticks per ms
}
