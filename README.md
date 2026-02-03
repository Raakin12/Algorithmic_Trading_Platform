# RM Capital Markets – Trading Workstation (C++/Qt + Cloud)

---

## 💡 What This Is

A real-time trading workstation I built from scratch for my M.Eng. capstone and beyond.  
Written in modern C++ with Qt, this system mimics pro-level desktop terminals. It streams high-frequency data, lets you place live orders, enforces custom risk, and syncs to a cloud backend for analytics and alpha scoring.  

**No vendor APIs, no black boxes—just full visibility and full control.**

---

## 🧠 Why I Built It

Markets aren’t a side interest — they’re how I spend most of my time, every day.

I trade crypto, futures, and FX. I backtest strategies, track my performance, and build tools to help me improve. I built this workstation because I needed it — to think more clearly, test faster, and move with more intent.

- **Full ownership.** I want to inspect every branch, optimize every loop, and ship fixes at 3:00 AM if that’s what gives me an edge.  
- **Relentless improvement.** I move fast, profiling, refactoring, and fine-tuning until the code fades and only the results remain.  
- **Career North Star.** My goal is to earn a seat in institutional trading, and I’m prepared to out-learn, out-build, and out-work to get there.

I’m the kind of candidate who keeps building after everyone else logs off. Ideas follow me onto flights, walks, and so-called weekends. For me, this isn’t work , it’s **oxygen**.

> _If I need something faster, safer, or smarter — I write it myself._


---

## 🔍 System Overview

| Area           | What’s Under the Hood                                                                 |
|----------------|----------------------------------------------------------------------------------------|
| **Market Data** | Tick-level price feed via Binance WebSocket, streamed in real-time with custom handlers |
| **Charting**     | Multi-timeframe candlestick engine built with custom Qt rendering — no third-party libraries |
| **Execution**    | Custom order ticket with real-time size/SL/DD checks before dispatch                 |
| **Risk Engine**  | Server-side SL/TP enforcement + account drawdown kill switch                         |
| **Account View** | Live P&L, Live Equity, alpha score, balance—streamed via zero-polling WebSocket                   |
| **Analytics**    | Daily alpha benchmarking vs BTC + per-user Sharpe-like score                         |
| **Architecture** | C++17 · Qt6 · Modular → UI / App / Services / Domain (below)                         |

---
## 🧱 Architecture Snapshot

           ┌────────────┐             ┌──────────────┐
           │  Binance   │             │ Cloud Server │
           └────┬───────┘             └───────┬──────┘
                │                             │
                ▼                             ▼
          ┌──────────────────────────────────────────┐
          │               Service                    │◄────────────────┐ 
          │  - WebSocket Clients                     │                 |
          │  - Rest Clients                          │                 |
          │  - DB Adapters                           │            ┌───────────┐            
          └──────────────┬───────────────────────────┘            |  Domain   |
                         │                                        └────┬──────┘
                         ▼                                             | 
          ┌──────────────────────────────────────────┐                 |
          │             Application                  │◄────────────────┘
          │  - Controllers                           │             
          │  - Command Handlers                      │                                                  
          └──────────────┬───────────────────────────┘             
                         |
		             |
			     ▼                                        
                    ┌───────┐                                    
                    │  UI   │                                    
                    └───────┘                                    
                                                    

    


## 🎥 Demo

[![Watch the demo](https://img.youtube.com/vi/YOUR_VIDEO_ID/0.jpg)](https://youtu.be/33d28Iv1PHs)

Click the thumbnail above to watch a full walk-through of the workstation in action.

---

## 🐳 Docker Setup (Pull & Run the Full Stack)

You can pull and run the trading workstation and cloud backend directly using Docker Hub — no build step required.

### 🔧 Prerequisites

- [Docker](https://www.docker.com/products/docker-desktop)
- X11 enabled (for GUI support if running the Qt workstation container)

---

### 🧱 Pull Prebuilt Images

~~~bash
# Pull trading system 
docker pull raakin12/trading_system:latest

# Pull cloud backend 
docker pull raakin12/trading_cloud:latest
~~~

---

### ▶️ Run the Containers

~~~bash
# Create docker network
docker network create trading_network

# Start the cloud system 
docker run -d \
  --name cloud_system \
  --network trading_network \
  --restart unless-stopped \
  -p 8000:8000 \
  raakin12/trading_cloud:latest

# Start the Qt GUI trading system
docker run -it \
  --name trading_system_qt \
  --network trading_network \
  --restart unless-stopped \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  raakin12/trading_system:latest
~~~
