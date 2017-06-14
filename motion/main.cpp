// Filename: main.cpp
// Built by: Brandon Boesch
// Date    : June 12th, 2017

#include "mbed.h"
#include "FXOS8700CQ.h"

FXOS8700CQ device(I2C_SDA,I2C_SCL);      // create object and specifiy pins
Serial pc(USBTX, USBRX);                 //use these pins for serial coms.
int main(){
  pc.baud(115200);                       //Set baudrate here.
  device.init();                         // call initialization method
  while (1) {
    // poll the sensor and get the values, storing in a struct
    Data values = device.get_values();
        
    // print each struct member over serial
    printf("ax = %f ay = %f az = %f \n\r",values.ax, values.ay, values.az);
    wait(1);
  }
}
