// Filename: slaveComms.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "slaveComms.h"
#include "ip6string.h"

#define MULTICAST_ADDR_STR "ff03::1"

// ******************** GLOBALS *************************************
NetworkInterface *NetworkIf;                // interface used to create a UDP socket
Ticker Ticker1;                              // for LED blinking
DigitalOut led_1(MBED_CONF_APP_LED, 1);

bool DEBUG=true;                            // turn on debug mode
uint8_t MultiCastAddr[16] = {0};

// ******************************************************************


// ****************** FUNCTIONS *************************************

// ******** start_slave()() *****************************************
// about:  Initializes a new socket using UDP
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
void start_slave(NetworkInterface *interface){
  if(DEBUG)  printf("testing start_slave()\n");            // FIXME: for debug only.
  NetworkIf = interface;
  stoip6(MULTICAST_ADDR_STR, strlen(MULTICAST_ADDR_STR), MultiCastAddr);
}


// ******** start_blinking()() **************************************
// about:  Creates a thread every second which blinks the status LED
// input:  none
// output: none
// ******************************************************************
void start_blinking(){
  Ticker1.attach(blink, 1.0);
}


// ******** cancel_blinking()() *************************************
// about:  kill blinking led thread
// input:  none
// output: none
// ******************************************************************
void cancel_blinking(){
  Ticker1.detach();
  led_1=1;
}


// ******** blink() *************************************************
// about:  flip the status of the LED 
// input:  none
// output: none
// ******************************************************************
static void blink(){
  led_1 = !led_1;
}


