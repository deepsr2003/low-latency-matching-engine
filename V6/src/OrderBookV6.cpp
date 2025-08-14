#include "OrderBookV6.h"
#include <algorithm>

// GCC/Clang builtins for bit manipulation
#if defined(__GNUC__) || defined(__clang__)
#define BUILTIN_CLZLL __builtin_clzll
#define BUILTIN_CTZLL __builtin_ctzll
#else
#error "Compiler not supported for bit manipulation builtins"
#endif

constexpr size_t BITMAP_SIZE = (MAX_PRICE / 64) + 1;

OrderBookV6::OrderBookV6() 
    : bids_(MAX_PRICE + 1), 
      asks_(MAX_PRICE + 1),
      order_pool_(MAX_ORDER_ID),
      order_map_(MAX_ORDER_ID, nullptr),
      best_bid_(0),
      best_ask_(MAX_PRICE),
      bids_bitmap_(BITMAP_SIZE, 0),
      asks_bitmap_(BITMAP_SIZE, 0) {}

inline void OrderBookV6::set_bit(Price p, std::vector<uint64_t>& bitmap) {
    bitmap[p >> 6] |= (1ULL << (p & 63));
}

inline void OrderBookV6::clear_bit(Price p, std::vector<uint64_t>& bitmap) {
    bitmap[p >> 6] &= ~(1ULL << (p & 63));
}

void OrderBookV6::AddOrder(OrderId order_id, Side side, Price price, Quantity quantity) {
    if (side == Side::BUY) {
        while (quantity > 0 && price >= best_ask_ && best_ask_ < MAX_PRICE) {
            PriceLevel_V6& level = asks_[best_ask_];
            if (level.head == nullptr) { UpdateBestAsk(); continue; }
            HP_Order_V6* current_order = level.head;
            while (current_order && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, current_order->quantity);
                current_order->quantity -= trade_quantity;
                quantity -= trade_quantity;
                level.total_quantity -= trade_quantity;
                if (current_order->quantity == 0) {
                    HP_Order_V6* next_order = current_order->next;
                    order_map_[current_order->order_id] = nullptr;
                    RemoveFromList(current_order);
                    order_pool_.DeleteOrder(current_order);
                    current_order = next_order;
                }
            }
            if (level.head == nullptr) {
                clear_bit(best_ask_, asks_bitmap_);
                UpdateBestAsk();
            }
        }

        if (quantity > 0) {
            bool is_new_level = (bids_[price].head == nullptr);
            HP_Order_V6* new_order = order_pool_.NewOrder();
            new_order->order_id = order_id;
            new_order->quantity = quantity;
            new_order->price = price;
            new_order->side = Side::BUY;
            AddToList(price, new_order);
            order_map_[order_id] = new_order;
            if (is_new_level) set_bit(price, bids_bitmap_);
            if (price > best_bid_) best_bid_ = price;
        }
    } else { // Side::SELL
        while (quantity > 0 && price <= best_bid_ && best_bid_ > 0) {
            PriceLevel_V6& level = bids_[best_bid_];
            if (level.head == nullptr) { UpdateBestBid(); continue; }
            HP_Order_V6* current_order = level.head;
            while (current_order && quantity > 0) {
                Quantity trade_quantity = std::min(quantity, current_order->quantity);
                current_order->quantity -= trade_quantity;
                quantity -= trade_quantity;
                level.total_quantity -= trade_quantity;
                if (current_order->quantity == 0) {
                    HP_Order_V6* next_order = current_order->next;
                    order_map_[current_order->order_id] = nullptr;
                    RemoveFromList(current_order);
                    order_pool_.DeleteOrder(current_order);
                    current_order = next_order;
                }
            }
            if (level.head == nullptr) {
                clear_bit(best_bid_, bids_bitmap_);
                UpdateBestBid();
            }
        }
        
        if (quantity > 0) {
            bool is_new_level = (asks_[price].head == nullptr);
            HP_Order_V6* new_order = order_pool_.NewOrder();
            new_order->order_id = order_id;
            new_order->quantity = quantity;
            new_order->price = price;
            new_order->side = Side::SELL;
            AddToList(price, new_order);
            order_map_[order_id] = new_order;
            if (is_new_level) set_bit(price, asks_bitmap_);
            if (price < best_ask_) best_ask_ = price;
        }
    }
}

void OrderBookV6::CancelOrder(OrderId order_id) {
    if (order_id >= order_map_.size()) return;
    HP_Order_V6* order = order_map_[order_id];
    if (order == nullptr) return;

    Price price = order->price;
    Side side = order->side;

    RemoveFromList(order);
    order_map_[order_id] = nullptr;
    order_pool_.DeleteOrder(order);

    if (side == Side::BUY && bids_[price].head == nullptr) {
        clear_bit(price, bids_bitmap_);
        if (price == best_bid_) UpdateBestBid();
    } else if (side == Side::SELL && asks_[price].head == nullptr) {
        clear_bit(price, asks_bitmap_);
        if (price == best_ask_) UpdateBestAsk();
    }
}


void OrderBookV6::AddToList(Price price, HP_Order_V6* order) {
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

void OrderBookV6::RemoveFromList(HP_Order_V6* order) {
    auto& level = (order->side == Side::BUY) ? bids_[order->price] : asks_[order->price];
    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;
    if (level.head == order) level.head = order->next;
    if (level.tail == order) level.tail = order->prev;
    level.total_quantity -= order->quantity;
    order->next = order->prev = nullptr;
}


void OrderBookV6::UpdateBestBid() {
    size_t index = best_bid_ >> 6;
    uint64_t chunk = bids_bitmap_[index];
    uint64_t mask = (1ULL << (best_bid_ & 63)) - 1;
    mask |= (1ULL << (best_bid_ & 63));
    chunk &= mask;

    while (chunk == 0) {
        if (index == 0) {
            best_bid_ = 0;
            return;
        }
        index--;
        chunk = bids_bitmap_[index];
    }
    best_bid_ = (index << 6) + (63 - BUILTIN_CLZLL(chunk));
}

void OrderBookV6::UpdateBestAsk() {
    size_t index = best_ask_ >> 6;
    uint64_t chunk = asks_bitmap_[index];
    uint64_t mask = ~((1ULL << (best_ask_ & 63)) - 1);
    chunk &= mask;
    
    while (chunk == 0) {
        index++;
        if (index >= BITMAP_SIZE) {
            best_ask_ = MAX_PRICE;
            return;
        }
        chunk = asks_bitmap_[index];
    }
    best_ask_ = (index << 6) + BUILTIN_CTZLL(chunk);
}
