#include "mbed.h"
#include "DigitDisplay.h"

AnalogIn pot(A3);
DigitDisplay display(A0, A1);

int main() {
  float ain;
  display.on();
	while(1) {
		ain = pot.read();
		unsigned short val = floor(ain * 100);
		display.write(val);
		while(ain == pot.read()) { }
	}
}
