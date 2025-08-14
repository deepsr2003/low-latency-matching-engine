cmake_minimum_required(VERSION 3.16)
project(OrderBookV6 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Note: V6 uses the same fast main driver as V4.
# We copy V4/src/main_fast.cpp and adapt it to call OrderBookV6
add_executable(orderbook_v6
    src/main_fast_v6.cpp # This is a renamed copy of V4's main
    src/OrderBookV6.cpp
)

target_compile_features(orderbook_v6 PRIVATE cxx_std_17)

# Apply aggressive optimizations
set_target_properties(orderbook_v6 PROPERTIES
    COMPILE_FLAGS "-O3 -march=native -flto -DNDEBUG"
    LINK_FLAGS "-flto"
)
