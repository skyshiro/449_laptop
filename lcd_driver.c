//Material is copyright of Hodge Industries
//blerg andy is going to die
#include <msp430.h>

void lcd_setup()
{
	P1DIR |= BIT3 + BIT2 + BIT1 + BIT0; //sets all of PORT1 to output, using PORT1 as parallel bus
	P2DIR |= BIT0 + BIT1 + BIT2; //using p2.0 through p2.2 as control lines

	__delay_cycles(25000);

	//send set function 1
	P2OUT = BIT2; //sets enable pin high
	P1OUT |= 0x2;  //MSB of function set
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0x2;
	//send set function 2
	P2OUT = BIT2; //sets enable pin high
	P1OUT |= 0x8;  //LSB of function set
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0x8;

	__delay_cycles(40);

	//Send display set 1
	P2OUT = BIT2;
	P1OUT |= 0x0;
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0x00;
	//Send display set 2
	P2OUT = BIT2;
	P1OUT |= 0xD;
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0xD;

	__delay_cycles(40);

	//Send clear display 1
	P2OUT = BIT2;
	P1OUT |= 0x0;
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0x00;
	//Send clear display 2
	P2OUT = BIT2;
	P1OUT |= 0x1;
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0x1;

	__delay_cycles(1600);
}

void lcd_write_message(char *message)
{
	unsigned int i,j;

	//Send clear display 1
	P2OUT = BIT2;
	P1OUT |= 0x0;
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0x00;
	//Send clear display 2
	P2OUT = BIT2;
	P1OUT |= 0x1;
	P2OUT = 0x00; //turn off enable so data is latched in
	P1OUT &= ~0x1;

	__delay_cycles(1600);

	if(strlen(message)<16)
	{

		for(i=0;i<(16-strlen(message))/2;i++)
			{
				P2OUT = BIT0+BIT2;
				P1OUT |= (int)(' ')>>4;
				P2OUT = BIT0; //turn off enable so data is latched in
				P1OUT &= ~((int)(' ')>>4);
				P2OUT = 0x00;

				P2OUT = BIT0+BIT2;
				P1OUT |= (int)(' ') & 0xF;
				P2OUT = BIT0; //turn off enable so data is latched in
				P1OUT &= ~((int)(' ') & 0xF);
				P2OUT = 0x00;

				__delay_cycles(200);

			}

			for(i=0;i<strlen(message);i++)
			{
				//change CGRAM data

				P2OUT = BIT0+BIT2;
				P1OUT |= (int)message[i]>>4;
				P2OUT = BIT0; //turn off enable so data is latched in
				P1OUT &= ~((int)message[i]>>4);
				P2OUT = 0x00;

				P2OUT = BIT0+BIT2;
				P1OUT |= (int)message[i] & 0xF;
				P2OUT = BIT0; //turn off enable so data is latched in
				P1OUT &= ~((int)message[i] & 0xF);
				P2OUT = 0x00;

				__delay_cycles(200);
			}
	}
	else
	{
		for(i=0;i<strlen(message);i++)
			{
			if(i == (strlen(message)-15)) //this is where the magic happens baby
			{
				break;
			}
				//Change address back to bottom left 1
				P2OUT = BIT2;
				P1OUT = 0xC;
				P2OUT = 0x00; //turn off enable so data is latched in
				P1OUT = 0x00;
				//Change address back to bottom left 2
				P2OUT = BIT2;
				P1OUT = 0x0;
				P2OUT = 0x00; //turn off enable so data is latched in
				P1OUT = 0x00;

				__delay_cycles(200);

				for (j = 0; j < 16; j++)
				{
					//change CGRAM data

					P2OUT = BIT0+BIT2;
					P1OUT = (int)message[i+j]>>4;
					P2OUT = BIT0; //turn off enable so data is latched in
					P1OUT = 0x00;
					P2OUT = 0x00;

					P2OUT = BIT0+BIT2;
					P1OUT = (int)message[i+j] & 0xF;
					P2OUT = BIT0; //turn off enable so data is latched in
					P1OUT = 0x00;
					P2OUT = 0x00;

					__delay_cycles(200);

				}

				//Change address back to bottom left 1
				P2OUT = BIT2;
				P1OUT = 0xC;
				P2OUT = 0x00; //turn off enable so data is latched in
				P1OUT = 0x00;
				//Change address back to bottom left 2
				P2OUT = BIT2;
				P1OUT = 0x0;
				P2OUT = 0x00; //turn off enable so data is latched in
				P1OUT = 0x00;

				__delay_cycles(50000);
			}
	}
}
