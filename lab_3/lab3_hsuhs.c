//  lab3
//  Hsuan-Chih Hsu
//  10.26.2022

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

// Port B:  bit4 green  seg0
//          bit5 blue   seg1
//          bit6 purple seg2
//          bit7 grey   pwm 
//               black  Vdd
//               white  Vss

#define TRUE 1
#define FALSE 0

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//Global variables
volatile int16_t count = 0x0000; //setup a counter
volatile uint8_t mode = 0x00; // default mode
volatile uint8_t num_mode;
volatile uint8_t bar_graph = 0x00; //default bargraph
volatile uint8_t enc_serial_output;
volatile uint8_t enc;
volatile uint8_t prev_enc;
volatile uint8_t enc_mode;

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
  	if (state[button] == 0xF000) return 1;
  	return 0;
}
//******************************************************************************


//***********************************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|
void segsum(uint16_t sum) {
  	uint16_t temp = sum; //use temporary variable to keep input

	//calculate all segment_data by using mod and divide
  	//break up decimal sum into 4 digit-segments
	segment_data[0] = dec_to_7seg[(temp%10)]; //digit0
	temp /= 10;
	segment_data[1] = dec_to_7seg[(temp%10)]; //digit1
	temp /= 10;
	segment_data[3] = dec_to_7seg[(temp%10)]; //digit2 
	temp /= 10;
	segment_data[4] = dec_to_7seg[(temp%10)]; //digit3

  	//blank out leading zero digits 
  	//now move data to right place for misplaced colon position
	if(sum < 1000) {segment_data[4] = dec_to_7seg[11];} //dec_to_7seg[11] is 0xFF
	if(sum < 100)  {segment_data[3] = dec_to_7seg[11];} //dec_to_7seg[11] is 0xFF
	if(sum < 10)   {segment_data[1] = dec_to_7seg[11];} //dec_to_7seg[11] is 0xFF
	
	//segment_data[2] = dec_to_7seg[11]; //colon is also not available
	if(enc_mode == 0x00) {segment_data[2] = 0b11111110;}
	if(enc_mode == 0x01) {segment_data[2] = 0b11111101;}
}//segment_sum
//***********************************************************************************


//***********************************************************************************
//															configuration of switch
//***********************************************************************************
void switch_ON() {
  //make PORTA an input port with pullups 
	DDRA = 0x00; 
	PORTA = 0xFF; 
	
  //enable tristate buffer for pushbutton switches
	PORTB |= (1<<PB4) | (1<<PB5) | (1<<PB6) | (1<<PB0); //also activate the clk_inh
}//switch_ON

void switch_OFF() {
	//make PORTA an output with pullups
	PORTA = 0xFF;
	DDRA = 0xFF;

	PORTB &= ~((1<<PB4) | (1<<PB5) | (1<<PB6)); //disable tri-state buffer
}//switch_OFF

//***********************************************************************************
//																	spi_init
//Initializes the SPI port on the mega128. 
//***********************************************************************************
void spi_init() {
	DDRB = 0x07; 								 //Turn on SS, MOSI, SCLK (SS is output)
	SPCR = (1<<SPE) | (1<<MSTR); //SPI enabled, master, low polarity, MSB 1st
  SPSR = (1<<SPI2X); 					 //run at i/o clock/2
}//spi_init

//***********************************************************************************


//***********************************************************************************
//																	spi_read
//Reads the spi port
//**********************************************************************************
uint8_t spi_read() {
	SPDR = 0x00; 											  //dummy write to SPDR
	while(bit_is_clear(SPSR, SPIF)) {;} //waits until 8 clock cycles are done
	return(SPDR);												//returns incoming data from SPDR
}//spi_read

//***********************************************************************************


