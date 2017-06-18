// Filename: main.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

/*
 * Copyright (c) 2016 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include "rtos.h"
#include "NanostackInterface.h"
#include "mbed-trace/mbed_trace.h"
#include "master.h"

#define ATMEL   1
#define MCR20   2
#define NCS36510 3
#define KW24D   4

#define MESH_LOWPAN     3
#define MESH_THREAD     4


// ****************** PROTOTYPES ************************************
void trace_printer(const char* str);
void start_blinking(float freq, const char* color);
void cancel_blinking();
void blink_red();
void blink_green();
void blink_blue();
// ******************************************************************


// ******************** GLOBALS *************************************
#if MBED_CONF_APP_RADIO_TYPE == ATMEL
#include "NanostackRfPhyAtmel.h"
NanostackRfPhyAtmel rf_phy(ATMEL_SPI_MOSI, ATMEL_SPI_MISO, ATMEL_SPI_SCLK, ATMEL_SPI_CS,
                           ATMEL_SPI_RST, ATMEL_SPI_SLP, ATMEL_SPI_IRQ, ATMEL_I2C_SDA, ATMEL_I2C_SCL);
#elif MBED_CONF_APP_RADIO_TYPE == MCR20 || MBED_CONF_APP_RADIO_TYPE == KW24D 
#include "NanostackRfPhyMcr20a.h"
NanostackRfPhyMcr20a rf_phy(MCR20A_SPI_MOSI, MCR20A_SPI_MISO, MCR20A_SPI_SCLK, MCR20A_SPI_CS, MCR20A_SPI_RST, MCR20A_SPI_IRQ);

#elif MBED_CONF_APP_RADIO_TYPE == NCS36510
#include "NanostackRfPhyNcs36510.h"
NanostackRfPhyNcs36510 rf_phy;
#endif //MBED_CONF_APP_RADIO_TYPE

#if MBED_CONF_APP_MESH_TYPE == MESH_LOWPAN
LoWPANNDInterface mesh;
#elif MBED_CONF_APP_MESH_TYPE == MESH_THREAD
ThreadInterface mesh;
#endif //MBED_CONF_APP_MESH_TYPE

Ticker Ticker1;                   // for LED blinking
DigitalOut LED_1(LED_RED, 1);     // onboard red LED
DigitalOut LED_2(LED_GREEN, 1);   // onboard green LED
DigitalOut LED_3(LED_BLUE, 1);    // onboard blue LED
// ******************************************************************


// ****************** FUNCTIONS *************************************

// ******** trace_printer() *****************************************
// about:  Function that calls printf
// input:  *str - pointer to string that will be printed
// output: none
// ******************************************************************

void trace_printer(const char* str){
  printf("%s\n", str);
}


// ******** start_blinking() ************************************
// about:  Creates a thread every second which blinks the red LED
// input:  freq - freq in which to blink led
//         color - color led you wish to turn on, in string format
// output: none
// ******************************************************************
void start_blinking(float freq, const char* color){
  if(strcmp("red", color) == 0)   Ticker1.attach(blink_red, freq);
  if(strcmp("green", color) == 0) Ticker1.attach(blink_green, freq);
  if(strcmp("blue", color) == 0)  Ticker1.attach(blink_blue, freq);
}


// ******** cancel_blinking() ***********************************
// about:  kill blinking led thread, and turn off all led colors
// input:  none
// output: none
// ******************************************************************
void cancel_blinking(){
  Ticker1.detach();
  LED_1 = 1;
  LED_2 = 1;
  LED_3 = 1;
}


// ******** blink_red() ********************************************
// about:  flip the status of the red LED 
// input:  none
// output: none
// *****************************************************************
void blink_red(){
  LED_1 = !LED_1;
}


// ******** blink_green() ********************************************
// about:  flip the status of the green LED 
// input:  none
// output: none
// *****************************************************************
void blink_green(){
  LED_2 = !LED_2;
}


// ******** blink_blue() ********************************************
// about:  flip the status of the blue LED 
// input:  none
// output: none
// *****************************************************************
void blink_blue(){
  LED_3 = !LED_3;
}


// ******** main() ************************************************
// about:  Initialize target to IP address and kick off comms application 
// input:  none
// output: none
// *****************************************************************
int main(){
  mbed_trace_init();
  mbed_trace_print_function_set(trace_printer);

  if(MBED_CONF_APP_BUTTON != NC && MBED_CONF_APP_LED != NC){
    start_blinking(0.5, "red");
  }
  else{
    printf("pins not configured correctly");
  }

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

  if(MBED_CONF_APP_BUTTON != NC && MBED_CONF_APP_LED != NC){
    cancel_blinking();
    start_blinking(0.5, "green");
    masterInit((NetworkInterface *)&mesh); 
  }
}
