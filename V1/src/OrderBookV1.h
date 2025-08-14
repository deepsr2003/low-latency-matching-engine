#pragma once

#include "Types.h"
#include <map>
#include <list>
#include <unordered_map>
#include <functional>

class OrderBookV1 {
public:
    void AddOrder(OrderId order_id, Side side, Price price, Quantity quantity);
    void CancelOrder(OrderId order_id);

private:
    using OrderList = std::list<Order>;

    struct OrderLocation {
        Price price;
        Side side;
        OrderList::iterator it;
    };

    std::map<Price, OrderList, std::greater<Price>> bids_;
    std::map<Price, OrderList> asks_;
    std::unordered_map<OrderId, OrderLocation> order_map_;
};
