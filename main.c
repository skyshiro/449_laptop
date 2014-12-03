#include <msp430.h> 
#include "lcd_driver.h"
#include <string.h>
#include "string_ext.h"

//start timing from hi to low because of active low from tone decoder

#define BUTTON BIT4
#define SIGNAL BIT0
#define TIMER_PERIOD 320		//sets timer for 20 us
#define TIMER_CONST	50,000		//divide timercounter by this value to get frequency
#define FREQ_TO_LUX	10			//to be calculated from 555 timer ckt

int timerCounter; 				//Keeps track of 2us periods
int timerCalcVal;				//Is used for calculating frequency, set to timerCounter after inputflag changes
int inputFlag; 					//If inputFlag = 1 then waiting for low to high transition, if = 0 waiting for high to low

void tostr(int i,char *s);

int main(void) {
	char frequencyMsg[] = "Freq(Hz) = ";
	char luxMsg[] = "Lux = ";

	char freqValMsg[4];

	int frequency;
	int lux;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    timerCounter = 0;

    //Config interrupts for i/o
    P1IE |= SIGNAL;				//enable interrupt
    P1IES |= SIGNAL;			//for hi/lo edge
    P1IFG &= ~SIGNAL;

    //config timer for 2us period
    CCTL0 = CCIE;				//enable interrupt
    TACTL = TASSEL_2;			//uses smclk and cont counting
    CCR0 = TIMER_PERIOD;			//set timer to TIMERCOUNT * 16E6 seconds

    inputFlag = 1;
    timerCalcVal = 5000;

    lcd_setup();

    //lcd_write_message(frequencyMsg);

    //START TEST CODE
    frequency = 10;

    snprint_sd16(freqValMsg,4,frequency);

    //tostr(frequency,freqValMsg);

    lcd_write_message(strcat(frequencyMsg,freqValMsg));

    //END TEST CODE

    while(1);

    __enable_interrupt();

    while(1)
    {
    	lcd_write_message(frequencyMsg+(char)frequency);

        if(inputFlag)
        {

        	frequency = TIMER_CONST / timerCalcVal;

        	lux = frequency * FREQ_TO_LUX;

        	P1IES |= SIGNAL;			//for lo/hi edge
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
			P1IFG |= SIGNAL; 				// P1.4 IFG cleared

			if(inputFlag)
			{
				inputFlag = 0;
			    timerCounter = 0;			//reset counter to start counting

				P1IES &= ~SIGNAL;			//for lo/hi edge
				TACTL |= MC_2;				//activate 2us timer
			}
			else
			{
				inputFlag = 1;
				TACTL &= ~MC_2;			//stop timer
				P1IES |= SIGNAL;		//for hi/low edge
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

void tostr(int i,char *s) // Convert Integer to String
{
	char *p;
	p=s;
	p[2]=i%10;
	i-=p[2];
	i/=10;
	p[1]=i%10;
	i-=p[1];
	i/=10;
	p[0]=i%10;
	i-=p[0];
        p[3] = 0; // mark end of string
	p[2]+=0x30;
	p[1]+=0x30;
	p[0]+=0x30;
}
