//  lab4
//  Hsuan-Chih Hsu
//  11.13.2022

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base.

// Port mapping:
// Port A:  bit0 brown  segment A
//          bit1 red    segment B
//          bit2 orange segment C
//          bit3 yellow segment D
//          bit4 green  segment E
//          bit5 blue   segment F
//          bit6 purple segment G
//          bit7 grey   decimal point
//               black  Vdd
//               white  Vss

// Port B:  bit0 red	CLK_INH
//	    bit1 red	SR_CLK
//	    bit2 brown	SDIN
//	    bit3 orange SER_OUT_MISO
//	    bit4 green  seg0
//          bit5 blue   seg1
//          bit6 purple seg2
//          bit7 grey   pwm 
//          EN   black  Vdd
//          EN_N white  Vss

// PORT C:  bit4 AMP

// PORT D:  bit2 orange REG_CLK

// PORT E:  bit6 grey   SH_LD

// PORT F:  bit7 yellow CdS

#define TRUE 1
#define FALSE 0

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "hd44780.h"
#include <string.h>
#include <stdlib.h>
#include "beaver_fight_song.h"

//Global variables
//volatile int16_t count = 0x0000; //setup a counter
volatile uint8_t mode = 0x00; // default mode
volatile uint8_t num_mode;
volatile uint8_t bar_graph = 0x00; //default bargraph
volatile uint8_t enc_serial_output;
volatile uint8_t enc;
volatile uint8_t prev_enc;
volatile uint8_t enc_mode;

//New variable
volatile uint8_t hour_format = 0x00; //24 hour format

volatile uint8_t set_sys_clock = 0x00; //default, lock sys clk
volatile int8_t temp_sys_minute;
volatile int8_t temp_sys_hour;
//volatile uint8_t play_beav_song = 0x00;

volatile int16_t beat;
volatile uint16_t max_beat;
volatile int8_t  notes;
#define ALARM_PIN 0x10

volatile uint8_t set_alarm = 0x00; //default, lock alarm time
volatile uint8_t alarm_hour;
volatile uint8_t alarm_minute;
volatile uint8_t uh_laam = 0x00; //default
volatile uint8_t alarm_armed = 0x00; //default


//char lcd_str[16];

void tcnt_init(void); //config of all tcnt
void config_IDLE(void);	//config of initial state
void disable_tristate();
void switch_ON(); 
void switch_OFF(); 
void clear_segment();

//alarm func
//void music_off(void);
//void music_on(void);
//void alarm_init(void);

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5]; 

//decimal to 7-segment LED display encodings, logic "0" turns on segment
//[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, X, X]
uint8_t dec_to_7seg[12] = {
	0b11000000,
	0b11111001,
	0b10100100,
	0b10110000,
	0b10011001,
	0b10010010,
	0b10000010,
	0b11011000,
	0b10000000,
	0b10010000,
	0b11111111,
	0b11111111}; 


//******************************************************************************
//                            chk_buttons                                      
//Checks the state of the button number passed to it. It shifts in ones till   
//the button is pushed. Function returns a 1 only once per debounced button    
//push so a debounce and toggle function can be implemented at the same time.  
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"            
//Expects active low pushbuttons on PINA port.  Debounce time is determined by 
//external loop delay times 12. 
//
uint8_t chk_buttons(uint8_t button) {
	static uint16_t state[8] = {0}; //holds present state
  	state[button] = (state[button] << 1) | (! bit_is_clear(PINA, button)) | 0xE000;
  	if(state[button] == 0xF000) return 1;
  	return 0;
}
//******************************************************************************


//***********************************************************************************
//																	spi_init
//Initializes the SPI port on the mega128. 
//***********************************************************************************
void spi_init() {

	DDRF 	|= 0x08; //PORT F bit 3 is enabling for LCD
	PORTF &= 0xF7;   //PORT F bit 3 is initially low

	DDRB 	= 0x07;  		//Turn on SS, MOSI, SCLK (SS is output)
	SPCR  = (1<<SPE) | (1<<MSTR);  	//SPI enabled, master, low polarity, MSB 1st
  	SPSR  = (1<<SPI2X);		//run at i/o clock/2
}//spi_init

