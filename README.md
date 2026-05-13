# CS 551 - Project 3 - Chat Server

## Building and Running

The server listens on port 10551.

```sh
make
./chatserver
```

## Software Architecture

### Overview

When the program starts, a number of threads equal to the maximum number of connected clients is created. Each thread has a corresponding message queue (`waiting_messages` in `messages.h`).

The main thread queues up incoming connections, and each available threads dequeues a connection and hands it off to the client handler (`client_handle` in `client_handler.c`). 

The client handler then creates another thread for the client listener (`client_listen` in `client_handler.c`). The job of the client handler now is to wait for messages from the client and push them onto every message queue. The client listener waits for messages to enter its message queue, then sends them down to the client.

### Message Queues

Every message queue has a corresponding mutex and conditional variable, so the message queue for thread `i` is in `waiting_messages[i]`, which is controlled by the mutex in `waiting_messages_mutexes[i]` and the conditional variable in `waiting_messages_conds[i]`.

Message queues are implemented as [circular buffers](https://en.wikipedia.org/wiki/Circular_buffer), and therefore have a fixed capacity.

## Message Format

```json
{
  "from_addr": "127.0.0.1",
  "from_name": "anonymous",
  "message": "Hello, World!"
}
```

Messages are sent and received in the following format. Clients do not need to provide the `from_addr` field, as this is automatically filled in by the server. The `from_name` filed may also be blank. The server uses the [cJSON](https://github.com/davegamble/cjson) library for creating and parsing JSON.

## Known Quirks and Issues

- When a client disconnects, the message queue is not flushed, so the next client to connect on that thread receives all the remaining messages in that thread's message queue. Just clearing the queue would mean that a client would not be able to see messages that users sent before they connected. This could be remedied by maintaining a message queue containing the server's chat history (a `struct MessageQueue` with `.overwrite` set to `true`), and fill the client's message queue with the chat history when they connect.
- I do not know performance implications of a program having potentially hundreds of threads.
- If all threads are occupied with clients, any clients that attempt to connect will hang until a thread clears up.
