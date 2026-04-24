---
sidebar_position: 3
---

# Lesson 3: NUMA Architecture

In modern servers, CPUs are highly powerful but they are physically bound by space. Large enterprise machines often have multiple physical CPU chips (called Sockets) installed on the motherboard.

## What is Non-Uniform Memory Access (NUMA)?
When you have multiple physical CPUs, they cannot all share the exact same physical wires to the RAM, because the data traffic would cause a massive bottleneck. 

Instead, hardware engineers divide the RAM into "Nodes" and attach each Node directly to a specific CPU Socket.
- **Node 0** memory is physically wired to **CPU 0**.
- **Node 1** memory is physically wired to **CPU 1**.

This is NUMA (Non-Uniform Memory Access). It is "Non-Uniform" because the speed at which you can access memory depends on where the memory physically lives.

### Local vs. Remote Memory
If a thread running on **CPU 0** needs memory from **Node 0**, the access is incredibly fast. This is **Local Memory**.

If a thread running on **CPU 0** needs memory from **Node 1**, it has to reach across the motherboard over an interconnect bridge (Intel calls this UPI/QPI). This bridge acts like a toll booth. It adds significant latency and variance to the memory access. This is **Remote Memory**.

## Why Thread Pinning Matters
In a typical desktop application, the Operating System will freely move your application's threads around between different cores to balance the power load. 

However, if the OS moves our Producer thread from CPU 0 over to CPU 1, suddenly the SPSC Queue memory (which was allocated on Node 0) becomes "Remote", and our application suffers a massive latency penalty.

### The Solution
We use strict thread pinning (`pthread_setaffinity_np`) to forcefully lock our threads to specific logical cores, and we use `libnuma` (`numa_alloc_onnode`) to explicitly allocate our ring buffer into the exact RAM node physically connected to those cores. This guarantees we stay entirely within the fastest possible hardware path.
