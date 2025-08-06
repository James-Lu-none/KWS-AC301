#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <vector>
#include <iomanip>
#include <unordered_map>
#include <string>
#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "config.h"
using json = nlohmann::json;
using namespace std;

termios tty{};
int fd;
uint8_t restartCounter;

void jsonLog(const string& message, const string& level, const vector<uint8_t>& data = {});
void printHex(const vector<uint8_t>& data);
vector<uint8_t> buildModbusRTURequest(uint8_t slaveAddr, uint8_t functionCode, uint16_t startAddr, uint16_t numRegs);
vector<uint8_t> getDataByCommand(const string& commandType, uint16_t numRegs);
vector<uint8_t> getDataByStartAddr(uint16_t startAddr, uint16_t numRegs);
bool crc_check(uint8_t* message, size_t size);

int main() {
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        jsonLog("ERROR opening device", "FATAL");
        return 1;
    }
    jsonLog("Opened device successfully.", "DEBUG");

    
    if (tcgetattr(fd, &tty) != 0) {
        jsonLog("ERROR getting tty attributes", "FATAL");
        close(fd);
        return 1;
    }

    jsonLog("Configuring tty attributes.", "DEBUG");
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 5;
    tty.c_cc[VTIME] = 1;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        jsonLog("ERROR setting tty attributes", "fatal");
        close(fd);
        return 1;
    }
    jsonLog("TTY attributes configured successfully.", "DEBUG");
    while(true){
        restartCounter = 0;
        auto start = chrono::steady_clock::now();       
        vector<uint8_t> data = getDataByStartAddr(0x0010,16);
        vector<uint8_t> data1 = getDataByCommand("VOLTAGE",2);
        auto end = chrono::steady_clock::now();

        if (restartCounter >= restartThreshold) {
            jsonLog("Restart counter exceeded", "FATAL");
            break;
        }
        if (data.empty() || data1.empty()) {
            continue;
        }
        float sampleRate = 1000.0 / chrono::duration_cast<chrono::milliseconds>(end - start).count();
        float activePower = (data[5] << 8 | data[6]) / 10.0f;
        float apparentPower = (data[13] << 8 | data[14]) / 10.0f;
        float kilowattHours = (data[17] << 8 | data[18]) / 1000.0f;
        float reactivePower = sqrt(pow(apparentPower, 2) - pow(activePower, 2));
        float elapsedTime = (data[21] << 8 | data[22]) / 60.0f;
        float temperature = (data[23] << 8 | data[24]) / 1.0f;
        float powerFactor = (data[29] << 8 | data[30]) / 100.0f;
        float frequency = (data[31] << 8 | data[32]) / 10.0f;
        float voltage = (data1[3] << 8 | data1[4]) / 10.0f;
        float current = (data1[5] << 8 | data1[6]) / 1000.0f;
        // printHex(data);
        json j = {
            {"TIMESTAMP", chrono::system_clock::now().time_since_epoch().count()},
            {"LEVEL", "INFO"},
            {"ACTIVE_POWER", activePower},
            {"APPARENT_POWER", apparentPower},
            {"KILOWATT_HOURS", kilowattHours},
            {"REACTIVE_POWER", reactivePower},
            {"ELAPSED_TIME", elapsedTime},
            {"TEMPERATURE", temperature},
            {"POWER_FACTOR", powerFactor},
            {"FREQUENCY", frequency},
            {"VOLTAGE", voltage},
            {"CURRENT", current},
            {"SAMPLE_RATE", sampleRate}
        };
        cout << j << endl;
    }

    close(fd);
    if (fd < 0) {
        jsonLog("ERROR closing device", "FATAL");
        return 1;
    }
    jsonLog("Closed device successfully.", "DEBUG");
    return 0;
}

bool crc_check(uint8_t* message, size_t size) {
    if (size < 2) return false; // CRC requires at least 2 bytes
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < size - 2; pos++) {
        crc ^= message[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return (crc == (message[size - 2] | (message[size - 1] << 8)));
}

vector<uint8_t> buildModbusRTURequest(uint8_t slaveAddr, uint8_t functionCode, uint16_t startAddr, uint16_t numRegs) {
    vector<uint8_t> req;
    req.push_back(slaveAddr);
    req.push_back(functionCode);
    req.push_back((startAddr >> 8) & 0xFF);
    req.push_back(startAddr & 0xFF);
    req.push_back((numRegs >> 8) & 0xFF);
    req.push_back(numRegs & 0xFF);

    // CRC16 calculation
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < req.size(); ++i) {
        crc ^= req[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc = crc >> 1;
        }
    }
    req.push_back(crc & 0xFF);         // CRC Low byte
    req.push_back((crc >> 8) & 0xFF);  // CRC High byte
    return req;
}

void jsonLog(const string& message, const string& level, const vector<uint8_t>& data) {
    json j = {
        {"TIMESTAMP", chrono::system_clock::now().time_since_epoch().count()},
        {"LEVEL", level},
        {"MESSAGE", message},
        {"DATA", data},
    };
    cout << j.dump() << endl;
    return;
}
vector<uint8_t> getDataByCommand(const string& commandType, uint16_t numRegs) {
    auto it = commandTypeToStartAddr.find(commandType);
    if (it == commandTypeToStartAddr.end()) {
        jsonLog("Invalid command type", "WARN", {});
        return {};
    }
    uint16_t startAddr = it->second;
    return getDataByStartAddr(startAddr, numRegs);
}

vector<uint8_t> getDataByStartAddr(uint16_t startAddr, uint16_t numRegs) {
    vector<uint8_t> request = buildModbusRTURequest(slaveAddr, 0x03, startAddr, numRegs);
    uint16_t expectedSize = 5 + numRegs * 2; // 5 bytes for header + 2 bytes per register
    vector<uint8_t> response(expectedSize);
    for (uint16_t i=0; i < retryTime; i++) {
        fill(response.begin(), response.end(), 0);
        tcflush(fd, TCIFLUSH);
        ssize_t bytesWritten = write(fd, request.data(), request.size());
        if (bytesWritten != static_cast<ssize_t>(request.size())) {
            jsonLog("ERROR writing to serial port", "ERROR", response);
            continue;
        }
        
        int totalBytesRead = 0;
        string message = "";
        while (totalBytesRead < expectedSize) {
            tcdrain(fd);
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000;
            int selectResult = select(fd + 1, &readfds, nullptr, nullptr, &timeout);
            if (!(selectResult > 0 && FD_ISSET(fd, &readfds))) {
                jsonLog("Timeout or ERROR in select", "WARN", response);
                break;
            }
            int bytesRead = read(fd, response.data() + totalBytesRead, expectedSize - totalBytesRead);
            if (bytesRead < 0) {
                jsonLog("ERROR reading from serial port", "ERROR", response);
                break;
            }
            totalBytesRead += bytesRead;
            if (totalBytesRead > 3){
                if (response[1] != 0x03) {
                    jsonLog("Unexpected response from slave", "WARN", response);
                    break;
                }
            }
        }
        if (!(crc_check(response.data(), response.size()))) {
            jsonLog("Modbus RTU CRC check failed", "WARN", response);
        }
        return response;
    }
    restartCounter++;
    jsonLog("Failed to get data after multiple retries", "ERROR", response);
    return {};
}

void printHex(const vector<uint8_t>& data) {
    for (const auto& byte : data) {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(byte) << " ";
    }
    cout << dec << endl;
}