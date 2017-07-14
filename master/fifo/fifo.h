/*
 * fifo.h
 *
 *  Created on: May 21, 2011
 *      Author: Sagar
 */

#ifndef FIFO_H_
#define FIFO_H_

#include "mbed.h"
#include "master.h"

#define FIFO_SIZE 256
#define FIFO_TYPE Coord

class fifo{
	FIFO_TYPE buffer[FIFO_SIZE];
	uint32_t head, tail;

public:
	fifo();
  void reset();
	uint8_t put(FIFO_TYPE data);// returns 0 on success
	uint8_t get(FIFO_TYPE* data);
	uint32_t available();
	uint32_t free();
};


#endif /* FIFO_H_ */
