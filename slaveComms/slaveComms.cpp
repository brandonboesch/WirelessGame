// Filename: slaveComms.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "slaveComms.h"
#include "ip6string.h"
#include "nanostack/socket_api.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "slaveComms"
#define MULTICAST_ADDR_STR "ff03::1"
#define UDP_PORT 1234
#define MESSAGE_WAIT_TIMEOUT (30.0)

// ******************** GLOBALS *************************************
NetworkInterface *NetworkIf;                // interface used to create a UDP socket
Ticker Ticker1;                             // for LED blinking
DigitalOut LED_1(MBED_CONF_APP_LED, 1);
UDPSocket* MySocket;                        // pointer to UDP socket
InterruptIn MyButton(MBED_CONF_APP_BUTTON); // user input button
EventQueue Queue1;                          // queue for sending messages from button press.
Timeout MessageTimeout;

uint8_t MultiCastAddr[16] = {0};
static const int16_t MulticastHops = 10;    // # of hops the multicast message can go
bool ButtonStatus = 0;
static const char BufferOn[2] = {'o','n'};
static const char BufferOff[3] = {'o','f','f'};
uint8_t ReceiveBuffer[5];

// ******************************************************************


// ****************** FUNCTIONS *************************************

// ******** start_slave()() *****************************************
// about:  Initializes a new socket using UDP
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
void start_slave(NetworkInterface *interface){
  tr_debug("testing start_slave()\n");           
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
  Ticker1.attach(blink, 1.0);
}


// ******** cancel_blinking()() *************************************
// about:  kill blinking led thread
// input:  none
// output: none
// ******************************************************************
void cancel_blinking(){
  Ticker1.detach();
  LED_1=1;
}


// ******** blink() ************************************************
// about:  flip the status of the LED 
// input:  none
// output: none
// *****************************************************************
static void blink(){
  LED_1 = !LED_1;
}


// ******** init_socket() ******************************************
// about:  initializes a new socket using UDP
// input:  none
// output: none
// *****************************************************************
static void init_socket(){
  MySocket = new UDPSocket(NetworkIf);
  MySocket->set_blocking(false);
  MySocket->bind(UDP_PORT);  
  MySocket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &MulticastHops, sizeof(MulticastHops));
  if(MBED_CONF_APP_BUTTON != NC){
  	MyButton.fall(&myButton_isr);
  }
  //if something happens in socket (packets in or out), the call-back is called.
  MySocket->sigio(callback(handle_socket));
  Queue1.dispatch();                           // dispatch forever
}


// ******** MyButton_isr()****************************************
// about:  We cannot use printing or network functions directly from isr.
// input:  none
// output: none
// ***************************************************************
static void myButton_isr() {
  ButtonStatus = !ButtonStatus;
  Queue1.call(send_message);
}


// ******** handle_socket()***************************************
// about:  Handler for socket for when packets come in or out
// input:  none
// output: none
// ***************************************************************
static void handle_socket(){
  Queue1.call(receive);  // call-back might come from ISR
}


// ******** messageTimeoutCallback()***************************************
// about:  Sends a message in case of timeout
// input:  none
// output: none
// ***************************************************************
static void messageTimeoutCallback(){
  send_message();
}


// ******** send_message()****************************************
// about:  Broadcast a message to all connected devices
// input:  none
// output: none
// ***************************************************************
static void send_message() {
  tr_debug("send msg %d", ButtonStatus);
  SocketAddress send_sockAddr(MultiCastAddr, NSAPI_IPv6, UDP_PORT);
  if(ButtonStatus) {
    LED_1 = 0;
    MySocket->sendto(send_sockAddr, BufferOn, 2);
  }
  else {
    LED_1 = 1;
    MySocket->sendto(send_sockAddr, BufferOff, 3);
  }
}


// ******** receive()*********************************************
// about:  Reads all data from the socket, and updates devices accordingly
// input:  none
// output: none
// ***************************************************************
static void receive() {
  // Read data from the socket
  SocketAddress source_addr;
  memset(ReceiveBuffer, 0, sizeof(ReceiveBuffer));
  bool something_in_socket = true;
  while(something_in_socket){
    int length = MySocket->recvfrom(&source_addr, ReceiveBuffer, sizeof(ReceiveBuffer) - 1);
    if (length > 0) {
      int timeout_value = MESSAGE_WAIT_TIMEOUT;
      tr_debug("Packet from %s\n", source_addr.get_ip_address());
      timeout_value += rand() % 30;
      tr_debug("Advertisiment after %d seconds", timeout_value);
      MessageTimeout.detach();
      MessageTimeout.attach(&messageTimeoutCallback, timeout_value);
      // Handle command - "on", "off"
      if(strcmp((char*)ReceiveBuffer, "on") == 0){
        tr_debug("Turning led on\n");
        LED_1 = 0;
        ButtonStatus=1;
      }
      if(strcmp((char*)ReceiveBuffer, "off") == 0){
        tr_debug("Turning led off\n");
        LED_1 = 1;
        ButtonStatus=0;
      }
    }
    else if(length!=NSAPI_ERROR_WOULD_BLOCK){
      tr_error("Error happened when receiving %d\n", length);
      something_in_socket=false;
    }
    else{
      // there was nothing to read.
      something_in_socket=false;
    }  
  }
}
