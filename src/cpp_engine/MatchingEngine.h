#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include <cstdint>
#include <cstddef>
#include <vector>

// Constants for sizing the pre-allocated structures
constexpr size_t MAX_ORDERS = 10'000'000;
constexpr size_t MAX_PRICE_LEVELS = 1'000'000; // e.g., prices from 0.00 to 10,000.00 with 1 cent ticks

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

struct Order {
    uint64_t id;
    uint64_t price;
    uint32_t quantity;
    Side side;
    int32_t next_order_id; // -1 if null
    int32_t prev_order_id; // -1 if null
    bool is_active;
};

struct PriceLevel {
    int32_t head_order_id; // -1 if null
    int32_t tail_order_id; // -1 if null
    uint64_t total_volume;
};

class MatchingEngine {
private:
    Order* orders;
    PriceLevel* bid_levels;
    PriceLevel* ask_levels;

    uint64_t best_bid;
    uint64_t best_ask;

    void add_order_to_level(int32_t order_id, uint64_t price, Side side);
    void remove_order_from_level(int32_t order_id, uint64_t price, Side side);
    void execute_order(int32_t maker_order_id, uint32_t exec_qty);
    void match_order(int32_t taker_order_id);

public:
    MatchingEngine();
    ~MatchingEngine();

    // Core LOB Operations
    void add_order(uint64_t id, uint64_t price, uint32_t quantity, uint8_t side);
    void cancel_order(uint64_t id);
    void modify_order(uint64_t id, uint32_t new_quantity);

    // Getters for testing/snapshot
    uint64_t get_best_bid() const { return best_bid; }
    uint64_t get_best_ask() const { return best_ask; }
    uint64_t get_volume_at_price(uint64_t price, uint8_t side) const;
};

// C-API for C# interoperability
extern "C" {
    MatchingEngine* create_engine();
    void destroy_engine(MatchingEngine* engine);
    void engine_add_order(MatchingEngine* engine, uint64_t id, uint64_t price, uint32_t quantity, uint8_t side);
    void engine_cancel_order(MatchingEngine* engine, uint64_t id);
    uint64_t engine_get_best_bid(MatchingEngine* engine);
    uint64_t engine_get_best_ask(MatchingEngine* engine);
    uint64_t engine_get_volume_at_price(MatchingEngine* engine, uint64_t price, uint8_t side);
}

#endif // MATCHING_ENGINE_H
