#include "OrderBookV4.h"
#include <algorithm>

OrderBookV4::OrderBookV4() 
    : bids_(MAX_PRICE + 1), 
      asks_(MAX_PRICE + 1),
      order_pool_(MAX_ORDER_ID),
      order_map_(MAX_ORDER_ID, nullptr), // Pre-allocate vector
      best_bid_(0),
      best_ask_(MAX_PRICE) {}

void OrderBookV4::AddOrder(OrderId order_id, Side side, Price price, Quantity quantity) {
    if (side == Side::BUY) {
        while (quantity > 0 && price >= best_ask_ && best_ask_ < MAX_PRICE) {
            PriceLevel_V4& level = asks_[best_ask_];
            if (level.head == nullptr) { UpdateBestAsk(); continue; }

            HP_Order_V4* current_order = level.head;
            while (current_order && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, current_order->quantity);
                current_order->quantity -= trade_quantity;
                quantity -= trade_quantity;
                level.total_quantity -= trade_quantity;
                if (current_order->quantity == 0) {
                    HP_Order_V4* next_order = current_order->next;
                    order_map_[current_order->order_id] = nullptr;
                    RemoveFromList(current_order);
                    order_pool_.DeleteOrder(current_order);
                    current_order = next_order;
                }
            }
            if (level.head == nullptr) UpdateBestAsk();
        }

        if (quantity > 0) {
            HP_Order_V4* new_order = order_pool_.NewOrder();
            new_order->order_id = order_id;
            new_order->quantity = quantity;
            new_order->price = price;
            new_order->side = Side::BUY;
            AddToList(price, new_order);
            order_map_[order_id] = new_order;
            if (price > best_bid_) best_bid_ = price;
        }
    } else { // Side::SELL
        while (quantity > 0 && price <= best_bid_ && best_bid_ > 0) {
            PriceLevel_V4& level = bids_[best_bid_];
            if (level.head == nullptr) { UpdateBestBid(); continue; }

            HP_Order_V4* current_order = level.head;
            while (current_order && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, current_order->quantity);
                current_order->quantity -= trade_quantity;
                quantity -= trade_quantity;
                level.total_quantity -= trade_quantity;
                if (current_order->quantity == 0) {
                    HP_Order_V4* next_order = current_order->next;
                    order_map_[current_order->order_id] = nullptr;
                    RemoveFromList(current_order);
                    order_pool_.DeleteOrder(current_order);
                    current_order = next_order;
                }
            }
            if (level.head == nullptr) UpdateBestBid();
        }

        if (quantity > 0) {
            HP_Order_V4* new_order = order_pool_.NewOrder();
            new_order->order_id = order_id;
            new_order->quantity = quantity;
            new_order->price = price;
            new_order->side = Side::SELL;
            AddToList(price, new_order);
            order_map_[order_id] = new_order;
            if (price < best_ask_) best_ask_ = price;
        }
    }
}

void OrderBookV4::CancelOrder(OrderId order_id) {
    if (order_id >= order_map_.size()) return;
    HP_Order_V4* order = order_map_[order_id];
    if (order == nullptr) return;

    Price price = order->price;
    Side side = order->side;

    RemoveFromList(order);
    order_map_[order_id] = nullptr;
    order_pool_.DeleteOrder(order);

    if (side == Side::BUY && bids_[price].head == nullptr && price == best_bid_) {
        UpdateBestBid();
    } else if (side == Side::SELL && asks_[price].head == nullptr && price == best_ask_) {
        UpdateBestAsk();
    }
}

void OrderBookV4::AddToList(Price price, HP_Order_V4* order) {
    auto& level = (order->side == Side::BUY) ? bids_[price] : asks_[price];
    if (level.head == nullptr) {
        level.head = level.tail = order;
    } else {
        level.tail->next = order;
        order->prev = level.tail;
        level.tail = order;
    }
    level.total_quantity += order->quantity;
}

void OrderBookV4::RemoveFromList(HP_Order_V4* order) {
    auto& level = (order->side == Side::BUY) ? bids_[order->price] : asks_[order->price];
    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;
    if (level.head == order) level.head = order->next;
    if (level.tail == order) level.tail = order->prev;
    level.total_quantity -= order->quantity;
    order->next = order->prev = nullptr;
}

void OrderBookV4::UpdateBestBid() {
    while (best_bid_ > 0 && bids_[best_bid_].head == nullptr) {
        best_bid_--;
    }
}

void OrderBookV4::UpdateBestAsk() {
    while (best_ask_ < MAX_PRICE && asks_[best_ask_].head == nullptr) {
        best_ask_++;
    }
}
