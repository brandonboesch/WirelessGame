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
#define MAX_SCORE 5
#define GAME_CALL_RATE 10        // the higher the value, the slower the system adds game() to the event queue
#define SCREEN_LEN_SHORT 128     // number of pixels on short dimension of screen
#define SCREEN_LEN_LONG 160      // number of pixels on long dimension of screen
#define ANGLE_DIV 0.0291         // PI / (SCREEN_LENGTH_SHORT-PADDLE_SIZE) = 0.0291 when paddle = 20
#define PADDLE_SIZE 20           // length of player's paddle. Update ANGLE_DIV if changes.
#define SCREEN_MINUS_PADDLE (SCREEN_LEN_SHORT-PADDLE_SIZE)
#define BARRIER_RIGHT (SCREEN_LEN_LONG-10)  // boundary on the right side of screen
#define BARRIER_LEFT 10                     // boundary on the left side of screen
#define LEFT_SCOREBOARD 0        // X coordinate for Slave1's scoreboard
#define RIGHT_SCOREBOARD 155     // X coordinate for Slave2's scoreboard

#define LEFT 0                   // enum for ball's directions
#define RIGHT 1                  // enum for ball's directions
#define STILL 2                  // enum for ball's directions


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

uint8_t MultiCastAddr[16] = {0};            // address used for multicast messages
static const int16_t MulticastHops = 10;    // # of hops multicast messages can   
uint8_t ReceiveBuffer[COMM_BUFF_SIZE];      // buffer that holds transmissions
bool Init_Mode = true;                      // determines wheter in init mode or game mode

float Slave1_Angle = PI/2;                  // latest angle stored in system for Slave1
int8_t Slave1_Old_Paddle_Top = 0;           // top pixel for slave1's previous paddle              
uint8_t Slave1_Score = 0;                   // Slave1's score

float Slave2_Angle = PI/2;                  // latest angle stored in system for Slave2
int8_t Slave2_Old_Paddle_Top = 0;           // top pixel for slave2's previous paddle 
uint8_t Slave2_Score = 0;                   // Slave2's score

uint8_t Ball_Position_X = 11;                   // current X coordinate for the ball
uint8_t Ball_Position_Y = SCREEN_LEN_SHORT/2;   // current Y coordinate for the ball

uint8_t Old_Ball_Position_X = Ball_Position_X;  // ball's previous X coordinate
uint8_t Old_Ball_Position_Y = Ball_Position_Y;  // ball's previous Y coordinate

uint8_t Ball_Path = RIGHT;                  // direction that ball is currently traveling
// ******************************************************************


// ****************** FUNCTIONS ***********************************


