// lab1_code.c 
// R. Traylor
// 7.13.20

//This program increments a binary display of the number of button pushes on switch 
//S0 on the mega128 board.

#include <avr/io.h>
#include <util/delay.h>

//*******************************************************************************
//                            debounce_switch                                  
// Adapted from Ganssel's "Guide to Debouncing"            
// Checks the state of pushbutton S0 It shifts in ones till the button is pushed. 
// Function returns a 1 only once per debounced button push so a debounce and toggle 
// function can be implemented at the same time. Expects active low pushbutton on 
// Port D bit zero. Debounce time is determined by external loop delay times 12. 
//*******************************************************************************

//check if button1(pin0) is pressed
int8_t debounce_switch_S1() {
  static uint16_t state_S1 = 0; //holds present state
  state_S1 = (state_S1 << 1) | (! bit_is_clear(PIND, 0)) | 0xE000;
  if (state_S1 == 0xF000) return 1;
  return 0;
}

//check if button2(pin1) is pressed
int8_t debounce_switch_S2() {
  static uint16_t state_S2 = 0; //holds present state
  state_S2 = (state_S2 << 1) | (! bit_is_clear(PIND, 1)) | 0xE000;
  if (state_S2 == 0xF000) return 1;
  return 0;
}

//check if both buttons are pressed
int8_t both_pressed() {
  if(bit_is_clear(PIND, 0) & bit_is_clear(PIND, 1)) return 1;
  return 0;
}

//*******************************************************************************
// Check switch S0.  When found low for 12 passes of "debounce_switch(), increment
// PORTB. This will make an incrementing count on the port B LEDS. 
//*******************************************************************************
int main() {

DDRD = 0xFC;  //set bit0 and bit1 in port D as inputs
DDRB = 0xFF;  //set port B to all outputs
PORTB = 0x00; //set as the counter, which starts from 0x00

while(1){     //do forever
 //both buttons are pressed, not to increase the counter
 if(both_pressed()) {PORTB = PORTB;} 
 
 //button1 is pressed, increasing the counter 
 else if(debounce_switch_S1()) {
   if(PORTB == 0xFF) {PORTB = 0x00;} //roll over after 0xFF to 0x00
   else {PORTB++;}
 }
 
 //button2 is pressed, decreasing the counter
 else if(debounce_switch_S2()) {
   if(PORTB == 0x00) {PORTB = 0xFF;} //roll under to 0xFF from 0x00
   else {PORTB--;}
 }

  _delay_ms(2); //keep in loop to debounce 24ms
  } //while 
} //main

