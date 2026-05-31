<div align="center">
  <h1>🚀 High-Frequency Limit Order Book & Matching Engine</h1>
  <p><strong>An elite, production-grade Quantitative Trading Architecture in C++, C#, and Python</strong></p>
  
  [![C++](https://img.shields.io/badge/C++-17-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
  [![C#](https://img.shields.io/badge/C%23-.NET%208.0-purple.svg?style=flat&logo=c-sharp)](https://docs.microsoft.com/en-us/dotnet/csharp/)
  [![Python](https://img.shields.io/badge/Python-3.9+-yellow.svg?style=flat&logo=python)](https://www.python.org/)
  [![License](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
</div>

<br/>

## 📖 Introduction

In the highly competitive world of algorithmic trading, latency is the ultimate metric. This repository showcases a **Level 3 (L3) Limit Order Book (LOB) matching engine** engineered from scratch to meet the punishing latency and throughput constraints of modern High-Frequency Trading (HFT). 

Built as a polyglot architecture, it leverages the unique strengths of three different programming languages:
1. **C++**: A zero-allocation, cache-optimized core matching engine pushing **~47.8 Million orders per second**.
2. **C# (.NET Core)**: An asynchronous, high-performance Order Management System (OMS) and network gateway handling interoperability and market data distribution.
3. **Python**: A quantitative edge node implementing the industry-standard **Avellaneda-Stoikov (2006)** market-making model.

This project is tailored to demonstrate deep systems-level optimization, hardware-aware data structure design, and advanced quantitative modeling capabilities—specifically engineered for quantitative trading internships and roles at elite market makers (e.g., SIG, Citadel, Optiver).

---

## 🏗️ Architectural Overview

The architecture is explicitly decoupled, operating across three distinct layers to ensure isolation, scalability, and optimal performance.

```mermaid
graph TD
    A[Python: Avellaneda-Stoikov Market Maker] -->|ZeroMQ Push (Orders)| B(C#: .NET Trading Server)
    B -->|ZeroMQ Pub (L3 Market Data)| A
    B -->|DllImport / P-Invoke| C{C++: LOB Matching Engine}
    C -->|O-1 Execution| C
```

---

## 🛠️ Phase-by-Phase Deep Dive

### Phase 1: The C++ Level 3 Matching Engine (The Core)
*Located in `/src/cpp_engine`*

Standard retail trading relies on Level 1 (Top of Book) or Level 2 (Aggregated Depth) data. This engine processes **Level 3 data**, providing an unfiltered, message-by-message view of every order addition, execution, modification, and cancellation.

#### Hardware-Aware Optimization & Data Structures
A naive Limit Order Book utilizes `std::map`, `std::unordered_map`, or a node-based linked list. However, dynamic memory allocation (`new`/`delete`), hashing overhead, and non-contiguous memory access cause severe CPU cache misses and Translation Lookaside Buffer (TLB) stalls.

To combat this, the engine employs **Pre-allocated Flat Arrays**:
- **Massive Contiguous Vector (`std::vector<Order>`)**: Orders are mapped where `OrderID == Index`. When an order is added, it requires zero hashing—just a direct, $O(1)$ memory jump.
- **Intrusive Pointers**: Instead of actual pointers, orders track the integer index of `next_order` and `prev_order`, forming a logical linked-list inside the flat array for time-priority (FIFO) matching.
- **Flat Price Levels**: An array of size `MAX_PRICE_TICKS` (e.g., 1,000,000 ticks) holds a `PriceLevel` struct. Finding the best bid or ask is an $O(1)$ array lookup `levels[price]`.

#### C-ABI Interoperability
The engine is compiled into a shared library (`.so` or `.dylib`) using Clang with `-O3 -march=native -flto` flags. It exposes `extern "C"` bindings, allowing managed languages to call it directly without the overhead of inter-process communication.

**Benchmark**: Achieving **~47,846,900 orders/sec** (approx. 209ms for 10 million orders).

---

### Phase 2: The C# .NET Trading Server (The Host)
*Located in `/src/trading_server`*

The Trading Server acts as the Order Management System (OMS). Built using the .NET Generic Host (`IHostBuilder`), it manages the lifecycle of the C++ engine and provides a high-throughput networking layer.

#### Zero-Copy Interop & P/Invoke
Using `DllImport`, the C# server maps the C++ bindings natively into the CLR. Because the data types passed (integers, bytes) are blittable, the CLR pins the memory and executes the C++ instructions in $O(1)$ time with zero garbage collection overhead.

#### Asynchronous Networking with ZeroMQ
To communicate with quantitative edges, the server runs a `BackgroundService` (`ZeroMQGateway`):
- **Order Ingestion (`PULL` socket)**: Deserializes incoming order JSON messages lock-free and pushes them into the C++ engine.
- **Market Data Feed (`PUB` socket)**: Upon every matching event, the server publishes a delta update of the LOB (Best Bid / Best Ask) to all subscribed algorithmic clients over TCP.

---

### Phase 3: The Python Market Maker (The Strategy)
*Located in `/src/python_strategy`*

With the blazing-fast execution infrastructure in place, the system deploys an advanced market-making algorithm based on the **Avellaneda-Stoikov (2006)** model. 

Market makers face two primary risks:
1. **Inventory Risk**: The asset's mid-price drifts, leaving the dealer holding a depreciating position.
2. **Transaction Risk**: The Poisson arrival of aggressive market orders.

#### The Avellaneda-Stoikov Mathematics
The Python script connects via ZeroMQ (`SUB` and `PUSH`) and dynamically recalculates quotes on every L3 tick:

**1. The Reservation Price ($r$)**: 
The algorithm calculates an "indifference" price, adjusting the actual mid-price ($s$) based on current inventory ($q$). If the dealer is long, the reservation price is lowered to incentivize selling.
$$ r = s - q \cdot \gamma \cdot \sigma^2 \cdot (T - t) $$
*(Where $\gamma$ is risk aversion, $\sigma^2$ is volatility, and $T-t$ is remaining time).*

**2. The Optimal Spread ($\delta$)**:
The algorithm calculates the perfect symmetric spread around the reservation price by analyzing the liquidity of the order book ($\kappa$). Greater competition forces a narrower spread.
$$ \delta = \gamma \cdot \sigma^2 \cdot (T - t) + \frac{2}{\gamma} \ln\left(1 + \frac{\gamma}{\kappa}\right) $$

**3. Execution**:
The algorithm calculates the integer ticks for `optimal_bid` and `optimal_ask`, cancels its previous active orders, and injects the new orders into the C++ core via the C# gateway.

---

## 🚀 Getting Started

### Prerequisites
- **C++ Compiler**: `clang++` or `g++` (supports C++17)
- **.NET SDK**: .NET 8.0+
- **Python**: 3.9+
- **ZeroMQ**: Native libs installed on OS

### 1. Build the C++ Engine
```bash
cd src/cpp_engine
make
```
*This generates `libmatchingengine.so` (Linux) or `libmatchingengine.dylib` (macOS).*

### 2. Run the C# Trading Server
```bash
cd src/trading_server
dotnet run
```
*The server will initialize the C++ engine and bind ZeroMQ to ports `5555` and `5556`.*

### 3. Start the Python Market Maker
```bash
cd src/python_strategy
pip install -r requirements.txt
python avellaneda_stoikov.py
```
*The Python client will connect to the C# gateway and begin calculating Avellaneda-Stoikov quotes.*

### 4. Run the C++ Benchmark (Optional)
```bash
cd tests
clang++ -O3 -std=c++17 -march=native -o benchmark benchmark.cpp ../src/cpp_engine/MatchingEngine.cpp
./benchmark
```

---

## 🎯 Future Roadmap (Simulating LOBSTER)

The next step for this architecture is backtesting against historical Level 3 data. Using datasets like **NASDAQ's Historical TotalView-ITCH** (accessible via LOBSTER), researchers can pipe massive historical order book messages directly into the C# ZeroMQ gateway. This allows the Python strategy to be backtested against highly realistic queue positions and tick-level microstructure dynamics.