// ******** main() *******************************************
// about:  Initializes a new socket using UDP, configures button interrupts, 
//         and dispatches the queue.
// input:  *interface - interface used to create a UDP socket
// output: none
// ******************************************************************
int main(void){
  printf("Initializing master device\n");
  start_blinking(0.5, "red");

  // setup trace printing for thread debuging. Must first be enabled in mbed_app.json
  mbed_trace_init();
  mbed_trace_print_function_set(trace_printer);
  
  // setup the display
  TFT.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  TFT.setRotation(3);
  TFT.fillScreen(ST7735_BLACK);
  TFT.drawString(30, 10, (unsigned char*)("PADDLE"), ST7735_YELLOW, ST7735_BLACK, 3);

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

  // if something happens in socket (packets in or out), the callback is called.
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
      // split broadcasted message into segments using tokens 
      printf("Receiving packet from %s: %s\n",source_addr.get_ip_address()+IP_LAST4_OFFSET,ReceiveBuffer);
      char* message = (char*)ReceiveBuffer;           // convert ReceiveBuffer into a string.
      char* segment = strtok(message, " = ");         // tokenize message into segments
      int segmentIndex = 0;                           // segment's index in tokenized message

      // update the slaves' based off message data
      // Slave1
      if(source_addr.get_ip_address() == Slave1_Addr){
        // check if slave is serving
        if((strcmp(segment, "button") == 0) && (Ball_Position_X == BARRIER_LEFT)){
          Ball_Path = RIGHT;
        }

        // update slave's angle value
        if(strcmp(segment, "angle") == 0){            // check for angle command
          while (segment != NULL){                    // cycle through all tokens
            if(segmentIndex == 1){                    // grab value for angle here
              Slave1_Angle = strtof (segment, NULL);  // update Slave's angle. str to float
            }
            segment = strtok(NULL, " = ");            // advances segment for next iteration
            segmentIndex++;                           // next segment
          }
        } 
      }

      // Slave2
      else if(source_addr.get_ip_address() == Slave2_Addr){
        // check if slave is serving
        if((strcmp(segment, "button") == 0) && (Ball_Position_X == BARRIER_RIGHT)){
          Ball_Path = LEFT;
        }

        // update slave's angle value        
        if(strcmp(segment, "angle") == 0){            // check for angle command
          while (segment != NULL){                    // cycle through all tokens
            if(segmentIndex == 1){                    // grab value for angle here
              Slave2_Angle = strtof (segment, NULL);  // update Slave's angle. str to float
            }
            segment = strtok(NULL, " = ");            // advances segment for next iteration
            segmentIndex++;                           // next segment
          }
        }
      }
    }

    else if(length!=NSAPI_ERROR_WOULD_BLOCK){
      tr_error("Error happened when receiving %d\n", length);
      something_in_socket = false;
    }
    else{
      // there was nothing to read.
      something_in_socket = false;
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
        printf("Slave4 assigned\n");
        TFT.drawString(0, 110, (unsigned char*)("--Player 4 paired"), ST7735_WHITE, ST7735_BLACK, 1); 
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
// about:  Function that calls printf. Enabled by setting in mbed_app.json
// input:  *str - pointer to string that will be printed
// output: none
// ******************************************************************
void trace_printer(const char* str){
  printf("%s\n", str);
}


// ******** game()************************************************
// about:  A thread which handles all things related to the game
// input:  none
// output: none
// ***************************************************************
void game(void){
  //erase old objects on screen
  TFT.drawFastVLine(BARRIER_LEFT, Slave1_Old_Paddle_Top, PADDLE_SIZE, ST7735_GREEN);    // erase Slave1's paddle
  TFT.drawFastVLine(BARRIER_RIGHT, Slave2_Old_Paddle_Top, PADDLE_SIZE, ST7735_GREEN);   // erase Slave2's paddle
  TFT.drawBall(Old_Ball_Position_X, Old_Ball_Position_Y, ST7735_GREEN); // erase ball's position from previous cycle

  // calculate and draw paddles
  float slave1_paddle_top = SCREEN_MINUS_PADDLE-(abs(Slave1_Angle)/ANGLE_DIV);    // translate angle to pixel location
  float slave2_paddle_top = SCREEN_MINUS_PADDLE-(abs(Slave2_Angle)/ANGLE_DIV);    // translate angle to pixel location
  TFT.drawFastVLine(BARRIER_LEFT, slave1_paddle_top, PADDLE_SIZE, ST7735_BLACK);  // draw Slave1's paddle
  TFT.drawFastVLine(BARRIER_RIGHT, slave2_paddle_top, PADDLE_SIZE, ST7735_BLACK); // draw Slave2's paddle

  // calculate ball's current x trajectory
  if(Ball_Path == RIGHT){
    Ball_Position_X++;
  }
  else if(Ball_Path == LEFT){
    Ball_Position_X--;
  }

  // determine if a goal was made, or the ball hit a paddle
  goalCheck(slave1_paddle_top, slave2_paddle_top);  

  // draw the ball in its updated position
  TFT.drawBall(Ball_Position_X, Ball_Position_Y, ST7735_RED);

  // update objects' old values
  Slave1_Old_Paddle_Top = slave1_paddle_top;        // update Slave1's old pixel
  Slave2_Old_Paddle_Top = slave2_paddle_top;        // update old pixel
  Old_Ball_Position_X = Ball_Position_X;            // update ball's old x position
  Old_Ball_Position_Y = Ball_Position_Y;            // update ball's old y position
}


// ******** goalCheck() *******************************************
// about:  Determine if a goal was made, or the ball hit a paddle
// input:  slave1_paddle_top - the top pixel for slave1's paddle
//         slave2_paddle_top - the top pixel for slave2's paddle
// output: none
// ***************************************************************
void goalCheck(float slave1_paddle_top, float slave2_paddle_top){
  // TODO make sure below is similar for both cases
  // check if the ball reached the barrier on the right side of the screen 
  if(Ball_Position_X == BARRIER_RIGHT){

    // check if the ball hit the right paddle
    if((Ball_Position_Y >= Slave2_Old_Paddle_Top) && (Ball_Position_Y <= Slave2_Old_Paddle_Top + PADDLE_SIZE) && (Ball_Path != STILL)){
      Ball_Path = LEFT;
    }

    // else if the ball scored a goal on right side of screen
    else{
      // place ball in center of right paddle
      Ball_Position_X = BARRIER_RIGHT;
      Ball_Position_Y = slave2_paddle_top + (PADDLE_SIZE/2);

      if(Ball_Path != STILL){
        Ball_Path = STILL;

        // update score
        Slave1_Score++;
        char buff[8];
        itoa(Slave1_Score,buff,10);           
        TFT.drawString(0, 0, (unsigned char*)(buff), ST7735_WHITE, ST7735_BLACK, 1);   
      }
      
      // check for winning game condition
      if(Slave1_Score >= MAX_SCORE){
        // Slave1 wins
      }
    }
  }

  // TODO make sure above is similar for both cases
  // else if the ball reached the barrier on the left side of the screen 
    else if(Ball_Position_X == BARRIER_LEFT){
    // check if the ball hit the left paddle
    if((Ball_Position_Y >= Slave1_Old_Paddle_Top) && (Ball_Position_Y <= Slave1_Old_Paddle_Top + PADDLE_SIZE) && (Ball_Path != STILL)){
      Ball_Path = RIGHT;
    }

    // else if the ball scored a goal on the left side of screen
    else{
      // place ball in center of left paddle
      Ball_Position_X = BARRIER_LEFT;
      Ball_Position_Y = slave1_paddle_top + (PADDLE_SIZE/2);

      if(Ball_Path != STILL){
        Ball_Path = STILL;

        // update score
        Slave2_Score++;
        char buff[8];
        itoa(Slave2_Score,buff,10);           
        TFT.drawString(SCREEN_LEN_LONG-5, 0, (unsigned char*)(buff), ST7735_WHITE, ST7735_BLACK, 1);   
      }

      // check for winning game condition
      if(Slave2_Score >= MAX_SCORE){
        // Slave2 wins
      }
    }
  }

  return;
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
  // if game is currently in initialization mode
  if(Init_Mode){
    Init_Mode = false;                             // finishing initilization mode
    cancel_blinking();                             // turn off last stages heartbeat
    start_blinking(0.5, "blue");                   // change LED color to signify next state
    Queue1.call(sendMessage, "Init complete\n");   // output to console

    // draw the field
    TFT.fillScreen(ST7735_GREEN);
    TFT.drawFastVLine(80, 0, 128, ST7735_BLACK);
    TFT.drawCircle(80, 64, 10, ST7735_BLACK);

    // TODO draw a line for debug
    uint16_t x1, y1;
    uint16_t x2, y2;

    x1 = 10;
    y1 = 10;
    x2 = 100;
    y2 = 100;

    TFT.drawLine(x1, y1, x2, y2, ST7735_BLACK);

    // draw the score board
    char buff[8];
    itoa(Slave1_Score,buff,10);           
    TFT.drawString(LEFT_SCOREBOARD, 0, (unsigned char*)(buff), ST7735_WHITE, ST7735_BLACK, 1);   
    itoa(Slave2_Score,buff,10);
    TFT.drawString(RIGHT_SCOREBOARD, 0, (unsigned char*)(buff), ST7735_WHITE, ST7735_BLACK, 1);    

    // specify rate to rerun game() after button press. 
    Queue1.call_every(GAME_CALL_RATE,game);
  }

  // else if initialization is complete and game is running
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

