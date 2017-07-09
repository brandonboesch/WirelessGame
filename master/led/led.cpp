// Filename: led.cpp
// Built by: Brandon Boesch
// Date    : June 18th, 2017

#include "mbed.h"
#include "led.h"

Ticker TickerLED;                 // for LED blinking
DigitalOut LED_1(LED_RED, 1);     // onboard red LED
DigitalOut LED_2(LED_GREEN, 1);   // onboard green LED
DigitalOut LED_3(LED_BLUE, 1);    // onboard blue LED

// ******** start_blinking() ************************************
// about:  Creates a thread every second which blinks the red LED
// input:  freq - freq in which to blink led
//         color - color led you wish to turn on, in string format
// output: none
// ******************************************************************
void start_blinking(float freq, const char* color){
  if(strcmp("red", color) == 0)   TickerLED.attach(blink_red, freq);
  if(strcmp("green", color) == 0) TickerLED.attach(blink_green, freq);
  if(strcmp("blue", color) == 0)  TickerLED.attach(blink_blue, freq);
}


// ******** cancel_blinking() ***********************************
// about:  kill blinking led thread, and turn off all led colors
// input:  none
// output: none
// ******************************************************************
void cancel_blinking(){
  TickerLED.detach();
  LED_1 = 1;
  LED_2 = 1;
  LED_3 = 1;
}


// ******** blink_red() ********************************************
// about:  flip the status of the red LED 
// input:  none
// output: none
// *****************************************************************
void blink_red(){
  LED_1 = !LED_1;
}


// ******** blink_green() ********************************************
// about:  flip the status of the green LED 
// input:  none
// output: none
// *****************************************************************
void blink_green(){
  LED_2 = !LED_2;
}


// ******** blink_blue() ********************************************
// about:  flip the status of the blue LED 
// input:  none
// output: none
// *****************************************************************
void blink_blue(){
  LED_3 = !LED_3;
}
