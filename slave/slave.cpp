// Filename: slave.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "slave.h"
#include "ip6string.h"
#include "NanostackInterface.h"
#include "NanostackRfPhyAtmel.h"
#include "nanostack/socket_api.h"
#include "mbed-trace/mbed_trace.h"
#include "FXOS8700CQ.h"
#include "led.h"
#include <math.h>
#include <string.h>

#define TRACE_GROUP "Slave"
#define MULTICAST_ADDR_STR "ff03::1"
#define UDP_PORT 1234
#define DATA_ARRAY_SIZE 20
#define DATA_COLLECT_RATE 0.01
#define IP_LAST4_OFFSET 18

// ******************** GLOBALS *************************************
NanostackRfPhyAtmel rf_phy(ATMEL_SPI_MOSI, ATMEL_SPI_MISO, ATMEL_SPI_SCLK, ATMEL_SPI_CS,
                           ATMEL_SPI_RST, ATMEL_SPI_SLP, ATMEL_SPI_IRQ, ATMEL_I2C_SDA, 
                           ATMEL_I2C_SCL);  // phy for Atmel shield
ThreadInterface mesh;                       // mesh interface using Thread
UDPSocket* MySocket;                        // pointer to UDP socket
InterruptIn MyButton(MBED_CONF_APP_BUTTON); // user input button
EventQueue Queue1;                          // queue for sending messages from button press
FXOS8700CQ device(I2C_SDA,I2C_SCL);         // accelerometer device
Data valueArray[DATA_ARRAY_SIZE];           // array to hold sampled accelerometer data
Ticker TickerAccel;                         // timer for measuring accelerometer data

uint8_t MultiCastAddr[16] = {0};            // address for multi device broadcasting
static const int16_t MulticastHops = 10;    // # of hops multicast messages can go       
uint8_t ReceiveBuffer[COMM_BUFF_SIZE];      // buffer that holds transmissions
uint8_t valueArrayIndex = 0;                // index for valueArray
bool Init_Mode = true;                      // determines wheter in init mode or game mode

// ******************************************************************


// ****************** FUNCTIONS *************************************

// ******** main() **************************************************
// about:  Initializes a new socket using UDP, configures button interrupts, 
//         and dispatches the queue.
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
int main(void){
  printf("Initializing slave device\n"); 

  // setup trace printing for debug
  mbed_trace_init();
  mbed_trace_print_function_set(trace_printer);
  start_blinking(0.5, "red");

  // connect to mesh and get IP address
  printf("\n\nConnecting...\n");
  mesh.initialize(&rf_phy);
  int error=-1;
  if((error=mesh.connect())){
    printf("Connection failed! %d\n", error);
    return error;
  }
  while(NULL == mesh.get_ip_address()){
    Thread::wait(500);
  }
  printf("connected. IP = %s\n", mesh.get_ip_address());
  cancel_blinking();
  start_blinking(0.5, "green");

  // initialize the network socket
  stoip6(MULTICAST_ADDR_STR, strlen(MULTICAST_ADDR_STR), MultiCastAddr);
  MySocket = new UDPSocket(&mesh);
  MySocket->set_blocking(false);
  MySocket->bind(UDP_PORT);  
  MySocket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &MulticastHops, sizeof(MulticastHops));

  // initialize accelerometer
  device.init();                       
 
  // configure button interrupt
 	MyButton.fall(&myButton_isr);


  //if something happens in socket (packets in or out), the call-back is called.
  MySocket->sigio(callback(socket_isr));

  // dispatch forever
  Queue1.dispatch();                           
}


// ******** calcAngle()******************************************
// about:  Averages values in the data array, translates cartesian 
//         coordinates to polar, and then sends angle as message.
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

  float angle = atan2(valueAvg.az,valueAvg.ay);      // translate cartesian to polar.
 
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
  printf("Sending packet from %s : %s\n", mesh.get_ip_address()+IP_LAST4_OFFSET, messageBuff);
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
      if(strcmp((const char*)ReceiveBuffer,"Init complete\n") == 0 && Init_Mode){   // strings are equal
        Init_Mode = false;
        cancel_blinking();
        start_blinking(0.5, "blue");
        printf("Begin accelerometer\n");
        // attach accelerometer measuring function to ticker
        TickerAccel.attach(accelMeasure_isr, DATA_COLLECT_RATE);
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


// ******** trace_printer() *****************************************
// about:  Function that calls printf
// input:  *str - pointer to string that will be printed
// output: none
// ******************************************************************
void trace_printer(const char* str){
  printf("%s\n", str);
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
  if(Init_Mode) Queue1.call(receiveMessage);  
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
