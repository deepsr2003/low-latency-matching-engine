cmake_minimum_required(VERSION 3.16)
project(OrderBookV1 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(orderbook_v1
    src/main.cpp
    src/OrderBookV1.cpp
)

target_compile_features(orderbook_v1 PRIVATE cxx_std_17)
