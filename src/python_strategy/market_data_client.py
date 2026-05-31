import zmq
import json
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("MarketDataClient")

class MarketDataClient:
    def __init__(self, market_data_url="tcp://localhost:5555", order_entry_url="tcp://localhost:5556"):
        self.context = zmq.Context()
        
        # Subscribe to market data updates
        self.sub_socket = self.context.socket(zmq.SUB)
        self.sub_socket.connect(market_data_url)
        self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, "MARKET_DATA")
        
        # Push orders to the matching engine
        self.push_socket = self.context.socket(zmq.PUSH)
        self.push_socket.connect(order_entry_url)

    def send_order(self, action: str, order_id: int, price: int, quantity: int, side: int):
        msg = {
            "Action": action,
            "Id": order_id,
            "Price": price,
            "Quantity": quantity,
            "Side": side
        }
        self.push_socket.send_string(json.dumps(msg))
        logger.debug(f"Sent order: {msg}")

    def add_order(self, order_id: int, price: int, quantity: int, side: int):
        self.send_order("ADD", order_id, price, quantity, side)

    def cancel_order(self, order_id: int):
        self.send_order("CANCEL", order_id, 0, 0, 0)

    def wait_for_update(self):
        topic = self.sub_socket.recv_string()
        data = self.sub_socket.recv_string()
        return json.loads(data)
