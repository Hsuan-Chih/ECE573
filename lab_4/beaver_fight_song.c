//Using code from Roger's webpage
#include <avr/io.h>
#include <string.h>
#include "beaver_fight_song.h"

volatile int16_t beat;
volatile uint16_t max_beat;
volatile int8_t notes;
#define ALARM_PIN 0x10

void alarm_init(void) {
   music_off();
   beat = 0;
   max_beat = 0;
   notes = 0;
}

void music_off(void) {
  //this turns the alarm timer off
  notes = 0;
  beat = 0;
  max_beat = 0;
  TCCR1B &= ~((1<<CS11)|(1<<CS10));
  //PORTC &= ~(ALARM_PIN);
}

void music_on(void) {
  //this starts the alarm timer running
  notes=0;
  TCCR1B |= (1<<CS11)|(1<<CS10);
  
  //and starts the selected song
  play_song(notes);
}


void beaver_song(uint16_t note) { 
  switch (note) {
    case 0: play_note('F', 0, 4, 8);
       break;
    case 1: play_note('E', 0, 4, 8);
       break;
    case 2: play_note('D', 0, 4, 8);
       break;
    case 3: play_note('C', 0, 4, 8);
       break;
    case 4: play_note('A', 0, 4, 6);
       break;
    case 5: play_note('A', 1, 4, 2);
       break;
    case 6: play_note('A', 0, 4, 6);
       break;
    case 7: play_note('A', 1, 4, 2);
       break;
    case 8: play_note('A', 0, 4, 16);
       break;
    case 9: play_note('F', 0, 4, 8);
       break;
    case 10: play_note('E', 0, 4, 8);
       break;
    case 11: play_note('D', 0, 4, 8);
       break;
    case 12: play_note('C', 0, 4, 8);
       break;
    case 13: play_note('B', 1, 4, 6);
       break;
    case 14: play_note('A', 0, 4, 2);
       break;
    case 15: play_note('B', 1, 4, 6);
       break;
    case 16: play_note('A', 0, 4, 2);
       break;
    case 17: play_note('B', 1, 4, 16);
       break;
    case 18: play_note('G', 0, 4, 3);
       break;
    case 19: play_rest(1); //rest
       break;
    case 20: play_note('G', 0, 4, 7);
       break;
    case 21: play_rest(1); //rest
       break;
    case 22: play_note('G', 1, 4, 4);
       break;
    case 23: play_note('G', 0, 4, 6);
       break;
    case 24: play_note('A', 0, 4, 2);
       break;
    case 25: play_note('B', 1, 4, 8);
       break;
    case 26: play_note('A', 0, 4, 2);
       break;
    case 27: play_rest(2); 
       break;
    case 28: play_note('A', 0, 4, 8);
       break;
    case 29: play_note('A', 1, 4, 4);
       break;
    case 30: play_note('A', 0, 4, 6);
       break;
    case 31: play_note('B', 1, 4, 2);
       break;
    case 32: play_note('C', 0, 5, 4);
       break;
    case 33: play_note('D', 1, 5, 4);
       break;
    case 34: play_note('D', 0, 5, 4);
       break;
    case 35: play_note('B', 0, 4, 8);
       break;
    case 36: play_note('A', 0, 4, 4);
       break;
    case 37: play_note('G', 0, 4, 8);
       break;
    case 38: play_note('A', 0, 4, 8);
       break;
    case 39: play_note('G', 0, 4, 24);
       break;
    case 40: play_rest(8);
       break;
    case 41: play_note('F', 0, 4, 8);
       break;
    case 42: play_note('E', 0, 4, 8);
       break;
    case 43: play_note('D', 0, 4, 8);
       break;
    case 44: play_note('C', 0, 4, 8);
       break;
    case 45: play_note('A', 0, 4, 6);
       break;
    case 46: play_note('A', 1, 4, 2);
       break;
    case 47: play_note('A', 0, 4, 6);
       break;
    case 48: play_note('A', 1, 4, 2);
       break;
    case 49: play_note('A', 0, 4, 16);
       break;
    case 50: play_note('F', 0, 4, 8);
       break;
    case 51: play_note('G', 1, 4, 8);
       break;
    case 52: play_note('G', 0, 4, 8);
       break;
    case 53: play_note('D', 0, 4, 8);
       break;
    case 54: play_note('B', 1, 4, 6);
       break;
    case 55: play_note('A', 0, 4, 2);
       break;
    case 56: play_note('B', 1, 4, 6);
       break;
    case 57: play_note('A', 0, 4, 2);
       break;
    case 58: play_note('B', 1, 4, 16);
      break;//phrase
    case 59: play_note('D', 0, 4, 16);
       break;
    case 60: play_note('D', 0, 5, 16);
       break;
    case 61: play_note('A', 0, 4, 16);
       break;
    case 62: play_note('C', 0, 5, 16);
       break;
    case 63: play_note('B', 1, 4, 8);
       break;
    case 64: play_note('C', 0, 5, 4);
       break;
    case 65: play_note('D', 0, 5, 4);
       break;
    case 66: play_note('A', 0, 4, 8);
       break;
    case 67: play_note('G', 0, 4, 8);
       break;
    case 68: play_note('F', 0, 4, 24);
       break;
    case 69: play_rest(8);
       break;
    case 70:
      notes = -1;
      beat = -1;
      max_beat = 0;
      break;
    default: notes = -1;
  }
}//beaver_song

