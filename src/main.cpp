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
#include <cstdlib>
#include <curl/curl.h>
#include "config.h"
using json = nlohmann::json;
using namespace std;

termios tty{};
int fd;
uint8_t restartCounter;

void init();
void jsonLog(const string& message, const string& level, const vector<uint8_t>& data = {});
void printHex(const vector<uint8_t>& data);
bool crc_check(uint8_t* message, size_t size);
void sendToInfluxDB(const json& j);
vector<uint8_t> buildModbusRTURequest(uint8_t slaveAddr, uint8_t functionCode, uint16_t startAddr, uint16_t numRegs);
vector<uint8_t> getDataByCommand(const string& commandType, uint16_t numRegs);
vector<uint8_t> getDataByStartAddr(uint16_t startAddr, uint16_t numRegs);
SensorData parseSensorData(const vector<uint8_t>& raw);

int main() {
    init();
    while(true){
        restartCounter = 0;
        auto start = chrono::steady_clock::now();
        vector<uint8_t> data = getDataByStartAddr(0x000E, 17);
        auto end = chrono::steady_clock::now();
        if (restartCounter >= restartThreshold) {
            jsonLog("Restart counter exceeded", "FATAL");
            close(fd);
            abort();
        }
        if (data.empty()) {
            continue;
        }
        float sampleRate = 1000.0 / chrono::duration_cast<chrono::milliseconds>(end - start).count();
        SensorData sensorData = parseSensorData(data);

        json j = {
            {"TIMESTAMP", chrono::system_clock::now().time_since_epoch().count()},
            {"LEVEL", "INFO"},
            {"ACTIVE_POWER", sensorData.activePower},
            {"APPARENT_POWER", sensorData.apparentPower},
            {"KILOWATT_HOURS", sensorData.kilowattHours},
            {"REACTIVE_POWER", sensorData.reactivePower},
            {"ELAPSED_TIME", sensorData.elapsedTime},
            {"TEMPERATURE", sensorData.temperature},
            {"POWER_FACTOR", sensorData.powerFactor},
            {"FREQUENCY", sensorData.frequency},
            {"VOLTAGE", sensorData.voltage},
            {"CURRENT", sensorData.current},
            {"SAMPLE_RATE", sampleRate}
        };
        sendToInfluxDB(j);
    }
    
    return 1;
}

void sendToInfluxDB(const json& j) {
    string influxDBAuthToken = getInfluxDBAuthToken();
    string influxDBUrl = getInfluxDBUrl();
    if (influxDBUrl.empty() || influxDBAuthToken.empty()) {
        jsonLog("InfluxDB URL or Auth Token not set", "ERROR");
        return;
    }
    string measurement = "power_meter";
    string tags = "level=" + j["LEVEL"].get<string>();
    string fields;
    bool firstField = true;
    for (auto& [key, value] : j.items()) {
        if (key == "TIMESTAMP" || key == "LEVEL") continue; // skip tag/timestamp keys
        if (!firstField) fields += ",";
        if (value.is_number_float() || value.is_number_integer()) {
            fields += key + "=" + to_string(value.get<double>());
        }
        firstField = false;
    }

    long long timestamp_ns = j["TIMESTAMP"].get<long long>();

    string lineProtocol = measurement + "," + tags + " " + fields + " " + to_string(timestamp_ns);

    cout << "Line protocol: " << lineProtocol << "\n";

    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, influxDBUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, lineProtocol.c_str());
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        headers = curl_slist_append(headers, ("Authorization: Token " + influxDBAuthToken).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "InfluxDB write failed: " << curl_easy_strerror(res) << "\n";
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return;
}


SensorData parseSensorData(const vector<uint8_t>& raw) {
    vector<uint16_t> data16(17);
    memcpy(data16.data(), raw.data() + 3, sizeof(uint16_t) * 17);
    for_each(data16.begin(), data16.end(), [](uint16_t& val){ val = (val >> 8) | (val << 8); });

    SensorData s;
    s.voltage = data16[0] / 10.0f;
    s.current = data16[1] / 1000.0f;
    s.activePower = data16[3] / 10.0f;
    s.apparentPower = data16[7] / 10.0f;
    s.kilowattHours = data16[9] / 1000.0f;
    s.elapsedTime = data16[11] / 60.0f;
    s.temperature = data16[12];
    s.powerFactor = data16[15] / 100.0f;
    s.frequency = data16[16] / 10.0f;
    s.reactivePower = sqrt(s.apparentPower * s.apparentPower - s.activePower * s.activePower);
    return s;
}


uint16_t crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    uint8_t index;

    while (length--)
    {
        index = *data++ ^ crc;
        crc >>= 8;
        crc  ^= crcTable[index];
    }
    return crc;
}

bool crc_check(uint8_t* message, size_t size) {
    if (size < 5) return false;
    uint16_t crc = crc16(message, size - 2);
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

    uint16_t crc = crc16(req.data(), req.size());
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
    sendToInfluxDB(j);
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

void init(){
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        jsonLog("ERROR opening device", "FATAL");
        abort();
    }
    jsonLog("Opened device successfully.", "DEBUG");

    
    if (tcgetattr(fd, &tty) != 0) {
        jsonLog("ERROR getting tty attributes", "FATAL");
        close(fd);
        abort();
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
        abort();
    }
    jsonLog("TTY attributes configured successfully.", "DEBUG");
}