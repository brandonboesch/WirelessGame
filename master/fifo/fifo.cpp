/*
 * fifo.cpp
 *
 *  Created on: May 21, 2011
 *      Author: Sagar
 */

#include "fifo.h"

fifo::fifo(){
	this->head = 0;
	this->tail = 0;
}

// resets the fifo to zero elements
void fifo::reset(){
	this->head = 0;
	this->tail = 0;
}


// returns how many items are available in the queue
uint32_t fifo::available(){
	return (FIFO_SIZE + this->head - this->tail) % FIFO_SIZE;
}


uint32_t fifo::free(){
	return (FIFO_SIZE - 1 - available());
}


// add an object to the fifo. returns 0 if full, 1 on success
uint8_t fifo::put(FIFO_TYPE data){
	uint32_t next;

	// check if FIFO has room
	next = (this->head + 1) % FIFO_SIZE;
	if (next == this->tail)
	{
		// full
		return 0;
	}

	this->buffer[this->head] = data;
	this->head = next;

	return 1;
}


// get an item from the fifo. returns 0 if empty, 1 on success
uint8_t fifo::get(FIFO_TYPE* data){
	uint32_t next;

	// check if FIFO has data
	if (this->head == this->tail){
		return 0; // FIFO empty
	}

	next = (this->tail + 1) % FIFO_SIZE;

	*data = this->buffer[this->tail];
	this->tail = next;

	return 1;
}
