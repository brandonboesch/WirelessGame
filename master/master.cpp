// Filename: master.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017


// ****** ST7735 Interface ******************************************
//
// Pinout:
//   LITE    - connected to +3.3 V
//   MISO    - connected to PTD7
//   SCK     - connected to PTD5
//   MOSI    - connected to PTD6
//   TFT_CS  - connected to PTD4
//   CARD_CS - unconnected 
//   D/C     - connected to PTC18
//   RESET   - connected to +3.3 V
//   VCC     - connected to +3.3 V
//   Gnd     - connected to ground

// Coordinates when rotation=3:
//   Top left     = (0, 0)
//   Bottom right = (159, 127)

// ******************************************************************

#include "mbed.h"
#include "master.h"
#include "ip6string.h"
#include "NanostackInterface.h"
#include "NanostackRfPhyAtmel.h"
#include "nanostack/socket_api.h"
#include "mbed-trace/mbed_trace.h"
#include "led.h"
#include "Adafruit_ST7735.h"
#include "bmp.h"

#define TRACE_GROUP "masterComms"
#define MULTICAST_ADDR_STR "ff03::1"
#define UDP_PORT 1234
#define IP_LAST4_OFFSET 35
#define MAX_NUM_SLAVES 4
#define SCREEN_LEN_SHORT 128         // number of pixels on short dimension of screen
#define SCREEN_LEN_LONG 160          // number of pixels on long dimension of screen
#define ANGLE_DIV 0.0291           // PI / (SCREEN_LENGTH_SHORT-PADDLE_SIZE) = 0.0291 when paddle = 20
//#define ANGLE_DIV 0.0293             // PI / (SCREEN_LENGTH_SHORT-PADDLE_SIZE) = 0.0293 when paddle = 21
#define PADDLE_SIZE 20               // length of player's paddle. Update ANGLE_DIV if changes.
#define SCREEN_MIN_PADDLE (SCREEN_LEN_SHORT-PADDLE_SIZE)


// ******************** GLOBALS *************************************
NanostackRfPhyAtmel rf_phy(ATMEL_SPI_MOSI, ATMEL_SPI_MISO, ATMEL_SPI_SCLK, ATMEL_SPI_CS,
                           ATMEL_SPI_RST, ATMEL_SPI_SLP, ATMEL_SPI_IRQ, ATMEL_I2C_SDA, 
                           ATMEL_I2C_SCL);  // phy for Atmel shield
ThreadInterface mesh;                       // mesh interface using Thread
NetworkInterface *NetworkIf;                // interface used to create a UDP socket
UDPSocket* MySocket;                        // pointer to UDP socket
InterruptIn MyButton(MBED_CONF_APP_BUTTON); // user input button
EventQueue Queue1;                          // queue for sending messages

SocketAddress Slave1_Addr = NULL;           // address for slave 1
SocketAddress Slave2_Addr = NULL;           // address for slave 2
SocketAddress Slave3_Addr = NULL;           // address for slave 3
SocketAddress Slave4_Addr = NULL;           // address for slave 4

Adafruit_ST7735 TFT(PTD6, PTD7, PTD5, PTD4, PTC18, PTC15); // MOSI, MISO, SCK, TFT_CS, D/C, RESET

Mutex stdio_mutex;

uint8_t MultiCastAddr[16] = {0};            // address used for multicast messages
static const int16_t MulticastHops = 10;    // # of hops multicast messages can   
uint8_t ReceiveBuffer[COMM_BUFF_SIZE];      // buffer that holds transmissions
bool Init_Mode = true;                      // determines wheter in init mode or game mode

float Slave1_Angle = 0;                     // latest angle stored in system for Slave1
int8_t Slave1_OldPixel = 0;                  
int8_t Slave1_Score = 0;                    // Slave1's score

float Slave2_Angle = 0;                     // latest angle stored in system for Slave2
int8_t Slave2_OldPixel = 0;
int8_t Slave2_Score = 0;                    // Slave2's score

int8_t Ball_Position_X = 0;
int8_t Ball_Position_Y = 0;

int8_t Old_Ball_Position_X = 0;
int8_t Old_Ball_Position_Y = 0;
// ******************************************************************


// ****************** FUNCTIONS *************************************