//***********************************************************************************


//***********************************************************************************
//																	spi_read
//Reads the spi port
//**********************************************************************************
uint8_t spi_read() {
	SPDR = 0x00; 				//dummy write to SPDR
	while(bit_is_clear(SPSR, SPIF)) {;} 	//waits until 8 clock cycles are done
	return(SPDR); 				//returns incoming data from SPDR
}//spi_read

//***********************************************************************************



//***********************************************************************************
//																ADC (CdS cell)
// start_adc(): Start ADC conversion
// set_brightness(): Calculate the brighness
// ISR(ADC_vect): store the result of ADC conversion
//***********************************************************************************
volatile uint16_t adc_result = 820; //initial value

#define MIN_BRIGHTNESS 	   230
#define DEFAULT_BRIGHTNESS 64
#define MAX_BRIGHTNESS 	   0

void start_adc() {ADCSRA |= (1<<ADSC);} //poke the ADSC bit and start conversion

//linear function: y = -0.357(x) + 357.143
void set_brightness(void) {
	if(adc_result < 440) 		{OCR2 = MIN_BRIGHTNESS;}
	else if(adc_result >= 1000) 	{OCR2 = MAX_BRIGHTNESS;}
	else 				{OCR2 = ((adc_result*-0.5) + 357.143);}
}//set_brightness()

ISR(ADC_vect) {
	adc_result = ADC;
	start_adc(); //start the next conversion
} //read the ADC value as 16 bits
//***********************************************************************************
//																Read Encoder(read_enc)
//Reads one encoder and detect two different mode: clockwise and counter-clockwise
//Details: if "clockwise", in first 2-bit enc:  
//				 if "counter-clockwise", enc_b = enc_a;
//Only need to read first two bits.
//***********************************************************************************
int8_t read_enc(void) {
	
	prev_enc = enc; //store the previous encoder output

	//clk_inh is high now => able to parallel read inputs if sh_ld is low
	PORTE &= ~(1<<PE6); 	//sets falling edge for sh_ld
	//_delay_ms(1);		//gives delay to read the parallel inputs 
	PORTE |= (1<<PE6); 	//rising edge for sh_ld

	PORTB &= ~(1<<PB0); //set clk_inh as low to read the encoder
	enc_serial_output = spi_read(); //read the encoder serial output 

	//only take the first two bit to compare
	enc = enc_serial_output & 0x03;
	prev_enc = prev_enc & 0x03;

	//detect if enc is changes, if so encoder is activated 
	if(prev_enc != enc) {
		//counter-clockwise
		if((prev_enc == 0x03) && (enc == 0x02)) {
			enc_mode = 0x01; 
			return 0;
		} 

		//clockwise
		else if((prev_enc == 0x03) && (enc == 0x01)) {
			enc_mode = 0x00; 
			return 1;
		} 
	}//end of if
	else {return -1;}
	
	return 2; 
	
}//read_enc



//***********************************************************************************
//			bargraph display section
//***********************************************************************************
void bar_display() {
	
	//setting sys clk right now  
	if(set_sys_clock==0x01) {
		bar_graph = (bar_graph | 0x01); //keep alarm_armed if it's enable
	}
	else if(set_alarm == 0x01) {
		bar_graph = ((bar_graph & 0x00) | 0x02); //only show the bit 1
	}
	//alarm is set and waited for awaken
	else if(alarm_armed) {
		bar_graph = ((bar_graph & 0x00) | 0xC0); //only show the upper two bits
	}
	//if alarm is activated
	else if(uh_laam) {
		if(bar_graph == 0x00) {bar_graph = 0x01;}
		else                  {bar_graph = (bar_graph<<1);}
	}
	else {bar_graph = 0x00;}

	SPDR = bar_graph; //sents data to data register
	while(bit_is_clear(SPSR, SPIF)) {;} //waits until data sent out
	PORTD |= (1<<PD2);  //giving the rising edge for reg_clk
	PORTD &= ~(1<<PD2); //giving the falling edge for reg_clk
	//bar_graph = 0x00;

}//bar_display


