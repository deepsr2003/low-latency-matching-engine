#include "OrderBookV3.h" // Changed include
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <market_data_file.csv>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    OrderBookV3 book; // Changed type
    std::string line;
    
    auto start_time = std::chrono::high_resolution_clock::now();

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        
        char type;
        char side_char;
        OrderId order_id;
        Price price;
        Quantity quantity;
        
        std::getline(ss, field, ',');
        type = field[0];
        
        std::getline(ss, field, ',');
        side_char = field[0];
        
        std::getline(ss, field, ',');
        order_id = std::stoull(field);
        
        if (type == 'A') {
            std::getline(ss, field, ',');
            price = std::stoul(field);
            
            std::getline(ss, field, ',');
            quantity = std::stoul(field);

            Side side = (side_char == 'B') ? Side::BUY : Side::SELL;
            book.AddOrder(order_id, side, price, quantity);

        } else if (type == 'C') {
            book.CancelOrder(order_id);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "V3 Processing Time: " << duration.count() << " ms" << std::endl;

    return 0;
}
