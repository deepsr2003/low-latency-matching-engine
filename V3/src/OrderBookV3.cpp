#include "OrderBookV3.h"
#include <algorithm> // For std::min

OrderBookV3::OrderBookV3() 
    : bids_(MAX_PRICE + 1), 
      asks_(MAX_PRICE + 1),
      order_pool_(MAX_ORDER_ID),
      best_bid_(0),
      best_ask_(MAX_PRICE) {}

void OrderBookV3::AddOrder(OrderId order_id, Side side, Price price, Quantity quantity) {
    if (side == Side::BUY) {
        while (quantity > 0 && price >= best_ask_) {
            PriceLevel& level = asks_[best_ask_];
            if (level.head == nullptr) { // Should not happen if best_ask is correct
                UpdateBestAsk();
                continue;
            }

            HP_Order* current_order = level.head;
            while (current_order && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, current_order->quantity);
                
                current_order->quantity -= trade_quantity;
                quantity -= trade_quantity;
                level.total_quantity -= trade_quantity;

                if (current_order->quantity == 0) {
                    HP_Order* next_order = current_order->next;
                    order_map_.erase(current_order->order_id);
                    RemoveFromList(best_ask_, current_order, Side::SELL);
                    order_pool_.DeleteOrder(current_order);
                    current_order = next_order;
                }
            }
            if (level.head == nullptr) {
                UpdateBestAsk();
            }
        }

        if (quantity > 0) {
            HP_Order* new_order = order_pool_.NewOrder();
            new_order->order_id = order_id;
            new_order->quantity = quantity;
            AddToList(price, new_order, Side::BUY);
            order_map_[order_id] = new_order;
            if (price > best_bid_) {
                best_bid_ = price;
            }
        }
    } else { // Side::SELL
        while (quantity > 0 && price <= best_bid_) {
            PriceLevel& level = bids_[best_bid_];
             if (level.head == nullptr) {
                UpdateBestBid();
                continue;
            }

            HP_Order* current_order = level.head;
            while (current_order && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, current_order->quantity);
                
                current_order->quantity -= trade_quantity;
                quantity -= trade_quantity;
                level.total_quantity -= trade_quantity;

                if (current_order->quantity == 0) {
                    HP_Order* next_order = current_order->next;
                    order_map_.erase(current_order->order_id);
                    RemoveFromList(best_bid_, current_order, Side::BUY);
                    order_pool_.DeleteOrder(current_order);
                    current_order = next_order;
                }
            }
            if (level.head == nullptr) {
                UpdateBestBid();
            }
        }

        if (quantity > 0) {
            HP_Order* new_order = order_pool_.NewOrder();
            new_order->order_id = order_id;
            new_order->quantity = quantity;
            AddToList(price, new_order, Side::SELL);
            order_map_[order_id] = new_order;
            if (price < best_ask_) {
                best_ask_ = price;
            }
        }
    }
}

void OrderBookV3::CancelOrder(OrderId order_id) {
    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) return;

    HP_Order* order = it->second;
    // We need to find the price and side to cancel correctly. A bit of a hack in V3.
    // A better implementation would store price/side in the order_map value.
    for (Price p = best_bid_; p > 0; --p) {
        if(bids_[p].head) {
             HP_Order* current = bids_[p].head;
             while(current) {
                if(current->order_id == order_id) {
                    RemoveFromList(p, order, Side::BUY);
                    if (bids_[p].head == nullptr && p == best_bid_) UpdateBestBid();
                    order_pool_.DeleteOrder(order);
                    order_map_.erase(it);
                    return;
                }
                current = current->next;
             }
        }
    }
     for (Price p = best_ask_; p < MAX_PRICE; ++p) {
        if(asks_[p].head) {
             HP_Order* current = asks_[p].head;
             while(current) {
                if(current->order_id == order_id) {
                    RemoveFromList(p, order, Side::SELL);
                    if (asks_[p].head == nullptr && p == best_ask_) UpdateBestAsk();
                    order_pool_.DeleteOrder(order);
                    order_map_.erase(it);
                    return;
                }
                current = current->next;
             }
        }
    }
}


void OrderBookV3::AddToList(Price price, HP_Order* order, Side side) {
    auto& level = (side == Side::BUY) ? bids_[price] : asks_[price];
    if (level.head == nullptr) {
        level.head = level.tail = order;
    } else {
        level.tail->next = order;
        order->prev = level.tail;
        level.tail = order;
    }
    level.total_quantity += order->quantity;
}

void OrderBookV3::RemoveFromList(Price price, HP_Order* order, Side side) {
    auto& level = (side == Side::BUY) ? bids_[price] : asks_[price];
    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;
    if (level.head == order) level.head = order->next;
    if (level.tail == order) level.tail = order->prev;
    level.total_quantity -= order->quantity;
}

void OrderBookV3::UpdateBestBid() {
    while (best_bid_ > 0 && bids_[best_bid_].head == nullptr) {
        best_bid_--;
    }
}

void OrderBookV3::UpdateBestAsk() {
    while (best_ask_ < MAX_PRICE && asks_[best_ask_].head == nullptr) {
        best_ask_++;
    }
}
