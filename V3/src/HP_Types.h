#pragma once

#include <cstdint>

// Use fixed-size integers for performance and predictability
using Price = uint32_t;
using Quantity = uint32_t;
using OrderId = uint64_t;

// Define compile-time constants for array sizes
constexpr Price MAX_PRICE = 25000;
constexpr OrderId MAX_ORDER_ID = 3000000;

enum class Side {
    BUY,
    SELL
};

// Intrusive linked list node
struct HP_Order {
    OrderId order_id;
    Quantity quantity;
    HP_Order* next = nullptr;
    HP_Order* prev = nullptr;
};

// Represents all orders at a single price
struct PriceLevel {
    Quantity total_quantity = 0;
    HP_Order* head = nullptr;
    HP_Order* tail = nullptr;
};
