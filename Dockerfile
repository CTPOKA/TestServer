FROM ubuntu:22.04

RUN apt update && apt install -y cmake g++ libboost-all-dev git libasio-dev

WORKDIR /app
COPY . .

RUN mkdir build && cd build && cmake .. && make && cd ..