// Filename: masterComms.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "masterComms.h"
#include "ip6string.h"
#include "nanostack/socket_api.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "masterComms"
#define MULTICAST_ADDR_STR "ff03::1"
#define UDP_PORT 1234
#define MESSAGE_WAIT_TIMEOUT (30.0)

// ******************** GLOBALS *************************************
NetworkInterface *NetworkIf;                // interface used to create a UDP socket
UDPSocket* MySocket;                        // pointer to UDP socket
InterruptIn MyButton(MBED_CONF_APP_BUTTON); // user input button
EventQueue Queue1;                          // queue for sending messages from button press
Timeout MessageTimeout;

uint8_t MultiCastAddr[16] = {0};
static const int16_t MulticastHops = 10;    // # of hops multicast messages can go                    
uint8_t ReceiveBuffer[BUFF_SIZE];           // buffer that holds transmissions
// ******************************************************************


// ****************** FUNCTIONS *************************************

// ******** start_master() *****************************************
// about:  Initializes a new socket using UDP and dispatches to queue
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
void start_master(NetworkInterface *interface){
  tr_debug("begin start_master()\n");           
  NetworkIf = interface;
  stoip6(MULTICAST_ADDR_STR, strlen(MULTICAST_ADDR_STR), MultiCastAddr);
  MySocket = new UDPSocket(NetworkIf);
  MySocket->set_blocking(false);
  MySocket->bind(UDP_PORT);  
  MySocket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &MulticastHops, sizeof(MulticastHops));
 
  // configure button interrupt
  if(MBED_CONF_APP_BUTTON != NC){
  	MyButton.fall(&myButton_isr);
  }

  //if something happens in socket (packets in or out), the call-back is called.
  MySocket->sigio(callback(socket_isr));
  Queue1.dispatch();                           // dispatch forever
}


// ******** myButton_isr()****************************************
// about:  Interrupt service routine for button presses. 
//         Note, isr must be quick (no printing or network functions 
//         directly called from isr).
// input:  none
// output: none
// ***************************************************************
static void myButton_isr() {
  Queue1.call(send_message, "button press");
}


// ******** socket_isr()***************************************
// about:  Handler for socket for when packets come in or out
// input:  none
// output: none
// ***************************************************************
static void socket_isr(){
  Queue1.call(receive);  // call-back might come from ISR
}


// ******** messageTimeoutCallback()***************************************
// about:  Broadcasts a message in case of timeout
// input:  none
// output: none
// ***************************************************************
static void messageTimeoutCallback(){
  send_message("callback");
}


// ******** send_message()****************************************
// about:  Broadcast a message to all connected devices
// input:  none
// output: none
// ***************************************************************
static void send_message(const char messageBuff[BUFF_SIZE]) {
  tr_debug("sending message: %s", messageBuff);
  SocketAddress send_sockAddr(MultiCastAddr, NSAPI_IPv6, UDP_PORT);
  MySocket->sendto(send_sockAddr, messageBuff, BUFF_SIZE);
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

      printf("Receiving message: %s\n", ReceiveBuffer);
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
