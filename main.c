#include <msp430.h> 
#include "lcd_driver.h"

#define BUTTON BIT4
#define SIGNAL BIT0
#define TIMER_PERIOD 320		//sets timer for 20 us
#define TIMER_CONST	50,000		//divide timercounter by this value to get frequency
#define FREQ_TO_LUX	10			//to be calculated from 555 timer ckt

int timerCounter; 				//Keeps track of 2us periods
int timerCalcVal;				//Is used for calculating frequency, set to timerCounter after inputflag changes
int inputFlag; 					//If inputFlag = 1 then waiting for low to high transition, if = 0 waiting for high to low

int main(void) {
	char frequencyMsg[] = "Frequency = ";
	char luxMsg[] = "Lux = ";

	int frequency;
	int lux;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    timerCounter = 0;

    //Config interrupts for i/o
    P1IE |= SIGNAL;				//enable interrupt
    P1IES &= ~SIGNAL;			//for lo/hi edge
    P1IFG &= ~SIGNAL;

    //config timer for 2us period
    CCTL0 = CCIE;				//enable interrupt
    TACTL = TASSEL_2;			//uses smclk and cont counting
    CCR0 = TIMER_PERIOD;			//set timer to TIMERCOUNT * 16E6 seconds

    inputFlag = 1;
    timerCalcVal = 5000;

    lcd_setup();

    __enable_interrupt();

    while(1)
    {
    	lcd_write_message(frequencyMsg+(char)frequency);

        if(inputFlag)
        {

        	frequency = TIMER_CONST / timerCalcVal;

        	lux = frequency * FREQ_TO_LUX;

        	P1IES &= ~SIGNAL;			//for lo/hi edge
        }
    }


	return 0;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	if(P1IFG & SIGNAL)
		{
			P1IFG &= ~SIGNAL; 				// P1.4 IFG cleared

			if(inputFlag)
			{
				inputFlag = 0;
			    timerCounter = 0;			//reset counter to start counting

				P1IES |= SIGNAL;			//for hi/lo edge
				TACTL |= MC_2;				//activate 2us timer

			}
			else
			{
				inputFlag = 1;
				TACTL &= ~MC_2;			//stop timer
				P1IES &= ~SIGNAL;		//for lo/hi edge
				P1IE &= ~SIGNAL;		//turn off i/o interrupt while calculating frequncy

				timerCalcVal = 2*timerCounter; //update the new timer value to be used in calculations
			}

		}
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	timerCounter += 1;
	CCR0 += TIMER_PERIOD;                            // Add Offset to CCR0
}