//***********************************************************************************
//			Interrupt Service Routine(ISR)
//***********************************************************************************
volatile uint8_t count_7ms = 0; //holds 7ms tick count in binary
volatile uint8_t second = 0;
//volatile uint8_t minute = 0;
//volatile uint8_t hour = 0;
volatile uint8_t colon_toggle = 0b11111111;
volatile uint8_t count_alarm = 0; //holds 7ms alarm count in binary
//test value
volatile uint8_t minute = 50;
volatile uint8_t hour = 13;

ISR(TIMER0_OVF_vect) {
	count_7ms++;
	
	if((count_7ms%128) == 0) {	//1sec
		second += 1;
		if(second == 60) {second = 0; minute += 1;}
		if(minute == 60) {minute = 0; hour += 1;}
		if(hour == 24)   {hour = 0;}

		colon_toggle = (colon_toggle ^ 0b00000011); //colon toggles at 1 sec intervals
		count_7ms = 0; 

		//detect if sys clk time match alarm time
		if((hour==alarm_hour) && (minute==alarm_minute) &&(second==0)) {
			alarm_armed = 0x00; 	//alarm_armed is disable when alarm is awaken 
			music_on(); 		//not tested because hardware issue(8 ohms speaker)
			uh_laam = 0x01;	 	//use to stimulate bar_graph
		}
		bar_display();
	}//end of if (1sec)
	
	if(uh_laam) {	//counter of Beaver Fight Song
		count_alarm++;
		if((count_alarm%8) == 0) {
			beat++; //for note duration

			if(beat >= max_beat) {  //if we've played the note long enough
			notes++;               	//move on to the next note
			play_song(notes);	//and play it
			}
			count_alarm = 0; 	//reset count_alarm	
		}
	}
}//ISR(TIMER0_OVF_vect)

//***********************************************************************************

//LED clock: determine the segments that each digit should display  
void clock_display(uint8_t hours, uint8_t minutes, uint8_t colons) {
	
	uint8_t temp_hours;

	//minutes
	segment_data[0] = dec_to_7seg[(minutes%10)]; 
	segment_data[1] = dec_to_7seg[((minutes/10)%10)];
	
	//12 hour format
	if(hour_format) {
		if(hours>=13) {temp_hours = (hours%12);}
		else          {temp_hours = hours;} 

		segment_data[3] = dec_to_7seg[(temp_hours%10)];
		segment_data[4] = dec_to_7seg[((temp_hours/10)%10)];
	}

	//24 hour format
	else {
		segment_data[3] = dec_to_7seg[(hours%10)];
		segment_data[4] = dec_to_7seg[((hours/10)%10)];
	}

	//colon
	segment_data[2] = colons;
}//clock_display()


volatile uint8_t temp_alarm_hour;
volatile uint8_t temp_alarm_minute;

