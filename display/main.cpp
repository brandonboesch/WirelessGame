#include "mbed.h"
#include "Adafruit_ST7735.h"

// There are two different SPI ports that can be utilized on the K64F. Update the tft global below to match whichever pins your screen is connected to.

// ST7735 Interface (Option 1)
//   LITE connected to +3.3 V
//   MISO connected to PTD7
//   SCK connected to PTD5
//   MOSI connected to PTD6
//   TFT_CS connected to PTD4
//   CARD_CS unconnected 
//   D/C connected to PTC18
//   RESET connected to +3.3 V
//   VCC connected to +3.3 V
//   Gnd connected to ground

// ST7735 Interface (Option 2)
//   LITE connected to +3.3 V
//   MISO connected to D12
//   SCK connected to D13
//   MOSI connected to D11
//   TFT_CS connected to D10
//   CARD_CS unconnected 
//   D/C connected to D8
//   RESET connected to D9
//   VCC connected to +3.3 V
//   Gnd connected to ground


Adafruit_ST7735 tft(PTD6, PTD7, PTD5, PTD4, PTC18, PTC15); // MOSI, MISO, SCK, TFT_CS, D/C, RESET

int main(void){
  // Use this initializer if you're using a 1.8" TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  tft.fillScreen(ST7735_BLUE);
}