// ******** game()************************************************
// about:  A thread which handles all things related to the game
// input:  none
// output: none
// ***************************************************************
void game(void){
  //erase old objects on screen
  TFT.drawFastVLine(10, Slave1_OldPixel, PADDLE_SIZE, ST7735_GREEN);    // erase Slave1's paddle
  TFT.drawFastVLine(150, Slave2_OldPixel, PADDLE_SIZE, ST7735_GREEN);   // erase Slave2's paddle
  TFT.drawBall(11, Old_Ball_Position_Y+(PADDLE_SIZE/2), ST7735_GREEN);  // erase Ball

  // calculate and draw new object locations
  float pixel1 = SCREEN_MIN_PADDLE-(abs(Slave1_Angle)/ANGLE_DIV);       // translate angle to pixel location
  float pixel2 = SCREEN_MIN_PADDLE-(abs(Slave2_Angle)/ANGLE_DIV);       // translate angle to pixel location
  TFT.drawFastVLine(10, pixel1, PADDLE_SIZE, ST7735_BLACK);             // draw Slave1's paddle
  TFT.drawFastVLine(150, pixel2, PADDLE_SIZE, ST7735_BLACK);            // draw Slave2's paddle
  TFT.drawBall(11, pixel1+(PADDLE_SIZE/2), ST7735_RED);                 // draw the pong ball

  // update objects' old values
  stdio_mutex.lock();
  Slave1_OldPixel = pixel1;                                             // update Slave1's old pixel
  Slave2_OldPixel = pixel2;                                             // update old pixel
  Old_Ball_Position_Y = pixel1;                                         // update ball's old position
  printf("Slave1_OldPixel: %d, Slave1_Angle: %.2f\n", Slave1_OldPixel, Slave1_Angle);
  stdio_mutex.unlock();
}


// ******** main() *******************************************
// about:  Initializes a new socket using UDP, configures button interrupts, 
//         and dispatches the queue.
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
int main(void){
  printf("Initializing master device\n");

  // setup trace printing for debug
  mbed_trace_init();
  mbed_trace_print_function_set(trace_printer);
  start_blinking(0.5, "red");

  // setup the display
  TFT.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  TFT.setRotation(3);
  TFT.fillScreen(ST7735_BLACK);
  TFT.drawString(45, 10, (unsigned char*)("PONG"), ST7735_YELLOW, ST7735_BLACK, 3);

  // connect to mesh and get IP address
  printf("\n\nConnecting...\n");
  TFT.drawString(0, 40, (unsigned char*)("Connecting..."), ST7735_WHITE, ST7735_BLACK, 1);
  mesh.initialize(&rf_phy);
  int error=-1;
  if((error=mesh.connect())){
    printf("Connection failed! %d\n", error);
    TFT.drawString(80, 40, (unsigned char*)("Failed!"), ST7735_RED, ST7735_BLACK, 1);
    return error;
  }
  while(NULL == mesh.get_ip_address()){
    Thread::wait(500);
  }
  printf("connected. IP = %s\n", mesh.get_ip_address());
  TFT.drawString(80, 40, (unsigned char*)("Success!"), ST7735_GREEN, ST7735_BLACK, 1);
  cancel_blinking();
  start_blinking(0.5, "green");
 
  // initialize the network socket
  stoip6(MULTICAST_ADDR_STR, strlen(MULTICAST_ADDR_STR), MultiCastAddr);
  MySocket = new UDPSocket(&mesh);
  MySocket->set_blocking(false);
  MySocket->bind(UDP_PORT);  
  MySocket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &MulticastHops, sizeof(MulticastHops));
 
  // configure button interrupt
  MyButton.fall(&myButton_isr);

  // if something happens in socket (packets in or out), the call-back is called.
  printf("Pair controllers now, and then press start.\n");
  TFT.drawString(0, 50, (unsigned char*)("Pair controllers now, and"), ST7735_WHITE, ST7735_BLACK, 1);
  TFT.drawString(0, 60, (unsigned char*)("then press start."), ST7735_WHITE, ST7735_BLACK, 1);
  MySocket->sigio(callback(socket_isr));

  // dispatch forever
  Queue1.dispatch();                           
}