void button_func(uint8_t button_num) {

//buttons' function: only allow one button pressed at a time
	switch(button_num) {
		case 0: //12(0x01), 24(0x00) hour format toggle
			hour_format ^= 0x01;
			break;

		case 1: //lock(0x00), unlock(0x01) system clock
			if(set_alarm == 0x00) { //not allowed if set_alarm is enable
				set_sys_clock ^= 0x01;

				//store the value of hour&minute when unlock
				if(set_sys_clock) {temp_sys_hour = hour; temp_sys_minute = minute;}
				//return value and set new sys time 
				else              {hour = temp_sys_hour; minute = temp_sys_minute;}
			} 
			break;	

		case 2: //+5 hour (system clock) when sys clk is unlocked
			if(set_sys_clock == 0x01) {temp_sys_hour = ((temp_sys_hour+5)%24);}
			break;

		case 3: //+10 minute (system clock) when sys clk is unlocked
			if(set_sys_clock == 0x01) {temp_sys_minute = ((temp_sys_minute+10)%60);}
			break;

		case 4: //turn on and set alarm: lock(0x00), unlock(0x01) alarm time
			if(set_sys_clock == 0x00) { //not allowed if set_sys_clock is enable
				set_alarm ^= 0x01;
				
				//store the value of hour&minute when unlock, in order to change them
				if(set_alarm) {temp_alarm_hour = hour; temp_alarm_minute = minute;}
				else {
					alarm_hour = temp_alarm_hour; 
					alarm_minute = temp_alarm_minute;
					alarm_armed = 0x01; //alarm_armed enable when alarm is set
				}
			}
			break;

		case 5: //stop alarm 
			music_off(); //not test!!! hardware issue
			uh_laam = 0x00; //turn off bar_graph
			break;

		case 6: //+5 hour (alarm clock)
			if(set_alarm == 0x01) {temp_alarm_hour = ((temp_alarm_hour+1)%24);}
			break;	

		case 7: //+10 minute (alarm clock)
			if(set_alarm == 0x01) {temp_alarm_minute = ((temp_alarm_minute+1)%60);}
			break;

		default: break;
	}//end of switch
}//button_func()


//User Interface
void user_interface() {

	switch_ON(); //switch is enable
	
	//check buttons first
	for(uint8_t j=0; j<8; j++) {
		if(chk_buttons(j)) {
			button_func(j); //execute the pressed button func
		} 
	}//end of for(j)

	//read encoder if sys clk is unlocked(set_sys_clock == 0x01)
	if(set_sys_clock) {
		switch(read_enc()) {
			//counter-clockwise: default: -1 minute, 
			//if minute goes under 00, then -1 hour and minute starts at 59
			//ex: 01:00 -> 00:59, 00:00 -> 23:59
			case 0: 
				temp_sys_minute--;
				if(temp_sys_minute < 0) {temp_sys_minute = 59; temp_sys_hour -= 1;}
				if(temp_sys_hour < 0)  {temp_sys_hour = 23;}
				break;

			//clockwise: default: +1 minute; however, if minute overflow, +1 hour  
			case 1: 
				temp_sys_minute++;
				if(temp_sys_minute == 60) {temp_sys_minute = 0; temp_sys_hour += 1;}
				if(temp_sys_hour == 24)   {temp_sys_hour = 0;}
				break;
			
			//not spin
			case -1: 
				temp_sys_minute = temp_sys_minute;
				temp_sys_hour = temp_sys_hour;
				break;
			
			default: break;
		}//end of switch
	}//end of if(set_sys_clock)


	PORTB |= (1<<PB0); //set clk_inh as high 
	switch_OFF(); //switch is not enable

}//user_interface()

//Alarm generator
ISR(TIMER1_COMPA_vect) {
	PORTC ^= ALARM_PIN; //flips the bit to create a tone

	//if(beat >= max_beat) {     //if we've played the note long enough
	//	notes++;               //move on to the next note
	//	play_song(notes);//and play it
	//}


	//if(notes > 50) {
	//	notes = 0;
	//	beat = 0;
	//	max_beat = 0;
		//music_off();
		//_delay_ms(1);
		//music_on();
	//}

	//if(notes == -1) {
	//	notes = 0;
	//	play_song(notes);
	//} //alarm keeps working until you stop it 

}//ISR(TIMER1_COMPA_vect)


