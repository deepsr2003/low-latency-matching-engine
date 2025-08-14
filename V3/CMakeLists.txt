cmake_minimum_required(VERSION 3.16)
project(OrderBookV3 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(orderbook_v3
    src/main.cpp
    src/OrderBookV3.cpp
)

target_compile_features(orderbook_v3 PRIVATE cxx_std_17)