// ******** sendMessage()****************************************
// about:  Broadcast a message to all connected devices
// input:  messageBuff - string of message you wish to broadcast
// output: none
// ***************************************************************
void sendMessage(const char messageBuff[COMM_BUFF_SIZE]) {
  tr_debug("sending message: %s", messageBuff);
  SocketAddress send_sockAddr(MultiCastAddr, NSAPI_IPv6, UDP_PORT);
  MySocket->sendto(send_sockAddr, messageBuff,COMM_BUFF_SIZE);
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

      // need to update the slaves' current angles based off their transmission data
      // Slave1
      if(source_addr.get_ip_address()==Slave1_Addr){
        char* angleString = (char*)ReceiveBuffer;
        char* segment;
        // splitting angleString into segments using tokens
        segment = strtok (angleString, " = ");
        int segmentIndex = 0;                       // segment # in string
        while (segment != NULL){
          if(segmentIndex == 1){                    // grab value for angle here
            stdio_mutex.lock();
            Slave1_Angle = strtof (segment, NULL);  // update Slave's angle. str to float
            stdio_mutex.unlock();
          }
          segment = strtok(NULL, " = ");            // advances segment for next iteration
          segmentIndex++;                           // next segment
        }
      }

      // Slave2
      else if(source_addr.get_ip_address()==Slave2_Addr){
        char* angleString = (char*)ReceiveBuffer;
        char* segment;
        // splitting angleString into segments using tokens
        segment = strtok (angleString, " = ");
        int segmentIndex = 0;                       // segment # in string
        while (segment != NULL){
          if(segmentIndex == 1){                    // grab value for angle here
            stdio_mutex.lock();
            Slave2_Angle = strtof (segment, NULL);  // update Slave's angle. str to float
            stdio_mutex.unlock();
          }
          segment = strtok(NULL, " = ");            // advances segment for next iteration
          segmentIndex++;                           // next segment
        }
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


// ******** pairSlaves()******************************************
// about:  Reads all data from the socket. Attempts to connect and 
//         enumerate slaves when they send pair requests. Should only
//         run while system is in Init_Mode. 
// input:  none
// output: none
// ***************************************************************
void pairSlaves() {
  // Read data from the socket
  SocketAddress source_addr;
  memset(ReceiveBuffer, 0, sizeof(ReceiveBuffer));
  bool something_in_socket = true;
  // continue looping while data exists in buffer
  while(something_in_socket){
    int length = MySocket->recvfrom(&source_addr, ReceiveBuffer, sizeof(ReceiveBuffer)-1);
    if (length > 0) {
      
      // checks if slave is already assigned in system.  If not, and If a slot is available, then assign the new pair request there.
      // Slave1
      if(Slave1_Addr.get_ip_address() == NULL  && source_addr != Slave2_Addr && source_addr != Slave3_Addr && source_addr != Slave4_Addr){
        printf("Slave1 assigned\n");
        TFT.drawString(0, 80, (unsigned char*)("--Player 1 paired"), ST7735_WHITE, ST7735_BLACK, 1);
        Slave1_Addr = source_addr;

      // Slave2
      }
      else if(Slave2_Addr.get_ip_address() == NULL  && source_addr != Slave1_Addr && source_addr != Slave3_Addr && source_addr != Slave4_Addr){
        printf("Slave2 assigned\n");
        TFT.drawString(0, 90, (unsigned char*)("--Player 2 paired"), ST7735_WHITE, ST7735_BLACK, 1);

        Slave2_Addr = source_addr;
      }

      // Slave3
      else if(Slave3_Addr.get_ip_address() == NULL  && source_addr != Slave1_Addr && source_addr != Slave2_Addr && source_addr != Slave4_Addr){
        printf("Slave3 assigned\n");
        TFT.drawString(0, 100, (unsigned char*)("--Player 3 paired"), ST7735_WHITE, ST7735_BLACK, 1);
        Slave3_Addr = source_addr;
      }
      // Slave4
      else if(Slave4_Addr.get_ip_address() == NULL  && source_addr != Slave1_Addr && source_addr != Slave2_Addr && source_addr != Slave3_Addr){
       TFT.drawString(0, 110, (unsigned char*)("--Player 4 paired"), ST7735_WHITE, ST7735_BLACK, 1); 
        printf("Slave4 assigned\n");
        Slave4_Addr = source_addr;
      }

      // print out current pairing results
      printf("\nSlave1_Addr: %s\nSlave2_Addr: %s\nSlave3_Addr: %s\nSlave4_Addr: %s\n", Slave1_Addr.get_ip_address(), Slave2_Addr.get_ip_address(), Slave3_Addr.get_ip_address(), Slave4_Addr.get_ip_address());
    }
    
    // error checkong
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
  if(Init_Mode){
    Init_Mode = false;                             // finishing initilization mode
    cancel_blinking();                             // turn off last stages heartbeat
    start_blinking(0.5, "blue");                   // change LED color to signify next state
    Queue1.call(sendMessage, "Init complete\n");   // output to console
    TFT.fillScreen(ST7735_GREEN);
    TFT.drawFastVLine(80, 0, 128, ST7735_BLACK);
    TFT.drawCircle(80, 64, 10, ST7735_BLACK);
    char buff[8];
    itoa(Slave1_Score,buff,10);
    TFT.drawString(0, 0, (unsigned char*)(buff), ST7735_WHITE, ST7735_BLACK, 1);   
    itoa(Slave2_Score,buff,10);
    TFT.drawString(155, 0, (unsigned char*)(buff), ST7735_WHITE, ST7735_BLACK, 1);    
    Queue1.call_every(10,game);                     // start up the game after button press
  }
  else{
    Queue1.call(sendMessage, "button press\n");
  }
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

