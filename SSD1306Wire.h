/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
 * Copyright (c) 2018 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ThingPulse invests considerable time and money to develop these open source libraries.
 * Please support us by buying our products (and not the clones) from
 * https://thingpulse.com
 *
 */

#ifndef SSD1306Wire_h
#define SSD1306Wire_h

#include "OLEDDisplay.h"
//#include <Wire.h>

class SSD1306Wire : public OLEDDisplay {
  private:
      uint8_t             _address;
      uint8_t             _sda;
      uint8_t             _scl;
      bool                _doI2cAutoInit = false;

  public:
    SSD1306Wire(uint8_t _address, uint8_t _sda, uint8_t _scl, OLEDDISPLAY_GEOMETRY g = GEOMETRY_128_64) {
      setGeometry(g);

      this->_address = _address;
      this->_sda = _sda;
      this->_scl = _scl;
    }

      #define SSD1306_CMD_DISPLAY_OFF 0xAE//--turn off the OLED
      #define SSD1306_CMD_DISPLAY_ON 0xAF//--turn on oled panel 

    bool connect() {

      pinMode(this->_sda,OUTPUT);
      pinMode(this->_scl,OUTPUT);
      
      sendCommand(SSD1306_CMD_DISPLAY_OFF);//display off
      sendCommand(0x00);//Set Memory Addressing Mode
      sendCommand(0x10);//00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
      sendCommand(0x40);//Set Page Start Address for Page Addressing Mode,0-7
      sendCommand(0xB0);//Set COM Output Scan Direction
      sendCommand(0x81);//---set low column address
      sendCommand(0xCF);//---set high column address
      sendCommand(0xA1);//--set start line address
      sendCommand(0xA6);//--set contrast control register
      sendCommand(0xA8);
      sendCommand(0x3F);//--set segment re-map 0 to 127
      sendCommand(0xC8);//--set normal display
      sendCommand(0xD3);//--set multiplex ratio(1 to 64)
      sendCommand(0x00);//
      sendCommand(0xD5);//0xa4,Output follows RAM content;0xa5,Output ignores RAM content
      sendCommand(0x80);//-set display offset
      sendCommand(0xD9);//-not offset
      sendCommand(0xF1);//--set display clock divide ratio/oscillator frequency
      sendCommand(0xDA);//--set divide ratio
      sendCommand(0x12);//--set pre-charge period
      sendCommand(0xDB);//
      sendCommand(0x40);//--set com pins hardware configuration
      sendCommand(0x8D);//--set vcomh
      sendCommand(0x14);//0x20,0.77xVcc
      sendCommand(0xAF);//--set DC-DC enable
      sendCommand(SSD1306_CMD_DISPLAY_ON);//--turn on oled panel 
    
      delay(100);//wait for the screen loaded.
      return true;
    }

    void display(void) {
      initI2cIfNeccesary();
      const int x_offset = (128 - this->width()) / 2;
      #ifdef OLEDDISPLAY_DOUBLE_BUFFER
        uint8_t minBoundY = UINT8_MAX;
        uint8_t maxBoundY = 0;

        uint8_t minBoundX = UINT8_MAX;
        uint8_t maxBoundX = 0;
        uint8_t x, y;

        // Calculate the Y bounding box of changes
        // and copy buffer[pos] to buffer_back[pos];
        for (y = 0; y < (this->height() / 8); y++) {
          for (x = 0; x < this->width(); x++) {
           uint16_t pos = x + y * this->width();
           if (buffer[pos] != buffer_back[pos]) {
             minBoundY = _min(minBoundY, y);
             maxBoundY = _max(maxBoundY, y);
             minBoundX = _min(minBoundX, x);
             maxBoundX = _max(maxBoundX, x);
           }
           buffer_back[pos] = buffer[pos];
         }
         yield();
        }

        // If the minBoundY wasn't updated
        // we can savely assume that buffer_back[pos] == buffer[pos]
        // holdes true for all values of pos

        if (minBoundY == UINT8_MAX) return;

        sendCommand(COLUMNADDR);
        sendCommand(x_offset + minBoundX);
        sendCommand(x_offset + maxBoundX);

        sendCommand(PAGEADDR);
        sendCommand(minBoundY);
        sendCommand(maxBoundY);

        byte k = 0;
        for (y = minBoundY; y <= maxBoundY; y++) {
          for (x = minBoundX; x <= maxBoundX; x++) {
            if (k == 0) {
              startIIC();  //Pass addresss???
              writeByte(0x78);  //Slave address,SA0=0
              writeByte(0x40);
            }

            writeByte(buffer[x + y * this->width()]);
            k++;
            if (k == 16)  {
              stopIIC();
              k = 0;
            }
          }
          yield();
        }

        if (k != 0) {
          stopIIC();
        }
      #else

        sendCommand(COLUMNADDR);
        sendCommand(x_offset);
        sendCommand(x_offset + (this->width() - 1));

        sendCommand(PAGEADDR);
        sendCommand(0x0);
        sendCommand((this->height() / 8) - 1);

        if (geometry == GEOMETRY_128_64) {
          sendCommand(0x7);
        } else if (geometry == GEOMETRY_128_32) {
          sendCommand(0x3);
        }

        for (uint16_t i=0; i < displayBufferSize; i++) {
          startIIC();
          writeByte(0x78);  //Slave address,SA0=0
          sendByte(0x40);
          for (uint8_t x = 0; x < 16; x++) {
            sendByte(buffer[i]);
            i++;
          }
          i--;
          stopIIC();
        }
      #endif
    }

    void setI2cAutoInit(bool doI2cAutoInit) {
      _doI2cAutoInit = doI2cAutoInit;
    }

  private:
    inline void sendCommand(uint8_t command) __attribute__((always_inline)){
      writeCommand(command);
    }

    void initI2cIfNeccesary() {
      if (_doI2cAutoInit) {
//        Wire.begin(this->_sda, this->_scl);
      }
    }

/* 
 *  Following copied from Adafruit libraries. 
 *  
 */

void writeCommand(unsigned char cmd)
{
  startIIC();
  writeByte(0x78);  //Slave address,SA0=0
  writeByte(0x00);  //write command
  writeByte(cmd);
  stopIIC();
}
void writeByte(unsigned char b)
{
  unsigned char i;
  for(i=0;i<8;i++)
  {
    if((b << i) & 0x80){
      digitalWrite(this->_sda, HIGH);
    }else{
      digitalWrite(this->_sda, LOW);
    }
    digitalWrite(this->_scl, HIGH);
    digitalWrite(this->_scl, LOW);
    //    IIC_Byte<<=1;
  }
  digitalWrite(this->_sda, HIGH);
  digitalWrite(this->_scl, HIGH);
  
  digitalWrite(this->_scl, LOW);
}
void startIIC()
{
  digitalWrite(this->_scl, HIGH);
  digitalWrite(this->_sda, HIGH);
  digitalWrite(this->_sda, LOW);
  digitalWrite(this->_scl, LOW);
}
void stopIIC()
{
  digitalWrite(this->_scl, LOW);
  digitalWrite(this->_sda, LOW);
  digitalWrite(this->_scl, HIGH);
  digitalWrite(this->_sda, HIGH);  
  delay(1);
}






};

#endif
