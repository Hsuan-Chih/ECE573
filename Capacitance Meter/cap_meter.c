// cap_meter.c
// Hsuan-Chih Hsu
// 12.09.22

// Code to implement capacitance meter with Mega128.
// Implemented as integrating quantizer.

//compute a multiplicitive factor to make code cleaner. This is
//simply a cranking of the equation for current through a cap
//given the dv, Ic, and bandgap reference voltage.
#define DtoC_FACTOR 0.22358    //5V wall wart supply for accuracy

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "hd44780.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//global variable with LCD text, so ISR can change it
char lcd_message[14] = {"xxxx.x nF cap"};
char lcd_str_h[16]; //hold string to send to lcd
char lcd_str_l[16]; //hold string to send to lcd

/*****************************************************************************/
//SPI initalization for LCD display
void spi_init(void) {
    DDRB  |= 0x07; //Turn on SS_n, MOSI, SCLK. SS_n must be out for MSTR mode

    //Master mode, Clock=clk/2, Cycle half phase, low polarity, MSB first
    SPCR = (1<<SPE) | (1<<MSTR); //Enable SPI, clk low initailly, rising edge sample
    SPSR = (1<<SPI2X);           //SPI at 2x speed (8 MHz)
}
/*****************************************************************************/

/*****************************************************************************/
//Comparator init
//Use bandgap reference (1.23V), Input capture trigger for TCNT1 enabled 
void acomp_init(void){
    ACSR = (1<<ACBG) | (1<<ACIC);
}
/*****************************************************************************/

/*****************************************************************************/
//TCNT3_init
//Run in normal mode. Used to set the refresh rate for the LCD display.
//Refresh rate shold be about 4mS. Does not generate interrupts. Poll for
//interrupt flag.
void tcnt3_init(void){
    TCCR3A = 0x00; //normal mode
    TCCR3B = (1<<CS32); //refresh rate = (2^16 * 1)/(16,000,000) = 4.096ms/cycle
    TCCR3C = 0x00; //no forced compare
}
/*****************************************************************************/

/*****************************************************************************/
//TCNT1_init
//Use in normal mode. Suggest no pre-scaler. Start up initially with counter off.
//Must enable TCNT1 input capture and overflow interrupts.
//
void tcnt1_init(void){
    TCCR1A = 0x00; //normal mode
    TCCR1B |= (0<<CS12) | (0<<CS11) | (0<<CS10); //no prescaler, counter stopped
    TCCR1C = 0x00; //no forced compare

    //input capture interrupt enable, overflow interrupt enable
    TIMSK |= (1<<TICIE1) | (1<<TOIE1); 
}
/*****************************************************************************/

/*****************************************************************************/
//Input capture register ISR
//When input capture register interrupt happens: 
//  - read counter 1 value
//  - disable counter
//  - convert to cap value
//  - fill in the LCD message string only 
//
volatile uint16_t val_cnt1; //read counter 1 value
volatile div_t val_cap, val_cap_low; //double val_cap
volatile float too_large_cap;


ISR(TIMER1_CAPT_vect){
    val_cnt1 = ICR1;                     //read counter 1 value
    TCCR1B &= ~(1<<CS10);                //timer/counter stopped
    TCNT1 = 0; //reset the counter
    
    val_cap = div((DtoC_FACTOR*val_cnt1), 10); //(DtoC_FACTOR*val_cnt1)/10 
    itoa(val_cap.quot, lcd_str_h, 10);         //convert non-fractional part to ascii string
    val_cap_low = div((val_cap.rem*10), 10);   //get the decimal fraction into non-fractional form
    itoa(val_cap_low.quot, lcd_str_l, 10);     //convert fractional part to ascii string

    too_large_cap = ((float)DtoC_FACTOR*val_cnt1)/10; //convert to cap value
} 

/*****************************************************************************/

/*****************************************************************************/
//                              timer1 overflow ISR
//If TCNT1 overflows, before the analog comparator triggers, disable counter
// and display "----.-" to LCD
//
ISR(TIMER1_OVF_vect){
    val_cnt1 = 0xFFFF; 
    TCCR1B &= ~(1<<CS10); //timer/counter stopped
    strcpy(lcd_str_h, "----");
    strcpy(lcd_str_l, "-");
    
    //test if this ISR work
    PORTB ^= (1<<PB7);
}
/*****************************************************************************/

/*****************************************************************************/
int main(){
    DDRB |= 0x01;                //setup PORTB.0 LED for blinking
    //test
    DDRB |= 0x80;


    DDRF |= 0x08; PORTF &= 0xF7; //setup PORTF.3 to clock LCD    
    DDRE  = 0x00;                //set PE2,3 appropriately
    spi_init();    //initalize SPI
    tcnt1_init();  //initialize counter/timer one
    tcnt3_init();  //initialize counter/timer three
    acomp_init();  //initialize analog comparator
    _delay_us(70); //wait enough time for bandgap reference to startup 
    lcd_init();    //initialize the LCD
    sei();         //enable interrupts

    while(1){
        //if TCNT3 overflowed
        if (ETIFR & (1<<TOV3)){   
            ETIFR |= (1<<TOV3);    //clear overflow bit for next measurement
            PORTB ^= (1<<PB0);     //toggle B0 to see that the meter is running
            
	    clear_display();	   //need enough time to clear LCD display 
	    
	    TCNT1 = 0;             //ensure that TCNT1 starts at zero to time the charge interval
            DDRE |= (1<<PE2);      //make PE2 an output to discharge cap
            _delay_ms(5.1);        //delay enough to discharge the cap 
            TCCR1B |= (1<<CS10);   //start TC1 counter, no prescaling (62.5nS/tick)
            DDRE &= ~(_BV(DDE2));  //change PE2 back to high-Z (input) to allow charging cap
            
            //write string to LCD; message is created in the ISR
	    if(too_large_cap == 0) {
	   	string2lcd("----.- nF cap"); 
	    }
	    else{
	    	string2lcd(lcd_str_h); //write upper half
	    	char2lcd('.');         //write decimal point
            	string2lcd(lcd_str_l); //write lower half
            	string2lcd(" nF cap");
	    }
            cursor_home(); //put the cursor back to home
        }//if
    }//while
}//main