//***********************************************************************************
//				MAIN FUNCTION
//***********************************************************************************
int main() {

	tcnt_init(); //initial timer/count
  	spi_init();  //initialize spi port
  	lcd_init();
  	clear_display();

  	config_IDLE(); //setup I/O config	

  	sei(); //global interrupt enable

  	start_adc(); //poke ADSC and start conversion
  	alarm_init(); 

  	switch_OFF(); //enable led output
		
  	while(1) {

  		set_brightness();

  		user_interface();

  		if(set_sys_clock) {clock_display(temp_sys_hour, temp_sys_minute, colon_toggle);}
  		else if(set_alarm) {clock_display(temp_alarm_hour, temp_alarm_minute, colon_toggle);}
  		else {clock_display(hour, minute, colon_toggle);}	
  		
  		clear_segment();

  		for(uint8_t i=0; i<5; i++) {
			PORTA = segment_data[i]; //update the segment_data to PORTA
			PORTB = ((PORTB & ~(7<<4)) | (i<<4)); //only change bit4 ~ bit6; 	
			//PORTB = (j<<4); 	 //display all digit and start from digit_1  	
			_delay_ms(1);
			clear_segment();
 		}//end of for		

 		disable_tristate();
  	}//while
  	return 0;
}//main


//config of timer_counter
void tcnt_init(void) {
	//tcnt0
	ASSR  |= (1<<AS0); 	//ext osc TOSC
	TIMSK |= (1<<TOIE0); 	//enable timer/counter0 overflow interrupt
	TCCR0 |= (1<<CS00); 	//normal mode, no prescale

	//tcnt1, CTC mode
	TCCR1A |= 0x00;       //normal port operation
	TCCR1B |= (1<<WGM12); //CTC mode, ?? prescale
	TCCR1C |= 0x00;       //no forced compare
	OCR1A   = 0x0031;     //vary alarm freqz
	TIMSK  |= (1<<OCIE1A);//Output compare a match interrupt enable

	//tcnt2, fast PWM
	TCCR2 = (1<<WGM21) | (1<<WGM20) | (1<<COM21) | (1<<CS21) | (1<<CS20);
}


// IDLE state: LED displays single 0 only  
void config_IDLE(void) {
	DDRA  = 0xFF;	//PORTA is in output mode  
	DDRB |= (1<<PB7) | (1<<PB6) | (1<<PB5) | (1<<PB4); //PORTB upper 4 bit is in output mode 
	//PORTA = 0xFF;	//pull-up, and it is active low.

	DDRD  |= (1<<PD2); //REG_CLK
	DDRE  |= (1<<PE6); //SH_LD
	PORTE |= (1<<PE6); //SH_LD: high is default because it is active low

	//config of ADC
	DDRF  &= ~(_BV(DDF7)); 	//make port F bit 7 the ADC input
	PORTF &= ~(_BV(PF7));	//port F bit 7 pullups must be off

	//single-ended input, PORT F bit7, right adjusted, 10 bits 
	ADMUX  = (1<<MUX2) | (1<<MUX1) | (1<<MUX0) | (1<<REFS0);	
	
	//ADC enabled, don't start yet, single shot mode
	//ADC Interrupt Enable: ADC conversion interrupt is activated
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0) | (1<<ADIE);

	//config of alarm
	DDRC |= ALARM_PIN; //AMP
}//config_IDLE


//***********************************************************************************
//			configuration of switch
//***********************************************************************************
void switch_ON() {
  	//make PORTA an input port with pullups 
	DDRA = 0x00; 
	PORTA = 0xFF; 
	
  	//enable tristate buffer for pushbutton switches
	PORTB |= (1<<PB4) | (1<<PB5) | (1<<PB6);
	PORTB |= (1<<PB0); //also activate the clk_inh
}//switch_ON

void switch_OFF() {
	//make PORTA an output with pullups
	//PORTA = 0xFF;
	DDRA = 0xFF;

	//disable tri-state buffer

	//----------------------------
	//PORTB		xxxx_xxxx
	//~(7<<4)    & 	1000_1111
	//----------------------------
	//		x000_xxxx
	//(5<<4)     |  0101_0000
	//----------------------------
	//PORTB         x101_xxxx (x:don't care) 
	PORTB = ((PORTB & ~(7<<4)) | (5<<4)); 
	//PORTB &= ~((1<<PB4) | (1<<PB5) | (1<<PB6)); 
}//switch_OFF

void disable_tristate() {PORTB = ((PORTB & ~(7<<4)) | (5<<4));}
void clear_segment() {PORTA = 0xFF;}
