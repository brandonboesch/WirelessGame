/* The library of Grove - 4 Digit Display
 *
 * \author  Yihui Xiong
 * \date    2014/2/8
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Seeed Technology Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "DigitDisplay.h"

#define ADDR_AUTO  0x40
#define ADDR_FIXED 0x44

#define POSITION_COLON 1

#define DIGIT_UNKOWN 0x08
#define DIGIT_NULL   0x00
#define DIGIT_MINUS  0x40

const uint8_t DIGIT_TABLE[] = {0x3f, 0x06, 0x5b, 0x4f,
                              0x66, 0x6d, 0x7d, 0x07,
                              0x7f, 0x6f, 0x77, 0x7c,
                              0x39, 0x5e, 0x79, 0x71
                             }; //0~9,A,b,C,d,E,F


inline uint8_t conv(uint8_t n)
{
    uint8_t segments;
    
    if (n <= sizeof(DIGIT_TABLE)) {
        segments = DIGIT_TABLE[n];
    }else if (n == 0xFF) {
        segments = DIGIT_NULL;
    } else {
        segments = DIGIT_UNKOWN;
    }
    
    return segments;
}

DigitDisplay::DigitDisplay(PinName clk, PinName dio) : _clk(clk), _dio(dio)
{
    _dio.output();
    _dio = 1;
    _clk = 1;
    
    _brightness = 2;
    _colon      = false;
    _off        = true;
    
    for (uint8_t i = 0; i < sizeof(_content); i++) {
        _content[i] = DIGIT_NULL;
    }
}

void DigitDisplay::on()
{
    start();
    send(0x88 | _brightness);
    stop();
}

void DigitDisplay::off()
{
    start();
    send(0x80);
    stop();
}

void DigitDisplay::setBrightness(uint8_t brightness)
{
    if (brightness > 7) {
        brightness = 7;
    }
    
    _brightness = brightness;
    
    start();
    send(0x88 | _brightness);
    stop();
}

void DigitDisplay::setColon(bool enable)
{
    if (_colon != enable) {
        _colon = enable;
        
        if (enable) {
            _content[POSITION_COLON] |= 0x80;
        } else {
            _content[POSITION_COLON] &= 0x7F;
        }
        
        writeRaw(POSITION_COLON, _content[POSITION_COLON]);
    }    
}
    
void DigitDisplay::write(int16_t n)
{
    uint8_t negative = 0;
    
    if (n < 0) {
        negative = 1;
        n = (-n) % 1000;
    } else {
        n = n % 10000;
    }
    
    int8_t i = 3;
    do {
        uint8_t  r = n % 10;
        _content[i] = conv(r);
        i--;
        n = n / 10;
    } while (n != 0);
    
    if (negative) {
        _content[i] = DIGIT_MINUS;
        i--;
    }
    
    for (int8_t j = 0; j <= i; j++) {
        _content[j] = DIGIT_NULL;
    }
    
    if (_colon) {
        _content[POSITION_COLON] |= 0x80;
    }
    
    writeRaw(_content);
}

void DigitDisplay::write(uint8_t numbers[])
{
    for (uint8_t i = 0; i < 4; i++) {
        _content[i] = conv(numbers[i]);
    }
    
    if (_colon) {
        _content[POSITION_COLON] |= 0x80;
    }
    
    start();
    send(ADDR_AUTO);
    stop();
    start();
    send(0xC0);
    for (uint8_t i = 0; i < 4; i++) {
        send(_content[i]);
    }
    stop();
    
    if (_off) {
        _off = 0;
        start();
        send(0x88 | _brightness);
        stop();
    }
}

void DigitDisplay::write(uint8_t position, uint8_t number)
{
    if (position >= 4) {
        return;
    }
    
    uint8_t segments = conv(number);
    
    if ((position == POSITION_COLON) && _colon) {
        segments |= 0x80;
    }
    
    _content[position] = segments;
    
    start();
    send(ADDR_FIXED);
    stop();
    start();
    send(0xC0 | position);
    send(segments);
    stop();
    
    if (_off) {
        _off = 0;
        start();
        send(0x88 | _brightness);
        stop();
    }
}

void DigitDisplay::writeRaw(uint8_t segments[])
{
    for (uint8_t i = 0; i < 4; i++) {
        _content[i] = segments[i];
    }
    
    start();
    send(ADDR_AUTO);
    stop();
    start();
    send(0xC0);
    for (uint8_t i = 0; i < 4; i++) {
        send(segments[i]);
    }
    stop();
    
    if (_off) {
        _off = 0;
        start();
        send(0x88 | _brightness);
        stop();
    }
}

void DigitDisplay::writeRaw(uint8_t position, uint8_t segments)
{
    if (position >= 4) {
        return;
    }
    
    _content[position] = segments;

    start();
    send(ADDR_FIXED);
    stop();
    start();
    send(0xC0 | position);
    send(segments);
    stop();

    if (_off) {
        _off = 0;
        start();
        send(0x88 | _brightness);
        stop();
    }
}

void DigitDisplay::clear()
{
    for (uint8_t i = 0; i < 4; i++) {
        _content[i] = DIGIT_NULL;
    }
    _colon = false;
    
    writeRaw(0, DIGIT_NULL);
    writeRaw(1, DIGIT_NULL);
    writeRaw(2, DIGIT_NULL);
    writeRaw(3, DIGIT_NULL);
}
    
void DigitDisplay::start()
{
    _clk = 1;
    _dio = 1;
    _dio = 0;
    _clk = 0;
}

bool DigitDisplay::send(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        _clk = 0;
        _dio = data & 1;
        data >>= 1;
        _clk = 1;
    }
    
    // check ack
    _clk = 0;
    _dio = 1;
    _clk = 1;
    _dio.input();
    
    uint16_t count = 0;
    while (_dio) {
        count++;
        if (count >= 200) {
            _dio.output();
            return false;
        }
    }
    
    _dio.output();
    return true;
}

void DigitDisplay::stop()
{
    _clk = 0;
    _dio = 0;
    _clk = 1;
    _dio = 1;
}


