#pragma once

// Use the same types as V4
#include "HP_Types.h"
#include "ObjectPool.h"
#include <vector>

// Re-use V4's order struct for simplicity
// using HP_Order_V6 = HP_Order_V4;
// using PriceLevel_V6 = PriceLevel_V4;

// V6 needs the V4-style struct that includes price/side for fast cancellation.
struct HP_Order_V6 {
  OrderId order_id;
  Quantity quantity;
  Price price;
  Side side;
  HP_Order_V6 *next = nullptr;
  HP_Order_V6 *prev = nullptr;
};

struct PriceLevel_V6 {
  Quantity total_quantity = 0;
  HP_Order_V6 *head = nullptr;
  HP_Order_V6 *tail = nullptr;
};

class OrderBookV6 {
public:
  OrderBookV6();
  void AddOrder(OrderId order_id, Side side, Price price, Quantity quantity);
  void CancelOrder(OrderId order_id);

private:
  void AddToList(Price price, HP_Order_V6 *order);
  void RemoveFromList(HP_Order_V6 *order);
  void UpdateBestBid();
  void UpdateBestAsk();

  inline void set_bit(Price p, std::vector<uint64_t> &bitmap);
  inline void clear_bit(Price p, std::vector<uint64_t> &bitmap);

  std::vector<PriceLevel_V6> bids_;
  std::vector<PriceLevel_V6> asks_;

  ObjectPool<HP_Order_V6> order_pool_;
  std::vector<HP_Order_V6 *> order_map_;

  Price best_bid_;
  Price best_ask_;

  std::vector<uint64_t> bids_bitmap_;
  std::vector<uint64_t> asks_bitmap_;
};
