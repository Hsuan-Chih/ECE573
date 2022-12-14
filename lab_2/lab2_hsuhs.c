// lab2_skel.c 
// Hsuan-Chih Hsu
// 10.14.22

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
	segment_data[2] = dec_to_7seg[11]; //colon is also not available
	
}//segment_sum
//***********************************************************************************


// IDLE state: LED displays single 0 only  
void config_IDLE() {
	DDRA = 0xFF;		//PORTA is in output mode  
	DDRB = 0xF0;		//PORTB upper 4 bit is in output mode 
	PORTA = 0xFF;		//pull-up, and it is active low.
  
	//dispay digit0 as 0
	PORTB = 0x00;	
	PORTA = dec_to_7seg[0];
}

//configuration of switch
void switch_ON();
void switch_OFF();

//***********************************************************************************
int main() {

  //IDLE
  uint16_t count = 0x0000; //setup a counter
  config_IDLE();	   //setup I/O config	
			   
  while(1) {
    	switch_ON(); //config of switch_ON 	  

  	//insert loop delay for debounce
	for(uint8_t i=0; i<8; i++) {
  		//now check each button and increment the count as needed
		if(chk_buttons(i)) {count += (1<<i);}
		_delay_us(500);
	}
	
	
	//make sure no buttons are pressed
  	//if(PINA == 0xFF) {

		switch_OFF(); //disable tristate buffer for pushbutton switches

		//bound the count to 0 - 1023
  		if(count > 1023) {count = 0;}
	
		//break up the disp_value to 4, BCD digits in the array: call (segsum)
  		segsum(count);
	
		for(uint8_t j=0; j<5; j++) {
			PORTA = segment_data[j]; //update the segment_data to PORTA 	
			PORTB = (j<<4); 	 //display all digit and start from digit_1  	
			_delay_us(700);
	 	}//end of for

	//}//end of if(PINA == 0xFF)

  }//while
  return 0;
}//main

void switch_ON() {
  	//make PORTA an input port with pullups 
	DDRA = 0x00; 
	PORTA = 0xFF; 
	
  	//enable tristate buffer for pushbutton switches
	PORTB |= (1<<PB4) | (1<<PB5) | (1<<PB6);
}

void switch_OFF() {
	//make PORTA an output with pullups
	PORTA = 0xFF;
	DDRA = 0xFF;

	PORTB = 0X00; //disable tri-state buffer
}
