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

#ifndef __DIGIT_DISPLAY__
#define __DIGIT_DISPLAY__

#include "mbed.h"

class DigitDisplay
{
public:
    DigitDisplay(PinName clk, PinName dio);
    
    /**
     * Display a decimal number, A number larger than 10000 or smaller than -1000 will be truncated, MSB will be lost.
     */
    void write(int16_t number);
    
    /**
     * Display four numbers
     * \param   numbers     0 - 0xF:     0 - 9, A, b, C, d, E, F
     *                      0x10 - 0xFE: '_'(unknown)
     *                      0xFF:        ' '(null)
     */
    void write(uint8_t numbers[]);
    
    /**
     * Display a number in a specific position
     * \param   position    0 - 3, left to right
     * \param   number      0 - 0xF:     0 - 9, A, b, C, d, E, F
     *                      0x10 - 0xFE: '_'
     *                      0xFF:        ' '
     */
    void write(uint8_t position, uint8_t number);
    
    void writeRaw(uint8_t position, uint8_t segments);
    void writeRaw(uint8_t segments[]);
    
    void clear();
    void on();
    void off();
    
    /**
     * Set the brightness, level 0 to level 7
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * Enable/Disable the colon
     */
    void setColon(bool enable);
    
    DigitDisplay& operator= (int16_t number) {
        write(number);
        return *this;
    }
    
private:
    void start();
    bool send(uint8_t data);
    void stop();
    
    DigitalOut   _clk;
    DigitalInOut _dio;
    bool         _off;
    bool         _colon;
    uint8_t      _brightness;
    uint8_t      _content[4];
};

#endif // __DIGIT_DISPLAY__
