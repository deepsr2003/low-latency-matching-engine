# Recreating the High-Performance Order Book

This project contains four C++ implementations of a financial order book, each demonstrating a more advanced optimization technique.


<img width="772" height="819" alt="Screenshot 2025-08-15 at 4 10 04 AM" src="https://github.com/user-attachments/assets/14d73647-2058-4d33-9b90-a488e98c9d1c" />


## Project Structure

- **/scripts**: Python scripts to generate dense and sparse market data.
- **/V1**: A correct baseline implementation using standard library containers (`std::map`, `std::list`).
- **/V3**: A high-performance version focused on cache-friendliness, using arrays, object pools, and intrusive linked lists.
- **/V4**: An optimized version that fixes the I/O bottleneck with `mmap` and improves order lookups with a `std::vector`.
- **/V6**: The final version using bitmaps and compiler intrinsics for O(1) best-price discovery, making it robust on sparse data.




V3 (50.2s): Still catastrophically slow due to the cancel-search problem, and now slightly worse because the BBO scan also has to travel further on average.
V4 (462ms): V4's performance has degraded significantly (from 86ms to 462ms). This is the algorithmic weakness we wanted to expose. In a sparse book, when the best price level is cleared, the next best price might be hundreds or thousands of price points away. V4's while (best_bid_--)... loop has to iterate through all those empty array slots, which takes time. The fast I/O can't hide a bad algorithm when the data pattern is hostile.
V6 (89ms): This is the ultimate result. V6's performance is almost identical to its dense data performance (89ms vs 78ms). It doesn't care that the prices are far apart. The bitmap and compiler intrinsics (__builtin_clzll) find the next active price level in a single, constant-time operation. It is completely robust against this "hostile" data pattern.





V1 (5.8s): This is our baseline. It's slow due to constant memory allocations (std::list nodes), cache misses (std::map node traversals), and slow stream I/O.
V3 (40.3s): This is the most interesting and important result. V3 is dramatically slower than V1. Why? The implementation of CancelOrder in V3 was intentionally naive. To cancel an order, it only had the OrderId. It had to do a brute-force search through the price levels to find the order's price and side. This linear scan of the entire book for every cancel operation is catastrophically slow and completely overwhelms any benefits gained from the object pool. It's a perfect example of how one bad algorithmic choice can destroy performance.
V4 (86ms): An incredible speedup! This leap comes from two key changes:
Fast I/O (mmap): We eliminated the C++ stream parsing bottleneck. Reading from memory is orders of magnitude faster than parsing from a file stream.
Fast Cancels: We replaced the unordered_map and the slow search with a direct std::vector lookup. Cancellations are now true O(1).
V6 (78ms): On dense data, V6 is roughly the same speed as V4, maybe slightly faster. This is expected. The price levels are close together, so V4's linear BBO scan (best_bid_--) is very cheap. It only has to check one or two empty slots. The bitmap in V6 adds a tiny bit of overhead but doesn't provide a huge advantage here.


❯ echo "--- Dense Data Benchmark ---"
./V1/orderbook_v1 market_data_large.csv
./V3/orderbook_v3 market_data_large.csv
./V4/orderbook_v4 market_data_large.csv
./V6/orderbook_v6 market_data_large.csv

echo "--- Sparse Data Benchmark ---"
./V3/orderbook_v3 market_data_sparse.csv
./V4/orderbook_v4 market_data_sparse.csv
./V6/orderbook_v6 market_data_sparse.csv
--- Dense Data Benchmark ---
V1 Processing Time: 5798 ms
V3 Processing Time: 40348 ms
V4 Processing Time: 86 ms
V4 Processing Time: 78 ms
--- Sparse Data Benchmark ---
V3 Processing Time: 50201 ms
V4 Processing Time: 462 ms
V4 Processing Time: 89 ms


### Version 1: The Baseline (Correctness First)

**Philosophy:**
Prioritize correctness and ease of understanding over speed. Use standard, off-the-shelf C++ library components to let the library handle the difficult parts like sorting and memory management. This version is the reliable "textbook" implementation.

**Key Data Structures:**
*   **Book Storage:** `std::map<Price, ...>`. A balanced binary tree that automatically keeps the price levels sorted. Bids use `std::greater` to sort from high-to-low.
*   **Price Level Queues:** `std::list<Order>`. A linked list is used at each price level to maintain First-In, First-Out (FIFO) order priority. It allows for efficient O(1) removal of an order from the middle if you have an iterator to it.
*   **Cancel Lookup:** `std::unordered_map<OrderId, ...>`. A hash map provides fast (average O(1)) lookup to find any order by its unique ID, which is essential for cancellations.

**How It Works:**
When an order is added, the book checks the `std::map` of the opposite side to see if it can be matched. If a buy order's price is higher than the best ask price, it iterates through the `std::list` of orders at that best ask price, creating trades. If the order isn't fully filled, it's added to a `std::list` on its own side of the book. For cancellations, the `unordered_map` is used to instantly find the order's location and its iterator, allowing it to be erased from the `std::list`.

**Performance Bottlenecks:**
1.  **Memory Allocation:** Every single order and price level creates multiple small memory allocations on the heap (`map` nodes, `list` nodes). This is very slow.
2.  **Cache Misses:** The nodes of the map and list are scattered randomly in memory. Traversing them causes the CPU to constantly fetch data from slow main memory instead of its fast cache, crippling performance.
3.  **Slow I/O:** Reading the file line-by-line using C++ streams is inefficient and a major bottleneck in itself.

