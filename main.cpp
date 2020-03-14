#include <map>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iostream>
#include <oled-exp.h>
#include <relay-exp.h>
#include "ads1115.h"

int main(int argc, char *argv[])
{
    int relay = 0;
    ads1115 adc(3);
    char olederror[] = "Error starting ADC";
    
    oledDriverInit();
    oledSetTextColumns();
    
    relayDriverInit(7);
    relayCheckInit (7, &relay);
    
    if (relay == 0) {
        std::cout << "Unable to initialize relay board" << std::endl;
        exit(-1);
    }
    
    if (!adc.open(0x49)) {
        oledWrite(olederror);
        exit(-1);
    }
    
    while (1) {
        std::map<int, int> values = adc.rawValues();
        
        oledClear();
        for (int i = 0; i < 3; i++) {
            std::string oled;
            auto it = values.find(i);
            if (it != values.end()) {
                oled.append("Planter " + std::to_string(i) + ": ");
                if (it->second > 25000) {
                    oled.append("wet(");
                    oled.append(std::to_string(it->second));
                    oled.append(")");
                }
                else if (it->second > 15000) {
                    oled.append("damp(");
                    oled.append(std::to_string(it->second));
                    oled.append(")");
                }
                else if (it->second > 8000) {
                    oled.append("dry(");
                    oled.append(std::to_string(it->second));
                    oled.append(")");
                }
                else {
                    oled.append("water now");
                }
            }
            oledSetCursor(i, 0);
            oledWrite(const_cast<char*>(oled.c_str()));
        }
        
        time_t ttime = time(0);
        tm *lt = localtime(&ttime);
        if (lt->tm_hour >= 20 && lt->tm_hour <= 6)
            relaySetChannel(7, 0, 0);
        else
            relaySetChannel(7, 0, 1);
        
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}
