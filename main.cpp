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
using json = nlohmann::json;
using namespace std;

termios tty{};
const char* device = "/dev/ttyUSB0";
uint8_t slaveAddr = 0x02;
uint8_t retryTime = 100;
int fd;

void printHex(const vector<uint8_t>& data);
vector<uint8_t> buildModbusRTURequest(uint8_t slaveAddr, uint8_t functionCode, uint16_t startAddr, uint16_t numRegs);
vector<uint8_t> getdata(const string& commandType, uint16_t numRegs);
bool crc_check(uint8_t* message, size_t size);

unordered_map<string, uint16_t> commandTypeToStartAddr = {
    {"RATED_VOLTAGE", 0x010E},
    {"RATED_CURRENT", 0x020E},
    {"ACTIVE_POWER_UNDER_0_5L", 0x030E},
    {"REACTIVE_POWER_UNDER_0_5L", 0x040E},
    {"HARDWARE_VERSION_LOW", 0x050E},
    {"HARDWARE_VERSION_HIGH", 0x060E},
    {"SOFTWARE_VERSION_LOW", 0x070E},
    {"SOFTWARE_VERSION_HIGH", 0x080E},
    {"PROTOCOL_VERSION_LOW", 0x090E},
    {"PROTOCOL_VERSION_HIGH", 0x0A0E},
    {"MODULE_TYPE", 0x0B0E},
    {"COMM_ADDR_BAUD_STATUS", 0x0C0E},
    {"PULSE_CONSTANT", 0x0D0E},
    {"CHANNEL_VOLTAGE", 0x0E0E},
    {"CHANNEL_CURRENT_LOW", 0x0F0E},
    {"CHANNEL_CURRENT_HIGH", 0x100E},
    {"CHANNEL_ACTIVE_POWER_LOW_WATTS", 0x110E},
    {"CHANNEL_ACTIVE_POWER_HIGH", 0x120E},
    {"CHANNEL_REACTIVE_POWER_LOW", 0x130E},
    {"CHANNEL_REACTIVE_POWER_HIGH", 0x140E},
    {"CHANNEL_APPARENT_POWER_LOW", 0x150E},
    {"CHANNEL_APPARENT_POWER_HIGH", 0x160E},
    {"CHANNEL_ENERGY_LOW", 0x170E},
    {"CHANNEL_ENERGY_HIGH", 0x180E},
    {"OPERATING_MINUTES", 0x190E},
    {"CURRENT_EXTERNAL_TEMPERATURE", 0x1A0E},
    {"CURRENT_INTERNAL_TEMPERATURE", 0x1B0E},
    {"RTC_BATTERY_VOLTAGE", 0x1C0E},
    {"POWER_FACTOR", 0x1D0E},
    {"VOLTAGE_FREQUENCY", 0x1E0E},
    {"ALARM_STATUS_BYTE", 0x1F0E},
    {"RTC_CLOCK_YEAR", 0x280E},
    {"RTC_CLOCK_MONTH", 0x290E},
    {"RTC_CLOCK_DAY", 0x2A0E},
    {"RTC_CLOCK_HOUR", 0x2B0E},
    {"RTC_CLOCK_MINUTE", 0x2C0E},
    {"RTC_CLOCK_SECOND", 0x2D0E},

    {"CURRENT_VOLTAGE", 0x000E},
    {"CURRENT_CURRENT", 0x000F},
    {"0010", 0x0010}, // 0x0000 none
    {"CURRENT_ACTIVE_POWER", 0x0011},
    {"0012", 0x0012}, // 0x0000 none
    {"0013", 0x0013}, // 0x5c43 idk
    {"0014", 0x0014}, // 0x58d4 idk
    {"CURRENT_APPARENT_POWER", 0x0015},
    {"0016", 0x0016}, // 0x0000 none
    {"0017", 0x0017}, // maybe time
    {"0018", 0x0018}, // 0x0000 none
    {"0019", 0x0019}, // maybe time
    {"CURRENT_TEMPERATURE", 0x001a},
    {"001b", 0x001b}, // 0xdb99 idk
    {"001c", 0x001c}, // 0x0000 none
    {"CURRENT_POWER_FACTOR", 0x001d},
    {"001e", 0x001e}, // current (duplicate)
    {"001f", 0x001f}, // 0xc3c0 idk
    {"WRITE_PARAMETER_PASSWORD", 0x000E}
};

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
    while(true){
        cout << "Reading data..." << endl;
        printHex(getdata("CURRENT_VOLTAGE",16));
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

vector<uint8_t> getdata(const string& commandType, uint16_t numRegs) {
    auto it = commandTypeToStartAddr.find(commandType);
    if (it == commandTypeToStartAddr.end()) {
        cerr << "!! Invalid command type: " << commandType << endl;
        return {};
    }
    uint16_t startAddr = it->second;

    vector<uint8_t> request = buildModbusRTURequest(slaveAddr, 0x03, startAddr, numRegs);
    uint16_t expectedSize = 5 + numRegs * 2; // 5 bytes for header + 2 bytes per register
    for (uint16_t i=0; i < retryTime; i++) {
        vector<uint8_t> response(expectedSize);
        tcflush(fd, TCIFLUSH);
        ssize_t bytesWritten = write(fd, request.data(), request.size());
        if (bytesWritten != static_cast<ssize_t>(request.size())) {
            cerr << "!! Error writing to serial port" << endl;
            continue;
        }
        
        int totalBytesRead = 0;
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
                cerr << "!! Timeout" << endl;
                break;
            }
            int bytesRead = read(fd, response.data() + totalBytesRead, expectedSize - totalBytesRead);
            if (bytesRead < 0) {
                cerr << "!! Error reading from serial port" << endl;
                break;
            }
            totalBytesRead += bytesRead;
        }
        if (!(crc_check(response.data(), response.size()))) {
            cerr << "!! crc check failed" << endl;
            continue;
        }
        return response;
    }
    return {};
}

void printHex(const vector<uint8_t>& data) {
    for (const auto& byte : data) {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(byte) << " ";
    }
    cout << dec << endl;
}