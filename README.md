# Multi-Client TCP Chat Server

A simple, lightweight multi-client chat server implementation in C using TCP sockets. The server supports up to 10 concurrent clients with colored output, message history, and automatic reconnection features.

## Features

- **Multi-client support**: Up to 10 concurrent users
- **Message history**: New clients automatically receive the last 100 messages
- **Colored output**: Each client gets a unique color for their messages
- **Auto-reconnection**: Clients automatically attempt to reconnect if disconnected
- **Real-time messaging**: Instant message broadcasting to all connected clients
- **Interactive terminal**: Raw mode input with proper backspace and enter handling
- **Graceful disconnection**: Proper cleanup when clients leave

## Requirements

- GCC compiler
- Unix-like operating system (Linux, macOS)
- POSIX-compliant system with socket support

## Installation

1. **Download the code:**
   - Click the green "Code" button above
   - Select "Download ZIP" and extract it to your desired location
   - Or use the GitHub CLI/Git if you prefer

2. **Compile the server and client:**
```bash
gcc -o server server.c
gcc -o client client.c
```

## Usage

### Starting the Server

Run the server first:
```bash
./server
```

The server will start listening on `localhost:8080` and display:
```
listening on port 8080
```

### Connecting Clients

In separate terminals, run the client:
```bash
./client
```

You'll be prompted to enter your name:
```
Enter your name: YourName
```

After entering your name, you'll see any existing chat history and can start typing messages.

### Chat Commands

- Type any message and press Enter to send
- Type `quit` to disconnect from the server
- Use Backspace to delete characters while typing

## Architecture

### Server (`server.c`)

- Uses `select()` for handling multiple clients concurrently
- Maintains a circular buffer for message history (100 messages)
- Assigns unique colors to each client
- Handles client connections, disconnections, and message broadcasting

### Client (`client.c`)

- Implements raw terminal mode for better user experience
- Handles real-time input/output using `select()`
- Automatic reconnection with 3-minute timeout
- Filters out arrow key sequences to prevent terminal artifacts

## Configuration

You can modify these constants in the source code:

- `PORT`: Server port (default: 8080)
- `MAX_CLIENTS`: Maximum concurrent clients (default: 10)
- `HISTORY_SIZE`: Number of messages to keep in history (default: 100)
- `RECONNECT_TIMEOUT`: Client reconnection timeout in seconds (default: 180)

## Technical Details

### Message Format
Messages are formatted as: `[Color][Username]: Message[Reset]`

### Connection Handling
- Server uses non-blocking I/O with `select()`
- Client maintains persistent connection with automatic retry
- Proper socket cleanup on disconnection

### Memory Management
- Fixed-size buffers to prevent overflow
- Circular buffer for efficient message history storage
- No dynamic memory allocation for simplicity

## Limitations

- Maximum 10 concurrent clients
- Local network only (127.0.0.1)
- No message encryption or authentication
- No persistent storage (history lost on server restart)
- Limited to printable ASCII characters

## Examples

### Starting a Chat Session

1. Start the server:
```bash
$ ./server
listening on port 8080
```

2. Connect first client:
```bash
$ ./client
Enter your name: Ahmed
[Ahmed]: Hello everyone!
```

3. Connect second client:
```bash
$ ./client
Enter your name: Mostafa
[Ahmed]: Hello everyone!  # (message history)
[Mostafa]: Hi Ahmed!
```

### Sample Chat Output

```
[Ahmed]: Hey, how's everyone doing?
[Mostafa]: Pretty good! Working on some C programming
[Rafik]: Same here! This chat server is neat
[Ahmed]: Thanks! It was fun to build
```

## Contributing

1. **Fork this repository** by clicking the "Fork" button at the top right
2. **Make your changes** in your forked repository
3. **Create a Pull Request** by clicking "New Pull Request" on your fork
4. **Describe your changes** and submit for review

### Ways to Contribute:
- Report bugs by opening an Issue
- Suggest new features in the Issues section
- Improve documentation
- Add new features or fix bugs

## Troubleshooting

### Common Issues

**"Cannot connect to server"**
- Make sure the server is running first
- Check if port 8080 is available
- Verify firewall settings

**"Connection lost while sending message"**
- Server may have crashed or been terminated
- Client will automatically attempt to reconnect

**Strange characters in terminal**
- Terminal may not support ANSI colors
- Try running in a different terminal emulator

**Compilation errors**
- Ensure you have GCC installed
- Check that you're on a POSIX-compliant system
- Some systems may need additional flags: `gcc -o server server.c -std=c99`

### Getting Help
- Check the [Issues](../../issues) section for similar problems
- Open a new Issue if you need help
- Include your operating system and compiler version when reporting bugs

## Authors
- [@ahmedmedhat296](https://github.com/ahmedmedhat296)
- [@mostefmedhat](https://github.com/mostefmedhat)
