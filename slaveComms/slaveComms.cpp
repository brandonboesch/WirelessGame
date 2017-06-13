// Filename: slaveComms.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "slaveComms.h"


///////////////////////// Globals ////////////////////////////////

//NetworkInterface * network_if;
Ticker ticker;                      // for LED blinking
DigitalOut led_1(MBED_CONF_APP_LED, 1);

bool DEBUG=true;
///////////////////////// Functions /////////////////////////////

void start_slave(NetworkInterface *interface){
  if(DEBUG)  printf("testing start_slave\n");            // FIXME: for debug only.
  while(true){}; 
//  tr_debug("start_slave()");i
//  network_if = interface;
}


void start_blinking(){
  ticker.attach(blink, 1.0);
}


void cancel_blinking(){
  ticker.detach();
  led_1=1;
}


static void blink(){
  led_1 = !led_1;
}


