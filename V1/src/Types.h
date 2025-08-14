#pragma once

#include <cstdint>

using Price = uint64_t;
using Quantity = uint64_t;
using OrderId = uint64_t;

enum class Side {
    BUY,
    SELL
};

struct Order {
    OrderId order_id;
    Price price;
    Quantity quantity;
    Side side;
};
