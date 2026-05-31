#include "../src/cpp_engine/MatchingEngine.h"
#include <chrono>
#include <iostream>

int main() {
    std::cout << "Initializing matching engine...\n";
    MatchingEngine engine;
    
    std::cout << "Running benchmark (10 Million orders)...\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    for(uint64_t i = 0; i < 10000000; i++) {
        // Pseudo-random prices and side
        uint64_t price = 5000 + (i % 100);
        uint8_t side = i % 2; 
        engine.add_order(i, price, 10, side);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Time for 10M orders: " << duration << " ms\n";
    std::cout << "Throughput: " << (10000000.0 / duration) * 1000.0 << " orders/sec\n";
    
    return 0;
}
