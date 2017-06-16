// Filename: slave.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "slave.h"
#include "ip6string.h"
#include "nanostack/socket_api.h"
#include "mbed-trace/mbed_trace.h"
#include "FXOS8700CQ.h"
#include <math.h>

#define TRACE_GROUP "Slave"
#define MULTICAST_ADDR_STR "ff03::1"
#define UDP_PORT 1234
#define DATA_ARRAY_SIZE 20
#define DATA_COLLECT_RATE 0.01
#define IP_LAST4_OFFSET 35

// ******************** GLOBALS *************************************
NetworkInterface *NetworkIf;                // interface used to create a UDP socket
UDPSocket* MySocket;                        // pointer to UDP socket
InterruptIn MyButton(MBED_CONF_APP_BUTTON); // user input button
EventQueue Queue1;                          // queue for sending messages from button press
Timeout MessageTimeout;
Ticker TickerAccel;                         // timer for measuring accelerometer data
FXOS8700CQ device(I2C_SDA,I2C_SCL);         // create object and specifiy pins

uint8_t MultiCastAddr[16] = {0};
static const int16_t MulticastHops = 10;    // # of hops multicast messages can go       
uint8_t ReceiveBuffer[COMM_BUFF_SIZE];      // buffer that holds transmissions
Data valueArray[DATA_ARRAY_SIZE];           // array to hold sampled accelerometer data
uint8_t valueArrayIndex = 0;                // index for valueArray

// ******************************************************************


// ****************** FUNCTIONS *************************************

// ******** slaveInit() *******************************************
// about:  Initializes a new socket using UDP, configures button interrupts, 
//         and dispatches the queue.
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
void slaveInit(NetworkInterface *interface){
  printf("Initializing slave device\n");           
  NetworkIf = interface;
  stoip6(MULTICAST_ADDR_STR, strlen(MULTICAST_ADDR_STR), MultiCastAddr);
  MySocket = new UDPSocket(NetworkIf);
  MySocket->set_blocking(false);
  MySocket->bind(UDP_PORT);  
  MySocket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &MulticastHops, sizeof(MulticastHops));

  // initialize accelerometer
  device.init();                       

  // attach accelerometer measuring function to ticker
  TickerAccel.attach(accelMeasure_isr, DATA_COLLECT_RATE);
 
  // configure button interrupt
  if(MBED_CONF_APP_BUTTON != NC){
  	MyButton.fall(&myButton_isr);
  }

  //if something happens in socket (packets in or out), the call-back is called.
  MySocket->sigio(callback(socket_isr));

  // dispatch forever
  Queue1.dispatch();                           
}


// ******** calcAngle()******************************************
// about:  Averages values in the date array, translates cartesian 
//         coordinates to polar, and then sends as message.
// input:  none
// output: none
// ***************************************************************
void calcAngle(){
  Data valueAvg = {0};                               // holds average of values in array
  for(uint8_t i = 0; i < DATA_ARRAY_SIZE; i++){      // calculate average
    //valueAvg.ax += valueArray[i].ax;               // x value added. Not used
    valueAvg.ay += valueArray[i].ay;                 // y value added
    valueAvg.az += valueArray[i].az;                 // z value added
  }
  //valueAvg.ax = valueAvg.ax/DATA_ARRAY_SIZE;       // x value averaged.  Not used
  valueAvg.ay = valueAvg.ay/DATA_ARRAY_SIZE;         // y value averaged
  valueAvg.az = valueAvg.az/DATA_ARRAY_SIZE;         // z value averaged

  float angle = atan2(valueAvg.az,valueAvg.ay);      // translate cartesian to polar
  
  char angleBuff[COMM_BUFF_SIZE];                    // string to hold average
  snprintf(angleBuff, sizeof angleBuff, "angle = %.2f", angle);  // load the string
  sendMessage(angleBuff);                                  // broadcast averaged data
  TickerAccel.attach(accelMeasure_isr, DATA_COLLECT_RATE); // turn measuring isr back on
}


// ******** sendMessage()****************************************
// about:  Broadcast a message to all connected devices
// input:  messageBuff - string of message you wish to broadcast
// output: none
// **************************************************************
void sendMessage(const char messageBuff[COMM_BUFF_SIZE]) {
  tr_debug("sending message: %s", messageBuff);
  SocketAddress send_sockAddr(MultiCastAddr, NSAPI_IPv6, UDP_PORT);
  MySocket->sendto(send_sockAddr, messageBuff, COMM_BUFF_SIZE);
}


// ******** receiveMessage()**************************************
// about:  Reads all data from the socket
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
  Queue1.call(sendMessage, "button pushed");
}


// ******** socket_isr()******************************************
// about:  Handler for socket for when packets come in or out.
//         Note, isr must be quick (no printing or network functions 
//         directly called from isr).
// input:  none
// output: none
// ***************************************************************
void socket_isr(){
  Queue1.call(receiveMessage);  // call-back might come from ISR
}


// ******** accelMeasure_isr() ***********************************
// about:  measure the accelerometer, and store the results. If array 
//         is fills up, then initiate calcAngle function call.
// input:  none
// output: none
// ***************************************************************
void accelMeasure_isr(){
  valueArray[valueArrayIndex] = device.get_values();
  valueArrayIndex++;
  if(valueArrayIndex == DATA_ARRAY_SIZE){
    valueArrayIndex = 0;   
    TickerAccel.detach();
    Queue1.call(calcAngle);
  }
}