---

### Version 3: The First High-Performance Attempt

**Philosophy:**
Apply "Mechanical Sympathy"—the principle of designing software that works in harmony with the underlying hardware. The main goal is to attack the memory allocation and cache miss problems from V1.

**Key Data Structures:**
*   **Book Storage:** `std::vector<PriceLevel>`. The `std::map` is replaced with a simple array. The price itself is used as a direct index into the array (e.g., an order at price 10000 goes into `bids_[10000]`). This guarantees data is stored contiguously, which is fantastic for the CPU cache.
*   **Price Level Queues:** An **Intrusive Linked List**. Instead of using `std::list`, the `next` and `prev` pointers are placed directly inside the `HP_Order` struct itself. This eliminates the need for separate list node allocations.
*   **Memory Management:** An **`ObjectPool`**. All `HP_Order` objects are pre-allocated into a large vector at startup. "Creating" a new order simply means taking a pointer from a free list, and "deleting" it means returning the pointer. This completely eliminates dynamic memory allocation from the critical processing loop.

**How It Works:**
The logic is similar to V1, but instead of relying on maps, it manually tracks the `best_bid_` and `best_ask_` prices. When a price level is emptied, it finds the next best price with a simple loop (e.g., `best_bid_--`). The `CancelOrder` logic in this version was intentionally flawed: it had to perform a slow, brute-force search through the book to find an order before it could be cancelled, revealing an algorithmic weakness.

**Performance Bottlenecks:**
1.  **Algorithmic Flaw:** The `CancelOrder` search is disastrously slow (O(N)), making this version slower than V1 in practice.
2.  **Linear BBO Scan:** The `best_bid_--` loop is efficient when prices are dense, but it will become a major problem when prices are far apart (sparse data).
3.  **Slow I/O:** It still uses the same slow file reading method as V1, hiding the true performance of the new data structures.

---

### Version 4: The I/O and Cancellation Fix

**Philosophy:**
Eliminate the remaining obvious bottlenecks. Treat the input data as a single block of memory and replace the last complex data structure (`unordered_map`) with the simplest possible alternative.

**Key Data Structures:**
*   **Cancel Lookup:** `std::vector<HP_Order*>`. The `unordered_map` is replaced with a simple vector. The `OrderId` is used as a direct index into this vector. This provides the fastest possible O(1) lookup with zero overhead.
*   **I/O:** **Memory-Mapped File (`mmap`)**. Instead of reading the file line by line, the entire file is mapped directly into the application's memory space in one efficient operating system call. The program then parses the data by simply moving a pointer through this memory buffer.

**How It Works:**
The core order book logic is identical to V3, but with a crucial fix. The `CancelOrder` method is now incredibly fast, using the `OrderId` to index directly into the `order_map_` vector. The main processing loop no longer performs any system calls or memory allocations; it's a tight loop that just reads from memory and calls the order book methods.

**Performance Characteristics:**
*   **Strength:** Extremely fast on dense data. The combination of cache-friendly structures, zero-allocation processing, and memory-mapped I/O makes it orders of magnitude faster than V1 and V3.
*   **Weakness:** The linear BBO scan (`best_bid_--`) is still present. On sparse data, where the next best price might be thousands of array slots away, this linear scan becomes the new bottleneck.

---

### Version 6: The Ultimate Algorithm

**Philosophy:**
Fix the final, subtle algorithmic weakness of V4 to achieve robust performance on *any* data pattern. This demonstrates mastery of low-level, architecture-aware optimization.

**Key Data Structures:**
*   **BBO Discovery:** **Bitmaps** (`std::vector<uint64_t>`). This is the key addition. Two bitmaps (one for bids, one for asks) are maintained alongside the book. Each bit in the bitmap corresponds to a price level. If `bids_bitmap_` has the 10000th bit set to `1`, it means there are orders at price 10000.
*   **Bit-Scanning:** **Compiler Intrinsics** (`__builtin_clzll`, `__builtin_ctzll`). These are special functions that map directly to single, highly-optimized CPU instructions for finding the first or last set bit in a 64-bit integer.

**How It Works:**
When an order is added to a previously empty price level, the corresponding bit in the bitmap is set to `1`. When a price level becomes empty, its bit is cleared to `0`. Instead of the slow `best_bid_--` loop, the `UpdateBestBid` function now uses the `__builtin_clzll` (Count Leading Zeros) intrinsic on the bitmap to find the next highest active price in a single, constant-time operation.

**Performance Characteristics:**
*   **Strength:** Achieves robust, peak performance on both dense and sparse data. The bitmap completely solves the linear scan problem, making BBO discovery an O(1) operation. This version has no remaining algorithmic bottlenecks.
*   **Weakness:** The code is more complex and less portable (it relies on GCC/Clang specific intrinsics). It represents a trade-off where maximum performance is chosen over simplicity.







## How to Build and Run

### 1. Generate Test Data

First, run the Python scripts to create the necessary CSV files.

```bash
mkdir -p build # Create a build directory at the project root
cd build
python3 ../scripts/generate_data_dense.py
python3 ../scripts/generate_data_sparse.py
