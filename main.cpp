#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <oled-exp.h>
#include <relay-exp.h>
#include <nlohmann/json.hpp>

#include "mqttclient.h"

MQTTClient *g_client;

bool CRC8(uint8_t MSB, uint8_t LSB, uint8_t CRC)
{
    /*
    *	Name  : CRC-8
    *	Poly  : 0x31	x^8 + x^5 + x^4 + 1
    *	Init  : 0xFF
    *	Revert: false
    *	XorOut: 0x00
    *	Check : for 0xBE,0xEF CRC is 0x92
    */
    uint8_t crc = 0xFF;
    uint8_t i;
    crc ^=MSB;

    for (i = 0; i < 8; i++)
        crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;

    crc ^= LSB;
    for (i = 0; i < 8; i++)
        crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;

    if (crc != CRC) {
        std::cout << "BAD CRC: expected " << static_cast<uint32_t>(CRC) << " but calculated " << static_cast<uint32_t>(crc) << std::endl;
    }
    return (crc == CRC);
}

/**
 * \func void get_name(std::string &name)
 * \param name C++ reference to std::string
 * This function attempts to get the kernel hostname for this device and assigns it to name.
 */
void get_name(std::string &name)
{
    std::ifstream ifs;
    int pos;

    ifs.open("/proc/sys/kernel/hostname");
    if (!ifs) {
        std::cerr << __PRETTY_FUNCTION__ << ": Unable to open /proc/sys/kernel/hostname for reading" << std::endl;
        name = "omega";
    }
    name.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    try {
        name.erase(name.find('\n'));
    }
    catch (std::out_of_range &e) {
        std::cerr << __PRETTY_FUNCTION__ << ": " << e.what() << std::endl;
        return;
    }
    std::cout << __PRETTY_FUNCTION__ << ": Assigning " << name << " as device name" << std::endl;
}

void genericCallback(MQTTClient::CallbackType type, int mid)
{
    if (type == MQTTClient::CallbackType::CONNECT) {
        std::cout << "MQTT Connected" << std::endl;
    }
}

void incomingMessage(int mid, std::string topic, uint32_t *payload, int size)
{
}

void mqttError(std::string msg, int err)
{
}

void setupMQTT(std::string host, int port)
{
    std::string name;
    get_name(name);

    g_client = new MQTTClient(name, host, port);
    g_client->setGenericCallback(genericCallback);
    g_client->setMessageCallback(incomingMessage);
    g_client->setErrorCallback(mqttError);
}

void temperature(double &c, double &f, double &h)
{
    int fd;
    int bytes;
    static int lastHumidity = 0.0;
    double humidity;
    nlohmann::json doc;

    if ((fd = open("/dev/i2c-0", O_RDWR)) < 0) {
        std::cerr << "Unable to open i2c-0: " << errno << std::endl;
        return;
    }
    
    if (ioctl(fd, I2C_SLAVE, 0x45) < 0) {
        std::cerr << "Unable to aquire i2c bus: " << errno << std::endl;
        close(fd);
        return;
    }
    // Send high repeatability measurement command
    // Command msb, command lsb(0x2c, 0x06)
    uint8_t config[] = { 0x24, 0x00 };

    write(fd, config, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Read 6 bytes of data
    // temp msb, temp lsb, temp CRC, humidity msb, humidity lsb, humidity CRC
    uint8_t data[6] = {0};
    if ((bytes = read(fd, data, 6)) < 0) {
        std::cerr << "Unable to read data from i2c device: " << errno << std::endl;
        close(fd);
        return;
    }

    std::cout << "RAW: ";
    for (int i = 0; i < bytes; i++) {
        std::cout << "0x" << static_cast<uint32_t>(data[i]) << " ";
    }
    std::cout << std::endl;
    
    // Convert the data
    if (!CRC8(data[0], data[1], data[2])) {
        std::cout << "Bad temp data" << std::endl;
        close(fd);
        return;
    }
    else {
        doc["environment"]["celsius"] = c;
        doc["environment"]["farenheit"] = f;
    }
    std::cout << "Temp CRC passed" << std::endl;

    if (!CRC8(data[3], data[4], data[5])) {
        std::cout << "Bad humidity data" << std::endl;
        close(fd);
        return;
    }
    std::cout << "Temp CRC passed" << std::endl;
  
    c = (((data[0] * 256) + data[1]) / 65535.0) * 175.0 - 45.0;
    f = (((data[0] * 256) + data[1]) / 65535.0) * 315.0 - 49.0;
    humidity = (((data[3] * 256) + data[4]) / 65535.0) * 100;
    std::cout << "Got " << humidity << " from sensor" << std::endl;
    // Output data to screen
        
    // I get 0 sometimes, so just skip obvious wrong readings
    if (humidity > 0)
        lastHumidity = humidity;

    h = lastHumidity;
    doc["environment"]["humidity"] = h;
    g_client->publish(NULL, "planter/environment", doc.dump().size(), doc.dump().c_str(), 0, false);
    std::cout << "Payload size: " << doc.dump().size() << ": " << doc.dump() << std::endl;

    close(fd);
}

int main(int argc, char *argv[])
{
    bool relayState = false;
    int relay = 0;
    double c = 0.0;
    double f = 0.0;
    double h = 0.0;

    relayDriverInit(7);
    relayCheckInit (7, &relay);
    
    if (relay == 0) {
        std::cout << "Unable to initialize relay board" << std::endl;
        exit(-1);
    }

    setupMQTT("172.24.1.13", 1883);
 
    while (1) {
        time_t ttime = time(0);
        tm *lt = localtime(&ttime);
        if (lt->tm_hour >= 20 && lt->tm_hour <= 6) {
            if (relayState) {
                relaySetChannel(7, 1, 0);
                relayState = false;
            }
        }
        else {
            if (!relayState) {
                relaySetChannel(7, 1, 1);
                relayState = true;
            }
        }

        temperature(c, f, h);
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}
