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

void printHex(const vector<uint8_t>& data);
vector<uint8_t> buildModbusRTURequest(uint8_t slaveAddr, uint8_t functionCode, uint16_t startAddr, uint16_t numRegs);
vector<uint8_t> getDataByCommand(const string& commandType, uint16_t numRegs);
vector<uint8_t> getDataByStartAddr(uint16_t startAddr, uint16_t numRegs);
bool crc_check(uint8_t* message, size_t size);

int main() {
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        cerr << "Error opening " << device << endl;
        return 1;
    }
    cout << "Opened " << device << " successfully." << endl;

    
    if (tcgetattr(fd, &tty) != 0) {
        cerr << "Error getting tty attributes" << endl;
        close(fd);
        return 1;
    }
    
    cout << "Configuring tty attributes." << endl;
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
        cerr << "Error setting tty attributes" << endl;
        close(fd);
        return 1;
    }
    cout << "TTY attributes configured successfully." << endl;
    // while(true){
    //     vector<uint8_t> data = getDataByCommand("CURRENT_VOLTAGE",16);
    //     json j = {
    //         {"TIMESTAMP", chrono::system_clock::now().time_since_epoch().count()},
    //         {"LEVEL", "info"},
    //         {"CURRENT_VOLTAGE", (data[3] << 8 | data[4])/10.4f},
    //         {"CURRENT_CURRENT", (data[5] << 8 | data[6])/1000.4f},
    //         {"CURRENT_ACTIVE_POWER", (data[9] << 8 | data[10])/10.4f},
    //         {"CURRENT_APPARENT_POWER", (data[17] << 8 | data[18])/10.4f},
    //         {"CURRENT_TEMPERATURE", data[27] << 8 | data[28]},
    //         {"CURRENT_POWER_FACTOR", (data[33] << 8 | data[34])/100.4f}
    //     };
    //     cout << j << endl;
    // }
    // while(true){
    //     vector<vector<uint8_t>> alldata;
    //     for(uint32_t i=0;i<10;i++){
    //         uint32_t offset = i * 16;
    //         vector<uint8_t> data = getDataByStartAddr(offset,16);
    //         data.insert(data.begin(), offset&0xFF);
    //         data.insert(data.begin(), offset>>8);
    //         alldata.push_back(data);
    //     }
    //     for (const auto& data : alldata) {
    //         printHex(data);
    //     }
    //     cout << "----------------------------------------" << endl;
    // }
    // for(uint32_t i=0;i<0xFFF;i++){
    //     uint32_t offset = i * 16;
    //     vector<uint8_t> data = getDataByStartAddr(offset,16);
    //     data.insert(data.begin(), offset&0xFF);
    //     data.insert(data.begin(), offset>>8);
    //     printHex(data);
    // }

    for(uint32_t i=0;i<0xFFFF;i++){
        uint32_t offset = i;
        vector<uint8_t> data = getDataByStartAddr(0x0017,1);
        data.insert(data.begin(), offset&0xFF);
        data.insert(data.begin(), offset>>8);
        printHex(data);
    }



    close(fd);
    cout << "Closed " << device << " successfully." << endl;
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

void logError(const string& message) {
    json j = {
        {"TIMESTAMP", chrono::system_clock::now().time_since_epoch().count()},
        {"LEVEL", "error"},
        {"MESSAGE", message}
    };
    cout << j.dump() << endl;
}
vector<uint8_t> getDataByCommand(const string& commandType, uint16_t numRegs) {
    auto it = commandTypeToStartAddr.find(commandType);
    if (it == commandTypeToStartAddr.end()) {
        spdlog::error("Invalid command type: {}", commandType);
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
            spdlog::error("Error writing to serial port");
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
                message = "Timeout or error in select";
                break;
            }
            int bytesRead = read(fd, response.data() + totalBytesRead, expectedSize - totalBytesRead);
            if (bytesRead < 0) {
                message = "Error reading from serial port";
                break;
            }
            totalBytesRead += bytesRead;
            if (totalBytesRead > 3){
                if (response[1] != 0x03) {
                    message = "Invalid response from slave";
                    return response;
                }
            }
        }
        if (message != "") {
            // spdlog::error("Error: {}", message);
            continue;
        }
        if (!(crc_check(response.data(), response.size()))) {
            // logError(message);
            continue;
        }
        return response;
    }
    // logError("Failed to get data after multiple retries");
    // return {};
    return response; // Return the last response even if it failed
}

void printHex(const vector<uint8_t>& data) {
    for (const auto& byte : data) {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(byte) << " ";
    }
    cout << dec << endl;
}