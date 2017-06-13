// Filename: slaveComms.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "slaveComms.h"
#include "nanostack/socket_api.h"
#include "ip6string.h"

#define MULTICAST_ADDR_STR "ff03::1"
#define UDP_PORT 1234

// ******************** GLOBALS *************************************
NetworkInterface * NetworkIf;                // interface used to create a UDP socket
UDPSocket* my_socket;                        // pointer to UDP socket
Ticker ticker;                               // for LED blinking
DigitalOut led_1(MBED_CONF_APP_LED, 1);      // status LED
InterruptIn my_button(MBED_CONF_APP_BUTTON); // user input button

bool DEBUG=true;                             // turn on debug mode
uint8_t MultiCastAddr[16] = {0};
static const int16_t MulticastHops = 10;     // # of hops the multicast message can go
// ******************************************************************



// ****************** FUNCTIONS *************************************

// ******** start_slave()() *****************************************
// about:  Cnitializes a new socket using UDP
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
void start_slave(NetworkInterface *interface){
  if(DEBUG)  printf("testing start_slave()\n");            // FIXME: for debug only.
  NetworkIf = interface;
  stoip6(MULTICAST_ADDR_STR, strlen(MULTICAST_ADDR_STR), MultiCastAddr);
  init_socket();
}


// ******** start_blinking()() **************************************
// about:  Creates a thread every second which blinks the status LED
// input:  none
// output: none
// ******************************************************************
void start_blinking(){
  ticker.attach(blink, 1.0);
}


// ******** cancel_blinking()() *************************************
// about:  kill blinking led thread
// input:  none
// output: none
// ******************************************************************
void cancel_blinking(){
  ticker.detach();
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


// ******** init_socket() *******************************************
// about:  initializes a new socket using UDP
// input:  none
// output: none
// ******************************************************************
static void init_socket(){
  my_socket = new UDPSocket(NetworkIf);
  my_socket->set_blocking(false);
  my_socket->bind(UDP_PORT);    
  my_socket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &MulticastHops, sizeof(MulticastHops));
  if (MBED_CONF_APP_BUTTON != NC) {
  	my_button.fall(&my_button_isr);
  }
  //let's register the call-back function.
  //If something happens in socket (packets in or out), the call-back is called.
  my_socket->sigio(callback(handle_socket));
  // dispatch forever
  queue.dispatch();
}


// ******** my_button_isr()****************************************
// about:  We cannot use printing or network functions directly from isrs.
// input:  none
// output: none
// ******************************************************************
static void my_button_isr() {
  button_status = !button_status;
  queue.call(send_message);
}
