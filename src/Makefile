CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

TARGET := power_meter_reader
SRC := main.cpp

# Use system-installed headers
LIBS := -lspdlog -lfmt

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)
