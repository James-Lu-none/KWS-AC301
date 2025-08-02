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
using namespace std;

termios tty{};
const char* device = "/dev/ttyUSB0";
uint8_t slaveAddr = 0x02;
int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);

void printHex(const vector<uint8_t>& data);
vector<uint8_t> buildModbusRTURequest(uint8_t slaveAddr, uint8_t functionCode, uint16_t startAddr, uint16_t numRegs);
vector<uint8_t> getdata(const string& commandType);
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
    {"WRITE_PARAMETER_PASSWORD", 0x000E}
};

int main() {
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
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 5;
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
    vector<uint8_t> data;
    // iterate through command types and get data
    for (const auto& command : commandTypeToStartAddr) {
        cout << "Command: " << command.first << endl;
        do{
            data = getdata(command.first);
            if (!data.empty()) {
                cout << "Responded" << ": ";
                printHex(data);
            }
        }while(crc_check(data.data(), data.size()) == false);
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

vector<uint8_t> getdata(const string& commandType) {
    auto it = commandTypeToStartAddr.find(commandType);
    if (it == commandTypeToStartAddr.end()) {
        cerr << "!! Invalid command type: " << commandType << endl;
        return {};
    }
    uint16_t startAddr = it->second;
    vector<uint8_t> request = buildModbusRTURequest(slaveAddr, 0x03, startAddr, 1);

    tcflush(fd, TCIOFLUSH);
    ssize_t bytesWritten = write(fd, request.data(), request.size());
    if (bytesWritten != static_cast<ssize_t>(request.size())) {
        cerr << "!! Error writing to serial port" << endl;
        return {};
    }

    vector<uint8_t> response(256);
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int selectResult = select(fd + 1, &readfds, nullptr, nullptr, &timeout);
    if (selectResult > 0 && FD_ISSET(fd, &readfds)) {
        int bytesRead = read(fd, response.data(), response.size());
        if (bytesRead < 0) {
            cerr << "!! Error reading from serial port" << endl;
            return {};
        }
        response.resize(bytesRead);
        return response;
    } else {
        cerr << "!! Timeout" << endl;
        return {};
    }
}

void printHex(const vector<uint8_t>& data) {
    for (const auto& byte : data) {
        cout << hex << setw(2) << setfill('0') << static_cast<int>(byte) << " ";
    }
    cout << dec << endl;
}