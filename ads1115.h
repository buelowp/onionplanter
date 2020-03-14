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

#ifndef ADS1115_H
#define ADS1115_H

#include <map>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h> // open
#include <fcntl.h>
#include <linux/i2c-dev.h>

class ads1115
{
public:
    static constexpr double VPS = 4.096 / 32768.0; //volts per step
    
    ads1115(int);
    ~ads1115();

    bool open(int);
    void close();
    std::map<int, int> rawValues();
    std::map<int, double> voltages();
    
private:
    int m_fd;
    int m_numConnected;
    int m_address;
    bool m_enabled;
};

#endif // ADS1115_H
