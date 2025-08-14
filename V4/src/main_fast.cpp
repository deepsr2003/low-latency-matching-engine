#include "OrderBookV4.h"
#include <iostream>
#include <chrono>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Inlined fast integer parser
inline uint64_t parse_int(const char*& ptr) {
    uint64_t val = 0;
    while (*ptr >= '0' && *ptr <= '9') {
        val = val * 10 + (*ptr++ - '0');
    }
    return val;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <market_data_file.csv>" << std::endl;
        return 1;
    }

    const char* filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }
    size_t file_size = sb.st_size;

    const char* buffer = (const char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    close(fd);

    OrderBookV4 book;
    const char* ptr = buffer;
    const char* end = buffer + file_size;

    auto start_time = std::chrono::high_resolution_clock::now();

    while (ptr < end) {
        char type = *ptr;
        ptr += 2; // Skip type and comma
        
        char side_char = *ptr;
        ptr += 2; // Skip side and comma
        
        OrderId order_id = parse_int(ptr);
        
        if (type == 'A') {
            ptr++; // Skip comma
            Price price = parse_int(ptr);
            ptr++; // Skip comma
            Quantity quantity = parse_int(ptr);
            
            Side side = (side_char == 'B') ? Side::BUY : Side::SELL;
            book.AddOrder(order_id, side, price, quantity);

        } else if (type == 'C') {
            book.CancelOrder(order_id);
        }
        
        // Move to the next line
        while (ptr < end && *ptr != '\n') {
            ptr++;
        }
        if (ptr < end) ptr++;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "V4 Processing Time: " << duration.count() << " ms" << std::endl;

    munmap((void*)buffer, file_size);
    return 0;
}
