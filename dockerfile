FROM gcc:latest

RUN apt-get update && apt-get install -y \
    libspdlog-dev \
    nlohmann-json3-dev \
    make \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY main.cpp .
COPY Makefile .

RUN make

CMD ["./power_meter_reader"]