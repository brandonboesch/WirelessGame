// Filename: master.h
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#ifndef MASTER_H
#define MASTER_H

#include "fifo.h"

// ****************** CLASSES *******************************

// ******** Coord *******************************************
// about:  x and y cordinates for ball's path fifo.
// input:  none
// output: none
// **********************************************************
class Coord{
  public:
    int16_t x;
    int16_t y;
};


// ****************** FUNCTION PROTOTYPES *******************
int main(void);
void trace_printer(const char* str);
void sendMessage(const char messageBuff[COMM_BUFF_SIZE]); 
void receiveMessage();
void pairSlaves();

void fillLineBuffer(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void goalCheck(float slave1_paddle_top, float slave2_paddle_top);
void game(void);
void wallCheck(Coord ball_coord_start, Coord ball_coord_current, bool ball_still);
void gameOver(uint8_t playerNumber);
  
void socket_isr();
void myButton_isr();


#endif // MASTER_H
