#include <msp430.h> 
#include "lcd_driver.h"
#include <string.h>
#include "string_ext.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

//start timing from hi to low because of active low from tone decoder

#define BUTTON BIT7
#define SIGNAL BIT6
#define TIMER_PERIOD 320		//sets timer for 20 us
#define TIMER_CONST	50073		//divide timercounter by this value to get frequency
#define LCD_UPDATE 16000		//sets LCD timer rate to 1 ms
#define LCD_REFRESH 500			//update rate of LCD in ms
#define FREQ_TO_IPD 0.792
#define IPD_TO_FC 18.6
#define NUM_AVG 5

#define FREQ_TO_LUX	10			//to be calculated from 555 timer ckt

int timerCounter; 				//Keeps track of 2us periods
int timerCalcVal;				//Is used for calculating frequency, set to timerCounter after inputflag changes
int inputFlag; 					//If inputFlag = 1 then waiting for hi to lo transition, if = 0 waiting for lo to hi
int calcFlag;					//If calcFlag = 1 calc new values for everything else do nothings
int updateCounter;
int lcdDisplay = 0;
int frequencyAvg[NUM_AVG];
int frequency;
int measureCount = -1;

void itoa(long unsigned int value, char* result, int base);

int main(void) {
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	const char frequencyMsg[] = "Freq(Hz) = ";
	const char currentMsg[] = "Ipd(uA) = ";
	const char footCandleMsg[] = "I(fc) = ";

	char messageDisplay[16] = 0x00;

	char valueMsg[4];
	char decimalMsg[2];

	unsigned int footCandle = 0;
	unsigned int millifootCandle = 0;
	unsigned int milliHz = 0;
	unsigned int photoCurrent = 0;
	unsigned int nanoCurrent = 0;
	int i;
    timerCounter = 0;

    //Config interrupts for i/o
    P1IE |= SIGNAL+BUTTON;				//enable interrupt
    P1IES |= SIGNAL;					//for hi/lo edge
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
    updateCounter = 0;
    timerCalcVal = 5000;

    frequency = TIMER_CONST / timerCalcVal;

    __enable_interrupt();

    while(1)
    {
    	//updates LCD screen with frequency value
    	if(updateCounter == LCD_REFRESH)
    	{
    		if(lcdDisplay > 2)
    		{
    			lcdDisplay = 0;
    		}

    		updateCounter = 0;

    		messageDisplay[16] = 0x00;

    		BCSCTL1 = CALBC1_1MHZ;
			DCOCTL = CALDCO_1MHZ;

			if(lcdDisplay == 0)
			{
				//frequency

				itoa(frequency,valueMsg,10);
				itoa(milliHz,decimalMsg,10);
				strcpy(messageDisplay,frequencyMsg);
				strcat(messageDisplay,valueMsg);
				strcat(messageDisplay,".");
				strcat(messageDisplay,decimalMsg);
				lcd_write_message(messageDisplay);
			}

			if(lcdDisplay == 1)
			{
				//photodiode current

				itoa(photoCurrent,valueMsg,10);
				itoa(nanoCurrent,decimalMsg,10);
				strcpy(messageDisplay,currentMsg);
				strcat(messageDisplay,valueMsg);
				strcat(messageDisplay,".");
				strcat(messageDisplay,decimalMsg);
				lcd_write_message(messageDisplay);
			}

			if(lcdDisplay == 2)
			{
				//foot candle

				itoa(footCandle,valueMsg,10);
				itoa(millifootCandle,decimalMsg,10);
				strcpy(messageDisplay,footCandleMsg);
				strcat(messageDisplay,valueMsg);
				strcat(messageDisplay,".");
				strcat(messageDisplay,decimalMsg);
				lcd_write_message(messageDisplay);
			}

			BCSCTL1 = CALBC1_16MHZ;
			DCOCTL = CALDCO_16MHZ;
    	}

        if(calcFlag)
        {
        	calcFlag = 0;

        	if(timerCalcVal > 100)
        	{
        		measureCount++;

        		timerCalcVal /= 10;

        		if(measureCount == NUM_AVG)
        		{
        			measureCount = -1;

        			frequency = 0;

        			for(i = 0;i<NUM_AVG;i++)
        			{
        				frequency += frequencyAvg[i];
        			}

        			frequency /= NUM_AVG;

        			//gotta do this modulo 10, divide by 10 because of decimal requirement

					photoCurrent = frequency*FREQ_TO_IPD;

					footCandle = photoCurrent*IPD_TO_FC;
					millifootCandle = footCandle % 10;
					footCandle /= 100;

					nanoCurrent = photoCurrent % 100;
					photoCurrent /= 100;

					milliHz = frequency % 10;
					frequency /= 10;								//frequency in hz
        		}

        		frequencyAvg[measureCount] = TIMER_CONST / timerCalcVal; 	//frequency in decihertz?

        	}

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
	if(P1IFG & BUTTON)
	{
		lcdDisplay += 1;

		P1IFG &= ~BUTTON;
	}

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
	updateCounter += 1;
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
