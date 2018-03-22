
#define LEDPIN 2
#define F_CPU 8000000UL // clock speed
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1
#define T2PRESCALER 1024
// one tick of the timer for every 20ms
#define T2COUNT (256 - ((F_CPU/T2PRESCALER)/100))
#define R_FOR 3
#define R_BACK 4
#define L_FOR 7
#define L_BACK 0

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

volatile uint8_t left_motor;
volatile uint8_t right_motor;
volatile uint8_t time_to_run;
volatile uint8_t received_number;

volatile uint16_t timer;
volatile uint16_t last_receive_time;
volatile uint16_t next_led_time;

ISR(USART_RX_vect) 
{
	unsigned char status = 0x00;
	if(received_number==0)
	{
		status = UCSR0A;
		left_motor = UDR0;
		received_number = 1;
	}
	else if (received_number==1)
	{
		status = UCSR0A;
		right_motor = UDR0;
		received_number = 2;
	}
	else if (received_number==2)
	{
		status = UCSR0A;
		time_to_run = UDR0;
		received_number = 3;
	}
	else
	{// this means that there is a new command before the last one was run for
	// some reason. This shouldn't happen, so there was an error in the code
	// and we should stop.
		left_motor = 0;
		right_motor = 0;
	}
	// check to see if there were any errors
	if ( status & (_BV(FE0)| _BV(DOR0)| _BV(UPE0)) )
		received_number=0;
	last_receive_time = timer;



}

/* Should have a timer event every 20ms
	*/
ISR(TIMER2_OVF_vect)
{
	timer++;
	TCNT2 = T2COUNT;
}

void USART_Init(unsigned int ubrr)
{
	// set baud rate
	UBRR0H = (unsigned char) (ubrr>>8);
	UBRR0L = (unsigned char) ubrr;
	// enable receiver and receiver interrupt
	UCSR0B = _BV(RXEN0)| _BV(RXCIE0);
	// set frame format, 8 data, 2 stop bits
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

int main(void)
{
	uint8_t led_state = 0;
	USART_Init(MYUBRR);
	DDRD = 0xFF;
	PORTD = _BV(3)|_BV(7);
	DDRB = 0xFF;
	PORTB = 0x00;
	// set the timer counter control registers for timer 0
	// This is the timer for the PWM for the motors
	TCCR0A = _BV(COM0A1)| _BV(COM0B1)| _BV(COM0B1)| _BV(WGM20);
	TCCR0B = _BV(CS00);
	// This is the value for motor connected to "Right Motor" connector
	// Allowed range is 0-255, 255 is always on, 0 is always off
	//OCR0A = 75;
	// This is the value for motor connected to "Left Motor" connector
	// Allowed range is 0-255, 255 is always on, 0 is always off
	//OCR0B = 75;

	// set the counter to 'zero' - really whatever value was pre-computed
	// at compile time to be the right value to reset clock to
	TCNT2 = T2COUNT;
	// set the prescaler to 1024 and enable interrupt on overflow for timer 2
	// This is the timer for the LED operation
	TCCR2B = _BV(CS22)|_BV(CS21)|_BV(CS20);
	// make sure that all the defaults are kept
	TCCR2A = 0;
	TIMSK2 |= _BV(TOIE2);
	PRR = (_BV(PRTIM0)|_BV(PRTIM1)|_BV(PRTIM2));
	sei();
	PORTD |= _BV(LEDPIN);
   
   	while(1)
   	{
		// if the time to run is done then shut off motors
   		if((last_receive_time+time_to_run)<timer)
		{
			right_motor = 0;
			left_motor = 0;
			// turn off motor outputs
			PORTD &= ~(_BV(R_BACK)|_BV(R_FOR)|_BV(L_FOR));
			PORTB &= ~(_BV(L_BACK));
		}
		if(received_number>2)
		{
			received_number=0;
			// if the msb is 1 then move in reverse else move forwards
			if((1<<7) & right_motor)
			{
				PORTD |= (_BV(R_BACK));
				PORTD &= ~(_BV(R_FOR));
			}
			else
			{
				PORTD |= _BV(R_FOR);
				PORTD &= ~(_BV(R_BACK));
			}
			// if the msb is 1 then move in reverse else move forwards
			if((1<<7)&left_motor)
			{
				PORTB |= _BV(L_BACK);
				PORTD &= ~(_BV(L_FOR));
			}
			else
			{
				PORTD |= _BV(L_FOR);
				PORTB &= ~(_BV(L_BACK));
			}
			// left shift one place and then output to motors
			OCR0A = (right_motor<<1);
			OCR0B = (left_motor<<1);
		}

		// set it up to blink the LED in a 'heartbeat'
		if((next_led_time < timer))
		{
			// check for next state
			if(led_state==0)
			{
				led_state++;
				PORTD |= (1<<LEDPIN);
				next_led_time = timer + 10;
			}
			else if(led_state==1)
			{
				led_state++;
				PORTD &= ~(1<<LEDPIN);
				next_led_time = timer + 10;
			}
			else if(led_state==2)
			{
				led_state++;
				PORTD |= (1<<LEDPIN);
				next_led_time = timer + 10;
			}
			else
			{
				led_state=0;
				PORTD &= ~(1<<LEDPIN);
				next_led_time = timer + 50;
			}
		}



   	}
   
   	return 0;
} 


