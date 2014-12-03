#include <msp430.h> 
#include "lcd_driver.h"
#include <string.h>
#include "string_ext.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

//start timing from hi to low because of active low from tone decoder

#define BUTTON BIT4
#define SIGNAL BIT6
#define TIMER_PERIOD 20		//sets timer for 20 us
#define TIMER_CONST	50000		//divide timercounter by this value to get frequency
#define LCD_UPDATE 50000

#define FREQ_TO_LUX	10			//to be calculated from 555 timer ckt

int timerCounter; 				//Keeps track of 2us periods
int timerCalcVal;				//Is used for calculating frequency, set to timerCounter after inputflag changes
int inputFlag; 					//If inputFlag = 1 then waiting for hi to lo transition, if = 0 waiting for lo to hi
int calcFlag;					//If calcFlag = 1 calc new values for everything else do nothings
int updateFlag;
int frequency;

void itoa(long unsigned int value, char* result, int base);

int main(void) {
	char frequencyMsg[] = "Freq(Hz) = ";
	char frequencyMsgDisplay[16] = 0x00;
	char luxMsg[] = "Lux = ";

	char freqValMsg[4];


	int lux;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    timerCounter = 0;

    //Config interrupts for i/o
    P1IE |= SIGNAL;				//enable interrupt
    P1IES |= SIGNAL;			//for hi/lo edge
    P1IFG &= ~SIGNAL;

    //config timer for 2us period
    TACTL = TASSEL_2;			//uses smclk and cont counting
    CCTL0 |= CCIE;				//enable interrupt
    CCR0 = TIMER_PERIOD;		//set timer to TIMERCOUNT * 16E6 seconds

    TA1CTL = TASSEL_2+MC_1;
    TA1CCTL0 |= CCIE;
    TA1CCR0 |= LCD_UPDATE;

    //config LCD
    lcd_setup();

    inputFlag = 1;
    calcFlag = 0;
    updateFlag = 1;
    timerCalcVal = 5000;

    frequency = TIMER_CONST / timerCalcVal;

    __enable_interrupt();

    while(1)
    {
    	//updates LCD screen with frequency value
    	if(updateFlag)
    	{
    		updateFlag = 0;

    		frequencyMsgDisplay[16] = 0x00;

			itoa(frequency,freqValMsg,10);
			strcpy(frequencyMsgDisplay,frequencyMsg);
			strcat(frequencyMsgDisplay,freqValMsg);
			lcd_write_message(frequencyMsgDisplay);
    	}

        if(calcFlag)
        {
        	calcFlag = 0;

        	if(timerCalcVal > 100)
        	{
        		frequency = TIMER_CONST / timerCalcVal;
        	}

        	lux = frequency * FREQ_TO_LUX;

        	P1IE |= SIGNAL;				//enable interrupts again
        	P1IES |= SIGNAL;			//for hi/lo edge
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

				P1IES &= ~SIGNAL;			//for lo/hi edge
				TACTL |= MC_1;				//activate 2us timer
				CCTL0 |= CCIE;
			}
			else
			{
				inputFlag = 1;
				TACTL &= ~MC_1;			//stop timer
				CCTL0 &= ~CCIE;			//disable interrupt
				P1IES |= SIGNAL;		//for hi/low edge
				P1IE &= ~SIGNAL;		//turn off i/o interrupt while calculating frequncy

				timerCalcVal = 2*timerCounter; //update the new timer value to be used in calculations

				calcFlag = 1;
			}

		}
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	timerCounter += 1;
}

// Timer A1 interrupt service routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer_A_1 (void)
{
	updateFlag = 1;
}

//Here be dragons

void itoa(long unsigned int value, char* result, int base)
{
  // check that the base if valid
  if (base < 2 || base > 36) { *result = '\0';}

  char* ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  do {
	tmp_value = value;
	value /= base;
	*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while ( value );

  // Apply negative sign
  if (tmp_value < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while(ptr1 < ptr) {
	tmp_char = *ptr;
	*ptr--= *ptr1;
	*ptr1++ = tmp_char;
  }
}
