# Video Script (English, A2-B1, conversational)

**Project:** BLM 4140 — Wi-Fi Performance Analysis
**Target length:** ~6-7 minutes
**Style:** Talk to the camera. Short sentences. Don't read, just talk.

`[1:00]` markers are a rough timing guide.

---

## [0:00] Who I am  (~30s)

Hi! My name is Ozan Orhan. My student number is 21011077.

This is my final project for the course BLM 4140 — Wireless and
Mobile Networks. My teacher is Asst. Prof. Mehmet Şükrü Kuran.

In the next few minutes I will tell you what I did, show you my
results, and say what I learned.

## [0:30] What the project is about  (~1 min)

The question is simple:

> When many devices use the same Wi-Fi at the same time, what
> happens to the total speed?

A long time ago, a researcher called **Bianchi** said: *the more
devices share one Wi-Fi, the less total speed everyone gets*. The
reason is collisions — when two devices talk at the same moment,
both lose.

Bianchi wrote this in the year 2000. Today, Wi-Fi has many new
features — EDCA, A-MPDU, Block-ACK, TXOP. So I wanted to check:
is Bianchi still right? Or do the new features fix the problem?

I also compare two ways of sharing the Wi-Fi:

* **EDCA** — the normal way.
* **EDCA + RTS/CTS** — the same, but each device first asks
  "can I talk?" and waits for "yes".

## [1:30] What I did  (~1 min)

I used **NS-3**, a network simulator. I wrote one C++ file,
`wifi-project.cc`.

The setup:

* One Wi-Fi router (the AP)
* Around it: 4, 8, 12 or 20 devices
* All devices are very close (1 meter), so they reach the top
  speed
* They all upload TCP data at the same time
* I tried three loads: **50%, 80%, 90%** of the maximum Wi-Fi
  speed

That gives **24 scenarios** (4 × 2 × 3). I ran each one 5 times
with different random seeds to get stable averages. So **120
simulations**, about 20 minutes on my computer.

The teacher's PDF said "802.11ac", but he corrected it on May 14
— it should be **802.11n** at 2.4 GHz. So I used 802.11n with
the highest MCS, **72.2 Mbit/s**.

## [2:30] Quick look at the code  (~1 min)

*(Open `wifi-project.cc` briefly.)*

The file takes three things from the command line: the number of
devices, the MAC type and the load percentage. The same file
runs all 24 scenarios.

The four MAC features the project asks for are turned on like
this:

* **EDCA** is automatic when I pick 802.11n.
* **A-MPDU** is one line: `BE_MaxAmpduSize = 65535`.
* **Block-ACK** comes for free with A-MPDU.
* **TXOP** I set to **3.2 ms**, so devices can send a few
  packets in one turn.

**RTS/CTS** is a simple switch — when I want it on, the
threshold is 100 bytes (every packet triggers it). When I want
it off, the threshold is 65535 (nothing triggers it).

A small bash script (`run_all.sh`) loops over all 120 runs and
saves the results to a CSV. A Python script (`plot_results.py`)
makes the four figures.

## [3:30] Results — let's look at the graphs  (~2 min)

### Figure 1 — 50% load

At 50% load the Wi-Fi is not very busy. The two lines (EDCA in
blue, RTS/CTS in red) are very close to each other. The error
bars are big. So here, the choice between EDCA and RTS/CTS
doesn't really matter.

But notice: at **20 devices** both lines drop a lot. Even at low
load, too many devices is already a problem.

### Figure 2 — 80% load

Now the Wi-Fi is busy. **Big surprise:** the red line (RTS/CTS)
is above the blue line (EDCA) for *every* number of devices —
even with only 4!

This is the opposite of what the textbook says. The textbook
says RTS/CTS is bad with few devices because it adds extra
messages.

### Figure 3 — 90% load

Same picture, but stronger. With 20 devices RTS/CTS gives about
**38 Mbit/s**, but EDCA only gives **28 Mbit/s**. That's a big
difference.

### Figure 4 — All six lines on one plot

This last figure puts everything on one canvas:

1. **Every line goes down** as the number of devices grows.
   Bianchi was right, even with modern Wi-Fi.
2. At 80% and 90% load, **red is always above blue**.
3. At 50%, the two are almost on top of each other.

## [5:30] What I expected vs what I saw  (~1 min)

I expected: with few devices, RTS/CTS would be a little worse
(extra messages, no collisions to avoid). At high N, better.

What I saw: yes for "at high N, better" — but RTS/CTS was
better **even with only 4 devices** at high load! I did not
expect that.

Why? Two reasons:
1. With TCP, devices send **long packets**. Long packet
   collisions waste a lot of time. RTS/CTS makes the collisions
   **short**.
2. The AP also needs to send TCP acknowledgements back. With
   many devices uploading, the AP struggles to get the channel.
   RTS/CTS helps the AP grab its turn.

## [6:30] Conclusion + thanks  (~30s)

So what did I learn?

* Bianchi was right — 25 years later. The new Wi-Fi features
  help a little, but not enough.
* At low load, EDCA or RTS/CTS doesn't matter much.
* At high load, **RTS/CTS wins everywhere** — even with few
  devices, especially for TCP uploads.
* Practical advice: if many people upload at the same time
  (think: classroom of 20 students), turn RTS/CTS on.

All the files — code, results, graphs, report — are in the
project ZIP. Thank you for watching!
