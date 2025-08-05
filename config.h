#pragma once

#include <unordered_map>
#include <string>
#include <vector>
using std::string;
using std::unordered_map;
using std::vector;

const char* device = "/dev/ttyUSB0";
const uint8_t slaveAddr = 0x02;
const uint8_t retryTime = 3;

inline const unordered_map<string, uint16_t> commandTypeToStartAddr = {
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
    {"EXTERNAL_TEMPERATURE", 0x1A0E},
    {"INTERNAL_TEMPERATURE", 0x1B0E},
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

    {"VOLTAGE", 0x000E},
    {"CURRENT", 0x000F},
    {"0010", 0x0010}, // 0x0000 none
    {"ACTIVE_POWER", 0x0011},
    {"0012", 0x0012}, // 0x0000 none
    {"0013", 0x0013}, // 0x5c43 idk
    {"0014", 0x0014}, // 0x58d4 idk
    {"APPARENT_POWER", 0x0015},
    {"0016", 0x0016}, // 0x0000 none
    {"KILOWATT_HOUR", 0x0017},
    {"0018", 0x0018}, // 0x0000 none
    {"ELAPSED_TIME", 0x0019},
    {"TEMPERATURE", 0x001a},
    {"001b", 0x001b}, // 0xdb99 idk
    {"001c", 0x001c}, // 0x0000 none
    {"POWER_FACTOR", 0x001d},
    {"FREQUENCY", 0x001e},
    {"001f", 0x001f}, // 0xc3c0 idk
    {"WRITE_PARAMETER_PASSWORD", 0x000E}
};