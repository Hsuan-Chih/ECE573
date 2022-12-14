// heartbeat.c
// setup TCNT1 in PWM mode 
// setup TCNT3 in normal mode 
// set OC1A (PB5) as pwm output 
// pwm frequency:  (16,000,000)/(1 * (61440 + 1)) = 260hz
//

#include <avr/io.h>
#include <avr/interrupt.h>

uint16_t brightness[20] = {0x0100, 0x2000, 0x7000, 0xA000, 0xC000, 0xD000, 0xD0EE, 0xDF00, 0xE000, 0xEF00, 0xEF00, 0xE000, 0xDF00, 0xD0EE, 0xD000, 0xC000, 0xA000, 0x7000, 0x2000, 0x0100};

ISR(TIMER3_OVF_vect) {
  static uint8_t index=0;    //steps through the array 
  OCR1A = brightness[index];  //set bounds on index
  index++;  
  if(index == 20) {index = 0;}//sset OCR1A to new value
}

int main() {
  // setup TCNT1 in PWM mode 
  //set PORTB bit 5 as the PWM output
  DDRB   = 0x20;          
  //fast PWM, set on match, clear at top, ICR1 holds value of TOP 
  TCCR1A |= (1<<COM1A1) | (1<<COM1A0) | (1<<WGM11);
  //use ICR1 as source for TOP, use clk/1
  TCCR1B |= (1<<WGM13) | (1<<WGM12) | (1<<CS10);
  //no forced compare 
  TCCR1C = 0x00;
  //clear at 0xF000                               
  ICR1  = 0xF000;
  
// setup TCNT3 in normal mode to control the update rate 
// heartbeat update frequency = (16,000,000)/(8 * 2^16) = 30 cycles/sec
  //normal mode
  TCCR3A = 0x00;
  //use clk/8  (30hz)  
  TCCR3B = (1<<CS31);
  //no forced compare 
  TCCR3C = 0x00;
  //enable timer 3 interrupt on TOV
  ETIMSK = (1<<TOIE3);

  sei();  //set GIE
  while(1) {}; //loop forever waiting for interrupt
 
}  // main
