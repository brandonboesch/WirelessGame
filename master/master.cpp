// Filename: master.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "master.h"
#include "ip6string.h"
#include "nanostack/socket_api.h"
#include "mbed-trace/mbed_trace.h"
#include "led.h"

#define TRACE_GROUP "masterComms"
#define MULTICAST_ADDR_STR "ff03::1"
#define UDP_PORT 1234
#define IP_LAST4_OFFSET 35


// ******************** GLOBALS *************************************
NetworkInterface *NetworkIf;                // interface used to create a UDP socket
UDPSocket* MySocket;                        // pointer to UDP socket
InterruptIn MyButton(MBED_CONF_APP_BUTTON); // user input button
EventQueue Queue1;                          // queue for sending messages
SocketAddress Slave1_Addr = NULL;
SocketAddress Slave2_Addr = NULL;
SocketAddress Slave3_Addr = NULL;
SocketAddress Slave4_Addr = NULL;

uint8_t MultiCastAddr[16] = {0};
static const int16_t MulticastHops = 10;    // # of hops multicast messages can   
uint8_t ReceiveBuffer[BUFF_SIZE];           // buffer that holds transmissions
bool Init_Mode = true;                      // determines wheter in init mode or game mode
// ******************************************************************


// ****************** FUNCTIONS *************************************

// ******** master_init() *******************************************
// about:  Initializes a new socket using UDP, configures button interrupts, 
//         and dispatches the queue.
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
void masterInit(NetworkInterface *interface){
  printf("Initializing master device\n");           
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

  // dispatch forever
  Queue1.dispatch();                           
}


// ******** sendMessage()****************************************
// about:  Broadcast a message to all connected devices
// input:  messageBuff - string of message you wish to broadcast
// output: none
// ***************************************************************
void sendMessage(const char messageBuff[BUFF_SIZE]) {
  tr_debug("sending message: %s", messageBuff);
  SocketAddress send_sockAddr(MultiCastAddr, NSAPI_IPv6, UDP_PORT);
  MySocket->sendto(send_sockAddr, messageBuff, BUFF_SIZE);
}


// ******** receiveMessage()*********************************************
// about:  Reads all data from the socket, and updates devices accordingly
// input:  none
// output: none
// ***************************************************************
void receiveMessage() {
  // Read data from the socket
  SocketAddress source_addr;
  memset(ReceiveBuffer, 0, sizeof(ReceiveBuffer));
  bool something_in_socket = true;
  // loop while data exists in buffer
  while(something_in_socket){
    int length = MySocket->recvfrom(&source_addr, ReceiveBuffer, sizeof(ReceiveBuffer)-1);
    if (length > 0) {
      printf("Receiving packet from %s: %s\n",source_addr.get_ip_address()+IP_LAST4_OFFSET,ReceiveBuffer);
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


// ******** pairSlaves()******************************************
// about:  Reads all data from the socket and pairs with slaves
// input:  none
// output: none
// ***************************************************************
void pairSlaves() {
  // Read data from the socket
  SocketAddress source_addr;
  memset(ReceiveBuffer, 0, sizeof(ReceiveBuffer));
  bool something_in_socket = true;
  // loop while data exists in buffer
  while(something_in_socket){
    int length = MySocket->recvfrom(&source_addr, ReceiveBuffer, sizeof(ReceiveBuffer)-1);
    if (length > 0) {
      if(Slave1_Addr.get_ip_address() == NULL  && source_addr != Slave2_Addr && source_addr != Slave3_Addr && source_addr != Slave4_Addr){
        printf("Slave1 assigned\n");
        Slave1_Addr = source_addr;
      }
      else if(Slave2_Addr.get_ip_address() == NULL  && source_addr != Slave1_Addr && source_addr != Slave3_Addr && source_addr != Slave4_Addr){
        printf("Slave2 assigned\n");
        Slave2_Addr = source_addr;
      }

      else if(Slave3_Addr.get_ip_address() == NULL  && source_addr != Slave1_Addr && source_addr != Slave2_Addr && source_addr != Slave4_Addr){
        printf("Slave3 assigned\n");
        Slave3_Addr = source_addr;
      }
      else if(Slave4_Addr.get_ip_address() == NULL  && source_addr != Slave1_Addr && source_addr != Slave2_Addr && source_addr != Slave3_Addr){
        printf("Slave4 assigned\n");
        Slave4_Addr = source_addr;
      }

      printf("\nSlave1_Addr: %s\nSlave2_Addr: %s\nSlave3_Addr: %s\nSlave4_Addr: %s\n", Slave1_Addr.get_ip_address(), Slave2_Addr.get_ip_address(), Slave3_Addr.get_ip_address(), Slave4_Addr.get_ip_address());
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


// ***************************************************************


// ****************** ISR HANDLERS *******************************

// ******** myButton_isr()****************************************
// about:  Handler for button presses. 
//         Note, isr must be quick (no printing or network functions 
//         directly called from isr).
// input:  none
// output: none
// ***************************************************************
void myButton_isr() {
  Init_Mode = false;
  Queue1.call(sendMessage, "Init complete\n");
  }


// ******** socket_isr()******************************************
// about:  Handler for socket for when packets come in or out.
//         Note, isr must be quick (no printing or network functions 
//         directly called from isr).
// input:  none
// output: none
// ***************************************************************
void socket_isr(){
  if(Init_Mode){
    Queue1.call(pairSlaves);      
  }
  else{
    Queue1.call(receiveMessage);  
  }
}

