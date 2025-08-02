FROM gcc:latest

WORKDIR /app

COPY main.cpp .

RUN g++ -o power_meter_reader main.cpp
CMD ["./power_meter_reader"]