void play_song(uint8_t note) {beaver_song(note);} //beaver fight song

void play_rest(uint8_t duration) {
  //mute for duration
  //duration is in 64th notes at 120bpm
  //PORTD |= mute;:wq
  beat = 0;
  max_beat = duration;
  OCR1A=0x0000;
}

void play_note(char note, uint8_t flat, uint8_t octave, uint8_t duration) {
  //pass in the note, it's key, the octave they want, and a duration
  //function sets the value of OCR1A and the timer
  //note must be A-G
  //flat must be 1 (for flat) or 0 (for natural) (N/A on C or F)
  //octave must be 0-8 (0 is the lowest, 8 doesn't sound very good)
  //duration is in 64th notes at 120bpm
  //e.g. play_note('D', 1, 0, 16)
  //this would play a Db, octave 0 for 1 quarter note
  //120 bpm (every 32ms inc beat)
  //PORTD &= unmute;      //unmute (just in case)
  beat = 0;             //reset the beat counter
  max_beat = duration;  //set the max beat
  switch (octave) {
    case 0: 
      switch (note) {
      	case 'A': 
      		if(flat) {OCR1A=Ab0;}
        	else     {OCR1A=A0;}
   			break;
      	case 'B': 
      		if(flat) {OCR1A=Bb0;}
        	else     {OCR1A=B0;}
   			break;
      	case 'C': 
      		OCR1A=C0;
   			break;
      	case 'D': 
      		if(flat) {OCR1A=Db0;}
        	else     {OCR1A=D0;}
   			break;
      	case 'E': 
      		if(flat) {OCR1A=Eb0;}
        	else     {OCR1A=E0;}
   			break;
      	case 'F': 
      		OCR1A=F0;
   			break;
      	case 'G': 
      		if(flat) {OCR1A=Gb0;}
        	else     {OCR1A=G0;}
   			break;
      } 
      break;
    case 1: 
      switch (note) {
      	case 'A': 
      		if(flat) {OCR1A=Ab1;}
        	else     {OCR1A=A1;}
   			break;
      	case 'B': 
      		if(flat) {OCR1A=Bb1;}
        	else     {OCR1A=B1;}
   			break;
      	case 'C': 
      		OCR1A=C1;
   			break;
      	case 'D': 
      		if(flat) {OCR1A=Db1;}
        	else     {OCR1A=D1;}
   			break;
      	case 'E': 
      		if(flat) {OCR1A=Eb1;}
        	else     {OCR1A=E1;}
   			break;
      	case 'F': 
      		OCR1A=F1;
   			break;
      	case 'G': 
      		if(flat) {OCR1A=Gb1;}
        	else     {OCR1A=G1;}
   			break;
      } 
      break;
    case 2: 
      switch (note) {
      	case 'A': 
      		if(flat) {OCR1A=Ab2;}
        	else     {OCR1A=A2;}
   			break;
      	case 'B': 
      		if(flat) {OCR1A=Bb2;}
        	else     {OCR1A=B2;}
   			break;
      	case 'C': 
      		OCR1A=C2;
   			break;
      	case 'D': 
      		if(flat) {OCR1A=Db2;}
        	else {OCR1A=D2;}
   			break;
      	case 'E': 
      		if(flat) {OCR1A=Eb2;}
        	else     {OCR1A=E2;}
   			break;
      	case 'F': 
      		OCR1A=F2;
   			break;
      	case 'G': 
      		if(flat) {OCR1A=Gb2;}
        	else     {OCR1A=G2;}
   			break;
      } 
      break;
    case 3: switch (note) {
      case 'A': if(flat){OCR1A=Ab3;}
        else {OCR1A=A3;}
   break;
      case 'B': if(flat){OCR1A=Bb3;}
        else {OCR1A=B3;}
   break;
      case 'C': OCR1A=C3;
   break;
      case 'D': if(flat){OCR1A=Db3;}
        else {OCR1A=D3;}
   break;
      case 'E': if(flat){OCR1A=Eb3;}
        else {OCR1A=E3;}
   break;
      case 'F': OCR1A=F3;
   break;
      case 'G': if(flat){OCR1A=Gb3;}
        else {OCR1A=G3;}
   break;
      } 
      break;
    case 4: switch (note) {
      case 'A': if(flat){OCR1A=Ab4;}
        else {OCR1A=A4;}
   break;
      case 'B': if(flat){OCR1A=Bb4;}
        else {OCR1A=B4;}
   break;
      case 'C': OCR1A=C4;
   break;
      case 'D': if(flat){OCR1A=Db4;}
        else {OCR1A=D4;}
   break;
      case 'E': if(flat){OCR1A=Eb4;}
        else {OCR1A=E4;}
   break;
      case 'F': OCR1A=F4;
   break;
      case 'G': if(flat){OCR1A=Gb4;}
        else {OCR1A=G4;}
   break;
      } 
      break;
    case 5: switch (note) {
      case 'A': if(flat){OCR1A=Ab5;}
        else {OCR1A=A5;}
   break;
      case 'B': if(flat){OCR1A=Bb5;}
        else {OCR1A=B5;}
   break;
      case 'C': OCR1A=C5;
   break;
      case 'D': if(flat){OCR1A=Db5;}
        else {OCR1A=D5;}
   break;
      case 'E': if(flat){OCR1A=Eb5;}
        else {OCR1A=E5;}
   break;
      case 'F': OCR1A=F5;
   break;
      case 'G': if(flat){OCR1A=Gb5;}
        else {OCR1A=G5;}
   break;
      } 
      break;
    case 6: switch (note) {
      case 'A': if(flat){OCR1A=Ab6;}
        else {OCR1A=A6;}
   break;
      case 'B': if(flat){OCR1A=Bb6;}
        else {OCR1A=B6;}
   break;
      case 'C': OCR1A=C6;
   break;
      case 'D': if(flat){OCR1A=Db6;}
        else {OCR1A=D6;}
   break;
      case 'E': if(flat){OCR1A=Eb6;}
        else {OCR1A=E6;}
   break;
      case 'F': OCR1A=F6;
   break;
      case 'G': if(flat){OCR1A=Gb6;}
        else {OCR1A=G6;}
   break;
      } 
      break;
    case 7: switch (note) {
      case 'A': if(flat){OCR1A=Ab7;}
        else {OCR1A=A7;}
   break;
      case 'B': if(flat){OCR1A=Bb7;}
        else {OCR1A=B7;}
   break;
      case 'C': OCR1A=C7;
   break;
      case 'D': if(flat){OCR1A=Db7;}
        else {OCR1A=D7;}
   break;
      case 'E': if(flat){OCR1A=Eb7;}
        else {OCR1A=E7;}
   break;
      case 'F': OCR1A=F7;
   break;
      case 'G': if(flat){OCR1A=Gb7;}
        else {OCR1A=G7;}
   break;
      } 
      break;
    case 8: switch (note) {
      case 'A': if(flat){OCR1A=Ab8;}
        else {OCR1A=A8;}
   break;
      case 'B': if(flat){OCR1A=Bb8;}
        else {OCR1A=B8;}
   break;
      case 'C': OCR1A=C8;
   break;
      case 'D': if(flat){OCR1A=Db8;}
        else {OCR1A=D8;}
   break;
      case 'E': if(flat){OCR1A=Eb8;}
        else {OCR1A=E8;}
   break;
      case 'F': OCR1A=F8;
   break;
      case 'G': if(flat){OCR1A=Gb8;}
        else {OCR1A=G8;}
   break;
      } 
      break;
   default: OCR1A=0x0000;
  }
}