//***********************************************************************************
// IDLE state: LED displays single 0 only  
//***********************************************************************************
void config_IDLE() {
	DDRA = 0xFF;																			 //PORTA is in output mode  
	DDRB |= (1<<PB7) | (1<<PB6) | (1<<PB5) | (1<<PB4); //PORTB upper 4 bit is in output mode 
	PORTA = 0xFF;																			 //pull-up, and it is active low.
  
	//dispay digit0 as 0
	//PORTB = 0x00; 
	PORTA = dec_to_7seg[0];

	DDRD |= (1<<PD2); //REG_CLK
	DDRE |= (1<<PE6); //SH_LD
	PORTE |= (1<<PE6); //SH_LD: high is default because it is active low
}
//***********************************************************************************

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
	PORTE &= ~(1<<PE6); //sets falling edge for sh_ld
	//_delay_ms(1);				//gives delay to read the parallel inputs 
	PORTE |= (1<<PE6); 	//rising edge for sh_ld

	PORTB &= ~(1<<PB0); //set clk_inh as low to read the encoder
	enc_serial_output = spi_read(); //read the encoder serial output 

	//only take the first two bit to compare
	enc = enc_serial_output & 0x03;
	prev_enc = prev_enc & 0x03;

	//detect if enc is changes, if so encoder is activated 
	if(prev_enc != enc) {
		if((prev_enc == 0x03) && (enc == 0x02)) {enc_mode = 0x01; return 0;} //counter-clockwise: decrement
		else if((prev_enc == 0x03) && (enc == 0x01)) {enc_mode = 0x00; return 1;} //clockwise: increment
	}//end of if
	else {return -1;}
	
	return 2; 
	
}//read_enc



//***********************************************************************************


//			bargraph display section
//thermometer code
void bar_display() {
	
	for(uint8_t a=0; a<=(count/128); a++) {bar_graph |= (1<<a);}
	SPDR = bar_graph; //sents data to data register
	while(bit_is_clear(SPSR, SPIF)) {;} //waits until data sent out
	PORTD |= (1<<PD2);  //giving the rising edge for reg_clk
	PORTD &= ~(1<<PD2); //giving the falling edge for reg_clk
	bar_graph = 0x00;

}//bar_display


//***********************************************************************************
//			Interrupt Service Routine(ISR)
// Step 1: read button_1 & button_2 to determine the "MODE" in binary: 00, 01, 10, 11
// E.g., Mode 00, the default for increase or decrease by "1" 
// 			 Mode 01, button_1 is pressed => increase or decrease by "2"
// 			 Mode 10, button_2 is pressed => increase or decrease by "4"
//			 Mode 11, both buttons are pressed => stop counting
// Step 2: read the encoder to determine "INCREMENT" or "DECREMENT" 
// Detail: the output of read_enc(): 0 => ccw, 1 => cw, -1 => not spin
// Step 3: calculate "COUNT"
//***********************************************************************************
ISR(TIMER0_OVF_vect) {

	switch_ON(); //config of switch_ON 
	
	//"mode" selection: 0x00, 0x01, 0x02, 0x03
	for(volatile uint8_t i=0; i<2; i++) {
		if(chk_buttons(i)) {mode ^= (1<<i);}
	}//end of for loop
	
	//assign the value to the mode
	switch(mode) {
		case 0x00:	num_mode = 1; break;
		case 0x01: 	num_mode = 2; break;
		case 0x02: 	num_mode = 4; break;
		case 0x03:	num_mode = 0; break;
		default: break;
	}//end of switch
	

	//read the encoder
	//calculatiion of "count", based on the direction of encoder
	switch(read_enc()) {
		//ccw
		case 0:	count -= num_mode; break;
		//cw
		case 1: count += num_mode; break;
		//not spin
		case -1: count = count;
		default: break;
	}//end of switch

	PORTB |= (1<<PB0); //set clk_inh as high 
	switch_OFF(); 	   //disable tristate buffer for pushbutton switches
	

	if(count > 1023) {count = 0;} //next count will be 0, if roll over at 1023
	if(count < 0) {count = 1023;} //next count will be 1023, if decremented beyond 0
	
	bar_display(); //display bargraph (thermometer code)
		
  	segsum(count);
	for(volatile uint8_t j=0; j<5; j++) {
		PORTA = segment_data[j]; //update the segment_data to PORTA 	
		PORTB = (j<<4); 	 //display all digit and start from digit_1  	
		_delay_ms(1);
 	}//end of for
}//ISR

//***********************************************************************************


//***********************************************************************************
// 																MAIN FUNCTION
//***********************************************************************************
int main() {

	//timer counter 0 setup, running off i/o clock
  	TIMSK |= (1<<TOIE0);             //enable interrupts
  	TCCR0 |= (1<<CS02) | (1<<CS00);  //normal mode, prescale by 128

  	spi_init(); 			 //initialize spi port
  	config_IDLE();	   		 //setup I/O config	

  	sei(); //global interrupt enable
			   
  	while(1) {;}//while
  	return 0;
}//main

