#include "MatchingEngine.h"
#include <algorithm>
#include <iostream>

MatchingEngine::MatchingEngine() {
    // Pre-allocate massive flat arrays to avoid runtime allocations
    orders = new Order[MAX_ORDERS];
    bid_levels = new PriceLevel[MAX_PRICE_LEVELS];
    ask_levels = new PriceLevel[MAX_PRICE_LEVELS];

    for (size_t i = 0; i < MAX_ORDERS; ++i) {
        orders[i].is_active = false;
        orders[i].next_order_id = -1;
        orders[i].prev_order_id = -1;
    }

    for (size_t i = 0; i < MAX_PRICE_LEVELS; ++i) {
        bid_levels[i] = {-1, -1, 0};
        ask_levels[i] = {-1, -1, 0};
    }

    best_bid = 0;
    best_ask = MAX_PRICE_LEVELS - 1;
}

MatchingEngine::~MatchingEngine() {
    delete[] orders;
    delete[] bid_levels;
    delete[] ask_levels;
}

void MatchingEngine::add_order_to_level(int32_t order_id, uint64_t price, Side side) {
    PriceLevel* levels = (side == Side::Buy) ? bid_levels : ask_levels;
    PriceLevel& level = levels[price];

    Order& o = orders[order_id];
    o.next_order_id = -1;
    o.prev_order_id = level.tail_order_id;

    if (level.tail_order_id != -1) {
        orders[level.tail_order_id].next_order_id = order_id;
    } else {
        level.head_order_id = order_id; // First order at this price
    }

    level.tail_order_id = order_id;
    level.total_volume += o.quantity;

    // Update best bid/ask
    if (side == Side::Buy && price > best_bid) {
        best_bid = price;
    } else if (side == Side::Sell && price < best_ask) {
        best_ask = price;
    }
}

void MatchingEngine::remove_order_from_level(int32_t order_id, uint64_t price, Side side) {
    PriceLevel* levels = (side == Side::Buy) ? bid_levels : ask_levels;
    PriceLevel& level = levels[price];
    Order& o = orders[order_id];

    if (o.prev_order_id != -1) {
        orders[o.prev_order_id].next_order_id = o.next_order_id;
    } else {
        level.head_order_id = o.next_order_id;
    }

    if (o.next_order_id != -1) {
        orders[o.next_order_id].prev_order_id = o.prev_order_id;
    } else {
        level.tail_order_id = o.prev_order_id;
    }

    level.total_volume -= o.quantity;
    
    // Efficiently update best bid/ask by scanning backwards/forwards
    if (level.total_volume == 0) {
        if (side == Side::Buy && price == best_bid) {
            while (best_bid > 0 && bid_levels[best_bid].total_volume == 0) {
                best_bid--;
            }
        } else if (side == Side::Sell && price == best_ask) {
            while (best_ask < MAX_PRICE_LEVELS - 1 && ask_levels[best_ask].total_volume == 0) {
                best_ask++;
            }
        }
    }
}

void MatchingEngine::execute_order(int32_t maker_order_id, uint32_t exec_qty) {
    Order& maker = orders[maker_order_id];
    maker.quantity -= exec_qty;
    
    PriceLevel* levels = (maker.side == Side::Buy) ? bid_levels : ask_levels;
    levels[maker.price].total_volume -= exec_qty;

    if (maker.quantity == 0) {
        remove_order_from_level(maker_order_id, maker.price, maker.side);
        maker.is_active = false;
    }
}

void MatchingEngine::match_order(int32_t taker_order_id) {
    Order& taker = orders[taker_order_id];
    bool is_buy = (taker.side == Side::Buy);

    while (taker.quantity > 0) {
        // Can we match?
        if (is_buy && best_ask <= taker.price) {
            PriceLevel& level = ask_levels[best_ask];
            if (level.head_order_id == -1) break; // Should not happen if best_ask is correct

            int32_t maker_id = level.head_order_id;
            Order& maker = orders[maker_id];

            uint32_t exec_qty = std::min(taker.quantity, maker.quantity);
            execute_order(maker_id, exec_qty);
            taker.quantity -= exec_qty;

        } else if (!is_buy && best_bid >= taker.price && best_bid > 0) {
            PriceLevel& level = bid_levels[best_bid];
            if (level.head_order_id == -1) break;

            int32_t maker_id = level.head_order_id;
            Order& maker = orders[maker_id];

            uint32_t exec_qty = std::min(taker.quantity, maker.quantity);
            execute_order(maker_id, exec_qty);
            taker.quantity -= exec_qty;
        } else {
            break; // No more matches possible
        }
    }
}

void MatchingEngine::add_order(uint64_t id, uint64_t price, uint32_t quantity, uint8_t side_val) {
    // We assume ID is sequential and used as array index for O(1) lookup
    if (id >= MAX_ORDERS || price >= MAX_PRICE_LEVELS) return;

    Side side = static_cast<Side>(side_val);
    Order& o = orders[id];
    o.id = id;
    o.price = price;
    o.quantity = quantity;
    o.side = side;
    o.is_active = true;

    // Match first
    match_order(id);

    // If still quantity left, add to book
    if (o.quantity > 0) {
        add_order_to_level(id, price, side);
    } else {
        o.is_active = false;
    }
}

void MatchingEngine::cancel_order(uint64_t id) {
    if (id >= MAX_ORDERS) return;
    Order& o = orders[id];
    if (o.is_active) {
        remove_order_from_level(id, o.price, o.side);
        o.is_active = false;
    }
}

void MatchingEngine::modify_order(uint64_t id, uint32_t new_quantity) {
    if (id >= MAX_ORDERS) return;
    Order& o = orders[id];
    if (!o.is_active) return;

    if (new_quantity < o.quantity) {
        // Lose priority? Usually no for reduce. Just update volume.
        PriceLevel* levels = (o.side == Side::Buy) ? bid_levels : ask_levels;
        levels[o.price].total_volume -= (o.quantity - new_quantity);
        o.quantity = new_quantity;
    } else {
        // Increase qty usually means losing time priority
        cancel_order(id);
        add_order(id, o.price, new_quantity, static_cast<uint8_t>(o.side));
    }
}

uint64_t MatchingEngine::get_volume_at_price(uint64_t price, uint8_t side_val) const {
    if (price >= MAX_PRICE_LEVELS) return 0;
    Side side = static_cast<Side>(side_val);
    if (side == Side::Buy) {
        return bid_levels[price].total_volume;
    } else {
        return ask_levels[price].total_volume;
    }
}


// C-API Export implementations
extern "C" {
    MatchingEngine* create_engine() {
        return new MatchingEngine();
    }

    void destroy_engine(MatchingEngine* engine) {
        delete engine;
    }

    void engine_add_order(MatchingEngine* engine, uint64_t id, uint64_t price, uint32_t quantity, uint8_t side) {
        if (engine) engine->add_order(id, price, quantity, side);
    }

    void engine_cancel_order(MatchingEngine* engine, uint64_t id) {
        if (engine) engine->cancel_order(id);
    }

    uint64_t engine_get_best_bid(MatchingEngine* engine) {
        return engine ? engine->get_best_bid() : 0;
    }

    uint64_t engine_get_best_ask(MatchingEngine* engine) {
        return engine ? engine->get_best_ask() : 0;
    }

    uint64_t engine_get_volume_at_price(MatchingEngine* engine, uint64_t price, uint8_t side) {
        return engine ? engine->get_volume_at_price(price, side) : 0;
    }
}
