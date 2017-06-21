#include "mbed.h"
#include "Adafruit_ST7735.h"



////////////////////////////////////////////////////////////////////////////////
// ST7735 Interface

// LITE connected to +3.3 V
// MISO connected to D12
// SCK connected to D13
// MOSI connected to D11
// TFT_CS connected to D10
// CARD_CS unconnected 
// Data/Command connected to D8
// RESET connected to D9
// VCC connected to +3.3 V
// Gnd connected to ground

Adafruit_ST7735 tft(D11, D12, D13, D10, D8, D9); // MOSI, MISO, SCLK, TFT_CS, D/C, RESET


int main(void){
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLUE);
}
