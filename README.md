# MiniRedis

A Redis-like in-memory database server built from scratch in C++17.

MiniRedis is a lightweight Redis-inspired server that implements TCP networking, RESP protocol handling, multiple data structures, key expiration, and basic persistence.

The project is being developed from scratch to understand how database servers such as Redis work internally.

---

## 🚀 Features

### Networking

- TCP server using POSIX sockets
- Runs on port `6379`
- Multiple client connections
- Thread-per-client architecture
- Handles partial TCP sends
- Supports multiple commands per connection

### RESP Protocol

MiniRedis supports Redis Serialization Protocol (RESP) requests.

Example:

```text
*1\r\n
$4\r\nPING\r\n
````

Response:

```text
+PONG\r\n
```

It also supports simple inline commands through tools such as `nc`.

---

## 📦 Supported Commands

### Strings

| Command         | Description                 |
| --------------- | --------------------------- |
| `SET key value` | Stores a value              |
| `GET key`       | Retrieves a value           |
| `DEL key`       | Deletes a key               |
| `EXISTS key`    | Checks whether a key exists |
| `KEYS *`        | Returns available keys      |

Example:

```text
SET name Sonakshi
GET name
```

---

### Lists

| Command  | Description                      |
| -------- | -------------------------------- |
| `LPUSH`  | Adds an element to the beginning |
| `RPUSH`  | Adds an element to the end       |
| `LPOP`   | Removes the first element        |
| `RPOP`   | Removes the last element         |
| `LRANGE` | Returns elements in a range      |

Example:

```text
LPUSH fruits apple
RPUSH fruits banana
LRANGE fruits 0 -1
```

---

### Sets

| Command     | Description         |
| ----------- | ------------------- |
| `SADD`      | Adds a member       |
| `SREM`      | Removes a member    |
| `SISMEMBER` | Checks membership   |
| `SMEMBERS`  | Returns all members |

Example:

```text
SADD colors red
SADD colors blue
SMEMBERS colors
```

---

### Hashes

| Command   | Description                   |
| --------- | ----------------------------- |
| `HSET`    | Sets a hash field             |
| `HGET`    | Gets a hash field             |
| `HDEL`    | Deletes a hash field          |
| `HGETALL` | Returns all fields and values |

Example:

```text
HSET user name Sonakshi
HSET user course BTech
HGET user name
HGETALL user
```

---

### Key Expiration

| Command  | Description                |
| -------- | -------------------------- |
| `EXPIRE` | Sets a key expiration time |
| `TTL`    | Returns remaining lifetime |

Example:

```text
SET session active
EXPIRE session 10
TTL session
```

After expiration, the key is automatically treated as expired.

---

### Persistence

| Command | Description                 |
| ------- | --------------------------- |
| `SAVE`  | Saves database data to disk |
| `LOAD`  | Loads saved data            |

The current persistence file is:

```text
dump.rdb
```

---

### Server Commands

```text
PING
QUIT
EXIT
```

Example:

```text
PING
```

Response:

```text
+PONG
```

---

# 🏗️ Architecture

```text
                Client
                  |
                  v
        +-------------------+
        |    TCP Server     |
        +-------------------+
                  |
                  v
        +-------------------+
        |   RESP Protocol   |
        +-------------------+
                  |
                  v
        +-------------------+
        | Command Handler   |
        +-------------------+
                  |
                  v
        +-------------------+
        |     Database      |
        +-------------------+
          /       |       \
         /        |        \
     Strings    Lists    Sets/Hashes
                  |
                  v
             Persistence
```

---

# 📁 Project Structure

```text
mini-redis/
│
├── src/
│   ├── main.cpp
│   │
│   ├── database/
│   │   ├── database.h
│   │   └── database.cpp
│   │
│   ├── command/
│   │   ├── command.h
│   │   └── command.cpp
│   │
│   ├── network/
│   │   ├── tcp_server.h
│   │   └── tcp_server.cpp
│   │
│   └── protocol/
│       ├── resp.h
│       └── resp.cpp
│
├── .gitignore
├── README.md
└── LICENSE
```

---

# 🛠️ Technologies Used

* **C++17**
* **POSIX Sockets**
* **TCP/IP**
* **Multithreading**
* **RESP Protocol**
* **STL Containers**
* **File I/O**
* **Mutex-based synchronization**

---

# 💻 Requirements

* macOS / Linux
* C++17 compatible compiler
* `g++` or `clang++`
* `netcat (nc)` for testing

---

# ⚙️ Build

Clone the repository:

```bash
git clone https://github.com/sonakshibadgurjar/mini-redis.git
cd mini-redis
```

Compile:

```bash
g++ -std=c++17 -pthread \
src/main.cpp \
src/database/database.cpp \
src/command/command.cpp \
src/network/tcp_server.cpp \
src/protocol/resp.cpp \
-o mini-redis
```

---

# ▶️ Run

Start the server:

```bash
./mini-redis
```

Expected output:

```text
MiniRedis server started on port 6379
```

---

# 🧪 Testing

Open another terminal and connect using `nc`:

```bash
nc localhost 6379
```

Then:

```text
PING
```

Response:

```text
+PONG
```

Test a string:

```text
SET name Sonakshi
GET name
```

Test TTL:

```text
SET test hello
EXPIRE test 10
TTL test
```

---

# 🔌 RESP Testing

MiniRedis also accepts RESP formatted requests.

Example:

```bash
printf '*1\r\n$4\r\nPING\r\n' | nc localhost 6379
```

Expected:

```text
+PONG
```

GET example:

```bash
printf '*2\r\n$3\r\nGET\r\n$4\r\nname\r\n' | nc localhost 6379
```

---

# 🔐 Concurrency

Each connected client is handled using a separate thread.

The database uses a mutex to protect shared data from concurrent access.

This allows multiple clients to interact with the same MiniRedis database.

---

# 🚧 Current Status

MiniRedis is currently under active development.

### Implemented

* [x] TCP server
* [x] Multiple clients
* [x] Thread-per-client handling
* [x] RESP protocol
* [x] String operations
* [x] List operations
* [x] Set operations
* [x] Hash operations
* [x] TTL / key expiration
* [x] Basic persistence
* [x] SAVE / LOAD
* [x] PING / QUIT / EXIT

### Planned

* [ ] Improve persistence format
* [ ] Persist TTL information
* [ ] Stronger data-type validation
* [ ] More Redis commands
* [ ] MGET / MSET
* [ ] INCR / DECR
* [ ] Database statistics
* [ ] Improved error handling
* [ ] Automated testing
* [ ] Performance benchmarking
* [ ] Stress testing

---

# 🎯 Learning Objectives

This project is being developed to understand:

* How TCP servers work
* Client-server architecture
* Socket programming
* Network protocols
* RESP serialization
* In-memory databases
* Data structures used in databases
* Concurrency and synchronization
* Key expiration
* Persistence
* Command processing
* Database server architecture

---

# 🔮 Future Scope

The project will gradually move toward a more complete Redis-like implementation by adding:

1. More Redis commands
2. Better persistence
3. Improved protocol support
4. Automated test suites
5. Performance benchmarking
6. Memory management improvements
7. More advanced concurrency
8. Monitoring and server statistics

---

# 👩‍💻 Author

**Sonakshi Badgurjar**

B.Tech Student
C++ | Data Structures & Algorithms | Systems Programming

---

## ⭐ Project Status

MiniRedis is an educational project built from scratch to explore the internal concepts behind Redis-like database servers.


