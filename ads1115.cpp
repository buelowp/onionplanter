/*
 * Copyright (c) 2020 <copyright holder> <email>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ads1115.h"

ads1115::ads1115(int connected) : m_numConnected(connected), m_enabled(false)
{
    m_fd = -1;
}

ads1115::~ads1115()
{

}

bool ads1115::open(int address)
{
    m_address = address;
    if ((m_fd = ::open("/dev/i2c-0", O_RDWR)) < 0) {
        fprintf(stderr, "%s: open: %s(%d)\n", __PRETTY_FUNCTION__, strerror(errno), errno);
        return false;
    }

    // connect to ads1115 as i2c slave
    if (ioctl(m_fd, I2C_SLAVE, m_address) < 0) {
        fprintf(stderr, "%s: ioctl: %s(%d)\n", __PRETTY_FUNCTION__, strerror(errno), errno);
        return false;
    }
    
    return true;
}

void ads1115::close()
{
    ::close(m_fd);
    m_fd = -1;
}

bool ads1115::rawValues(std::map<int,int> &values)
{
    uint8_t writeBuf[3];
    uint8_t readBuf[2];
    int16_t val;
    
    std::map<int, int> readings;
    
    for (int i = 0; i < m_numConnected; i++) {
        // set config register and start conversion
        // ANC1 and GND, 4.096v, 128s/s
        writeBuf[0] = 1;        // config register is 1
        // bit 15 flag bit for single shot
        // Bits 14-12 input selection:
        // 100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3
        // Bits 11-9 Amp gain. Default to 010 here 001 P19
        // Bit 8 Operational mode of the ADS1115.
        // 0 : Continuous conversion mode
        // 1 : Power-down single-shot mode (default)
        switch (i) {
            case 0:
                writeBuf[1] = 0b11000011; // bit 15-8 0xD3
                break;
            case 1:
                writeBuf[1] = 0b11010011; // bit 15-8 0xD3
                break;
            case 2:
                writeBuf[1] = 0b11100011; // bit 15-8 0xD3
                break;
            case 3:
                writeBuf[1] = 0b11110011; // bit 15-8 0xD3
                break;
        }
        // Bits 7-5 data rate default to 100 for 128SPS
        // Bits 4-0    comparator functions see spec sheet.
        writeBuf[2] = 0b10000101;

        // begin conversion
        if (write(m_fd, writeBuf, 3) != 3) {
            fprintf(stderr, "%s: write: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
            return false;
        }

        // wait for conversion complete
        // checking bit 15
        do {
            if (read(m_fd, writeBuf, 2) != 2) {
                fprintf(stderr, "%s: read: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
                return false;
            }
        }
        while ((writeBuf[0] & 0x80) == 0);

        // read conversion register
        // write register pointer first
        readBuf[0] = 0;     // conversion register is 0
        if (write(m_fd, readBuf, 1) != 1) {
            fprintf(stderr, "%s: write: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
            return false;
        }
        
        // read 2 bytes
        if (read(m_fd, readBuf, 2) != 2) {
            fprintf(stderr, "%s: read: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
            return false;
        }

        // convert display results
        val = readBuf[0] << 8 | readBuf[1];

        if (val < 0)     
            val = 0;

        values[i] = val;
    }

    return true;
}

std::map<int, double> ads1115::voltages()
{
    uint8_t writeBuf[3];
    uint8_t readBuf[2];
    int16_t val;
    
    std::map<int, double> readings;
    
    for (int i = 0; i < m_numConnected; i++) {
        // set config register and start conversion
        // ANC1 and GND, 4.096v, 128s/s
        writeBuf[0] = 1;        // config register is 1
        // bit 15 flag bit for single shot
        // Bits 14-12 input selection:
        // 100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3
        // Bits 11-9 Amp gain. Default to 010 here 001 P19
        // Bit 8 Operational mode of the ADS1115.
        // 0 : Continuous conversion mode
        // 1 : Power-down single-shot mode (default)
        switch (i) {
            case 0:
                writeBuf[1] = 0b11000011; // bit 15-8 0xD3
                break;
            case 1:
                writeBuf[1] = 0b11010011; // bit 15-8 0xD3
                break;
            case 2:
                writeBuf[1] = 0b11100011; // bit 15-8 0xD3
                break;
            case 3:
                writeBuf[1] = 0b11110011; // bit 15-8 0xD3
                break;
        }
        // Bits 7-5 data rate default to 100 for 128SPS
        // Bits 4-0    comparator functions see spec sheet.
        writeBuf[2] = 0b10000101;

        // begin conversion
        if (write(m_fd, writeBuf, 3) != 3) {
            fprintf(stderr, "%s: write: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
            exit(-1);
        }

        // wait for conversion complete
        // checking bit 15
        do {
            if (read(m_fd, writeBuf, 2) != 2) {
            fprintf(stderr, "%s: read: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
                exit(-1);
            }
        }
        while ((writeBuf[0] & 0x80) == 0);

        // read conversion register
        // write register pointer first
        readBuf[0] = 0;     // conversion register is 0
        if (write(m_fd, readBuf, 1) != 1) {
            fprintf(stderr, "%s: write: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
            exit(-1);
        }
        
        // read 2 bytes
        if (read(m_fd, readBuf, 2) != 2) {
            fprintf(stderr, "%s: read: %s(%d)\n", __FUNCTION__, strerror(errno), errno);
            exit(-1);
        }

        // convert display results
        val = readBuf[0] << 8 | readBuf[1];

        if (val < 0)     
            val = 0;

        readings[i] = (val * VPS);
    }

    if (readings.size())
        return readings;
}
