# dragonfly-cli

Command-line client for DragonFlyDB - A high-performance, multi-threaded key-value store with Redis-like protocol.

## Installation

```bash
npm install -g dragonfly-cli
```

**Requirements:**
- Node.js >= 12
- gcc compiler (for building the native client)

### Installing gcc

**macOS:**
```bash
xcode-select --install
```

**Ubuntu/Debian:**
```bash
sudo apt install gcc
```

**Windows:**
- Install MinGW from https://www.mingw-w64.org/

## Usage

```bash
# Connect to localhost:6379
dragonfly-cli

# Connect to remote server
dragonfly-cli <host> <port>

# Example
dragonfly-cli 51.21.226.149 6379
```

## Commands

Once connected, you can use these commands:

```
SET key value    - Set a key to a value
GET key          - Get the value of a key
BEGIN            - Start a transaction
COMMIT           - Commit a transaction
QUIT             - Disconnect from server
help             - Show help message
```

## Example Session

```bash
$ dragonfly-cli 51.21.226.149 6379
=== DragonFlyDB Client ===

Connecting to 51.21.226.149:6379...
Connected to DragonFlyDB at 51.21.226.149:6379
Type commands (SET key value, GET key, BEGIN, COMMIT, QUIT)
Type 'help' for command list

dragonfly> SET user:1 alice
OK

dragonfly> GET user:1
"alice"

dragonfly> BEGIN
OK

dragonfly> SET user:2 bob
OK

dragonfly> SET user:3 charlie
OK

dragonfly> COMMIT
OK

dragonfly> QUIT
OK

Disconnected.
```

## Features

- ✅ Redis-like protocol
- ✅ Transaction support (BEGIN/COMMIT)
- ✅ Multi-threaded server architecture
- ✅ Cross-platform (Linux, macOS, Windows)
- ✅ Low latency, high throughput

## Server

This client connects to DragonFlyDB servers. The server uses a nothing-shared architecture with:
- 4 shard workers for parallel processing
- Hash-based key routing
- Two-phase commit for transactions
- Thread-per-connection model

## License

MIT

## Author

Your Name <your.email@example.com>

## Repository

https://github.com/YOUR_USERNAME/dragonfly-cli

## Issues

https://github.com/YOUR_USERNAME/dragonfly-cli/issues
