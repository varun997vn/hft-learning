#!/bin/bash
set -e

echo "Compiling OrderBook tests..."
g++ -std=c++17 -O3 -Wall -Wextra \
    ../src/OrderBook.cpp \
    test_orderbook.cpp \
    -o test_orderbook

echo "Running OrderBook tests..."
./test_orderbook
