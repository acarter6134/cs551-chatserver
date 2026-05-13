# CS 551 - Project 3 - Chat Server

## Building and Running

The server listens on port 10551.

```sh
make
./chatserver
```

## Message Format

```json
{
  "from_addr": "127.0.0.1",
  "from_name": "anonymous",
  "message": "Hello, World!"
}
```

Messages are sent and received in the following format. Clients do not need to provide the `from_addr` field, as this is automatically filled in by the server. The `from_name` filed may also be blank. The server uses the [cJSON](https://github.com/davegamble/cjson) library for creating and parsing JSON.
