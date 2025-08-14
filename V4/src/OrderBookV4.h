#pragma once

#include "HP_Types.h"
#include "ObjectPool.h"
#include <vector>

// To avoid storing side/price in the order map, we store it in the HP_Order struct
struct HP_Order_V4 {
    OrderId order_id;
    Quantity quantity;
    Price price;
    Side side;
    HP_Order_V4* next = nullptr;
    HP_Order_V4* prev = nullptr;
};

struct PriceLevel_V4 {
    Quantity total_quantity = 0;
    HP_Order_V4* head = nullptr;
    HP_Order_V4* tail = nullptr;
};

class OrderBookV4 {
public:
    OrderBookV4();
    void AddOrder(OrderId order_id, Side side, Price price, Quantity quantity);
    void CancelOrder(OrderId order_id);

private:
    void AddToList(Price price, HP_Order_V4* order);
    void RemoveFromList(HP_Order_V4* order);
    void UpdateBestBid();
    void UpdateBestAsk();

    std::vector<PriceLevel_V4> bids_;
    std::vector<PriceLevel_V4> asks_;
    
    ObjectPool<HP_Order_V4> order_pool_;
    std::vector<HP_Order_V4*> order_map_; // Swapped unordered_map for vector

    Price best_bid_;
    Price best_ask_;
};
