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
#include "dht_read.h"

MQTTClient *g_client;

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
    if (type == MQTTClient::CallbackType::DISCONNECT) {
        std::cout << "MQTT disconnected, code: " << errno << std::endl;
    }
}

void incomingMessage(int mid, std::string topic, uint32_t *payload, int size)
{
}

void mqttError(std::string msg, int err)
{
    std::cout << "MQTT error " << msg << ":" << err << std::endl;
}

void setupMQTT(std::string host, int port)
{
    std::string name;
//    get_name(name);
    name = "testclient2";
    
    g_client = new MQTTClient(name, host, port);
    g_client->setGenericCallback(genericCallback);
    g_client->setMessageCallback(incomingMessage);
    g_client->setErrorCallback(mqttError);
}

void temperature(double &c, double &f, double &h)
{
    nlohmann::json doc;
    float humidity = 0.0f;
    float temperature = 0.0f;
    int result = 0;
    int maxRetry = 3;
  
    do {
        result = dht_read(DHT22, 19, &humidity, &temperature);
        maxRetry--;
    } while(result != 0 && maxRetry > 0);
    
    if (result == 0 && maxRetry > 0) {
        doc["environment"]["humidity"] = humidity;
        doc["environment"]["celsius"] = temperature;
        if (g_client->isConnected())
            g_client->publish(NULL, "planter/environment", doc.dump().size(), doc.dump().c_str(), 0, false);
        else
            std::cout << "not connected" << std::endl;
        std::cout << "Payload size: " << doc.dump().size() << ": " << doc.dump() << std::endl;
    }
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
