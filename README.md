### Hi!

This is my port of RIOS into MSP430, specifically the MSP430FR2355.

The set of files is a full CCS (Code Composer Studio) Project. I used version 12.8.0.
You should be able to clone the repo into a CCS workspace and run the code. I plan to
verify this on a different device very soon.

I've implemented the non-premeptive (tasks don't stop other tasks) version where
the tasks run after a predefine period. The task functions themselves are just delays.

The file in `source-implementation/non-preemptive-simple-tasks.c` contains the original
implementation (non-preemtive, simple tasks) which can be found at: [LINK](https://www.cs.ucr.edu/~vahid/rios/). I've
added extensive notes which explain what each line is doing for both RIMS (a microcontroller
simulator) and AVR ATMega324P. The portion related to AVR mcu are just two functions which
can be found here: [LINK](https://www.cs.ucr.edu/~vahid/rios/rios_avr.htm)

The `main.c` is also commented with references to the MSP430 family user guide and
the device, MSP430FR2355, specific data sheet. The notes are not complete but I will
update that soon.
