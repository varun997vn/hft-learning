---
sidebar_position: 1
---

# Explain Like I'm 10: High Frequency Trading

Imagine you are playing a massive, fast-paced video game where the goal is to buy and sell rare trading cards. The rules are simple:
1. You want to buy cards when they are cheap.
2. You want to sell them when someone is willing to pay more.
3. **The catch:** Thousands of other players are trying to do the exact same thing, at the exact same time!

If you see a cheap card pop up, and you try to click "Buy", someone else might click it a split-second before you. If you are slow, you lose.

This project is like building the ultimate, super-fast robot to play this game for you. In the real world, this is called **High Frequency Trading (HFT)**, and instead of trading cards, we are trading stocks (pieces of a company).

Here is how our super-fast robot works, broken down into its main parts:

### 1. The Ears (ITCH Parser & Network Event Loop)
To buy and sell cards, you first need to hear what everyone else is doing. The stock market is constantly shouting out updates: *"Bob wants to buy 10 cards for $5!"* or *"Alice just sold 3 cards!"*

Our robot has a **Network Event Loop** which acts like a super-sensitive ear. It uses a technology called `epoll` to listen to the network without ever taking a break or getting distracted. 

When it hears an update, the message is in a secret code. The **ITCH Parser** is a lightning-fast translator. It reads the secret code and turns it into a language our robot understands, all in a fraction of a millionth of a second!

### 2. The Brain's Memory (Limit Order Book)
Imagine you have a giant whiteboard where you write down every single person who wants to buy or sell a card, organized by price. This is called a **Limit Order Book (LOB)**.

If our robot used a normal piece of paper, it would take too long to erase old names and write down new ones. Instead, our robot's "whiteboard" is built using special computer tricks (like *zero-allocation flat arrays*). It never throws paper away; it just recycles the same spots over and over again. This means our robot can update the whiteboard almost instantly without ever pausing to look for a new pen.

### 3. The Traffic Cop (SPSC Queue & Thread Affinity)
Our robot actually has multiple brains (called computer cores) working together. One brain is busy listening to the market (The Ears), and another brain is busy looking at the whiteboard to make decisions. 

We need a way for the "Ear" brain to pass notes to the "Decision" brain incredibly fast. If they bump into each other, they slow down. So, we built a **Single-Producer Single-Consumer (SPSC) Queue**. Think of it as a super-smooth, one-way conveyor belt. The Ear puts a note on the belt, and the Decision brain picks it up at the other end. No one bumps into each other!

We also glue these brains to their chairs using **Thread Affinity**. This stops the computer from forcing them to swap seats, which would waste precious time.

### 4. The Safety Inspector (Pre-Trade Risk)
Before the robot hits the "Buy" button, it has to make sure it's not doing something silly, like trying to buy 10 million cards when it only has $50. The **Pre-Trade Risk Engine** is a fast safety inspector that checks the math. If the math is wrong, it immediately blocks the trade.

### 5. The Mouth (OUCH Gateway & EMS)
When the robot is ready to make a trade, it needs to shout back to the market: *"I'll buy those cards!"* 

It uses the **Execution Management System (EMS)** to keep track of its own orders, making sure it remembers exactly what it asked for. Then, it uses the **OUCH Protocol Gateway** to translate its shout back into the market's secret code and sends it over the network.

### 6. The Diary (Async Logger)
Our robot needs to keep a diary of everything it does so we can check if it's working correctly later. But writing in a diary takes time! If the robot stops to write in its diary every time it makes a trade, it will lose the game.

So, we built an **Asynchronous Logger**. The robot just throws its diary entries into a separate pile without stopping. A completely different helper (a background thread) quietly picks up the entries and writes them into the actual book while the robot keeps playing the game.

### Summary
By building all these pieces using special math and deep knowledge of how computer hardware works, our robot can hear a price change, update its whiteboard, check the safety rules, and send a trade back to the market in just a few **nanoseconds** (a nanosecond is one-billionth of a second!). 

That's faster than you can blink!
