import math
import time
import logging
from market_data_client import MarketDataClient

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("AvellanedaStoikov")

class AvellanedaStoikovMarketMaker:
    def __init__(self, client: MarketDataClient, gamma=0.1, sigma=1.0, k=1.5, T=1.0):
        self.client = client
        self.gamma = gamma      # Inventory risk aversion
        self.sigma = sigma      # Asset volatility
        self.k = k              # Order book liquidity parameter
        self.T = T              # Terminal time
        
        self.inventory = 0
        self.start_time = time.time()
        self.current_order_id = 1000
        
        self.active_bid_id = None
        self.active_ask_id = None

    def get_time_remaining(self):
        elapsed = time.time() - self.start_time
        # Normalize to T = 1.0 being 1 hour for this simulation
        t = elapsed / 3600.0
        return max(0.0, self.T - t)

    def calculate_reservation_price(self, mid_price):
        """
        r = s - q * gamma * sigma^2 * (T - t)
        """
        time_rem = self.get_time_remaining()
        r = mid_price - self.inventory * self.gamma * (self.sigma ** 2) * time_rem
        return r

    def calculate_optimal_spread(self):
        """
        delta = gamma * sigma^2 * (T - t) + (2/gamma)*ln(1 + gamma/k)
        """
        time_rem = self.get_time_remaining()
        term1 = self.gamma * (self.sigma ** 2) * time_rem
        term2 = (2.0 / self.gamma) * math.log(1.0 + (self.gamma / self.k))
        return term1 + term2

    def run(self):
        logger.info("Starting Avellaneda-Stoikov Market Maker...")
        try:
            while True:
                # Wait for next order book update
                update = self.client.wait_for_update()
                best_bid = update.get("BestBid", 0)
                best_ask = update.get("BestAsk", 0)
                
                if best_bid == 0 or best_ask == 0:
                    continue
                    
                mid_price = (best_bid + best_ask) / 2.0
                
                # Calculate AS parameters
                res_price = self.calculate_reservation_price(mid_price)
                optimal_spread = self.calculate_optimal_spread()
                
                # Calculate optimal bid and ask
                # Prices are in integer ticks
                optimal_bid = int(round(res_price - optimal_spread / 2.0))
                optimal_ask = int(round(res_price + optimal_spread / 2.0))
                
                logger.info(f"Mid: {mid_price}, Inv: {self.inventory}, ResPrice: {res_price:.2f}, Spread: {optimal_spread:.2f} -> Q: [{optimal_bid}, {optimal_ask}]")
                
                # Cancel old quotes
                if self.active_bid_id:
                    self.client.cancel_order(self.active_bid_id)
                if self.active_ask_id:
                    self.client.cancel_order(self.active_ask_id)
                    
                # Submit new quotes
                self.active_bid_id = self.current_order_id
                self.client.add_order(self.active_bid_id, optimal_bid, 100, side=0) # 0 = Buy
                self.current_order_id += 1
                
                self.active_ask_id = self.current_order_id
                self.client.add_order(self.active_ask_id, optimal_ask, 100, side=1) # 1 = Sell
                self.current_order_id += 1
                
                # Simulate inventory fill for testing purposes if market crosses our spread
                if optimal_ask <= best_bid:
                    self.inventory -= 100
                if optimal_bid >= best_ask:
                    self.inventory += 100
                    
        except KeyboardInterrupt:
            logger.info("Shutting down market maker.")

if __name__ == "__main__":
    client = MarketDataClient()
    mm = AvellanedaStoikovMarketMaker(client)
    mm.run()
