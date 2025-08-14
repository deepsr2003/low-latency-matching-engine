#pragma once

#include "HP_Types.h"
#include "ObjectPool.h"
#include <vector>
#include <unordered_map>

class OrderBookV3 {
public:
    OrderBookV3();
    void AddOrder(OrderId order_id, Side side, Price price, Quantity quantity);
    void CancelOrder(OrderId order_id);

private:
    void AddToList(Price price, HP_Order* order, Side side);
    void RemoveFromList(Price price, HP_Order* order, Side side);
    void UpdateBestBid();
    void UpdateBestAsk();

    std::vector<PriceLevel> bids_;
    std::vector<PriceLevel> asks_;
    
    ObjectPool<HP_Order> order_pool_;
    std::unordered_map<OrderId, HP_Order*> order_map_;

    Price best_bid_;
    Price best_ask_;
};
