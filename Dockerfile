FROM ubuntu:22.04

RUN apt update && apt install -y \
    cmake g++ \
    libasio-dev \
    libboost-all-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libjsoncpp-dev \
    pkg-config \
    git

WORKDIR /app
COPY . .

RUN mkdir build && cd build && cmake .. && make && cd ..