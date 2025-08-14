cmake_minimum_required(VERSION 3.16)
project(OrderBookV4 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(orderbook_v4
    src/main_fast.cpp
    src/OrderBookV4.cpp
)

target_compile_features(orderbook_v4 PRIVATE cxx_std_17)

# Apply aggressive optimizations
set_target_properties(orderbook_v4 PROPERTIES
    COMPILE_FLAGS "-O3 -march=native -flto -DNDEBUG"
    LINK_FLAGS "-flto"
)
