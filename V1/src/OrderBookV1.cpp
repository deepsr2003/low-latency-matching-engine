#include "OrderBookV1.h"

void OrderBookV1::AddOrder(OrderId order_id, Side side, Price price, Quantity quantity) {
    if (side == Side::BUY) {
        // Match against asks
        while (quantity > 0 && !asks_.empty() && price >= asks_.begin()->first) {
            auto& best_ask_level = asks_.begin()->second;
            auto it = best_ask_level.begin();
            while (it != best_ask_level.end() && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, it->quantity);
                
                it->quantity -= trade_quantity;
                quantity -= trade_quantity;

                if (it->quantity == 0) {
                    order_map_.erase(it->order_id);
                    it = best_ask_level.erase(it);
                }
            }
            if (best_ask_level.empty()) {
                asks_.erase(asks_.begin());
            }
        }
        
        // Add remaining quantity to bids
        if (quantity > 0) {
            Order new_order = {order_id, price, quantity, side};
            auto& level = bids_[price];
            level.push_back(new_order);
            order_map_[order_id] = {price, side, std::prev(level.end())};
        }
    } else { // Side::SELL
        // Match against bids
        while (quantity > 0 && !bids_.empty() && price <= bids_.begin()->first) {
            auto& best_bid_level = bids_.begin()->second;
            auto it = best_bid_level.begin();
            while (it != best_bid_level.end() && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, it->quantity);

                it->quantity -= trade_quantity;
                quantity -= trade_quantity;

                if (it->quantity == 0) {
                    order_map_.erase(it->order_id);
                    it = best_bid_level.erase(it);
                }
            }
            if (best_bid_level.empty()) {
                bids_.erase(bids_.begin());
            }
        }
        
        // Add remaining quantity to asks
        if (quantity > 0) {
            Order new_order = {order_id, price, quantity, side};
            auto& level = asks_[price];
            level.push_back(new_order);
            order_map_[order_id] = {price, side, std::prev(level.end())};
        }
    }
}

void OrderBookV1::CancelOrder(OrderId order_id) {
    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) {
        return; // Order not found
    }

    const auto& loc = it->second;
    if (loc.side == Side::BUY) {
        auto& level = bids_.at(loc.price);
        level.erase(loc.it);
        if (level.empty()) {
            bids_.erase(loc.price);
        }
    } else { // Side::SELL
        auto& level = asks_.at(loc.price);
        level.erase(loc.it);
        if (level.empty()) {
            asks_.erase(loc.price);
        }
    }
    order_map_.erase(it);
}
