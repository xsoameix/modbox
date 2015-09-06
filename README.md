# Introduction

The glue between the modbus and your application.

A server with time-division multiplexing support.

# Getted Started

1.  Install

    The [nitrogen compiler](https://github.com/xsoameix/nitrogen)
    is used to generate c code, please install it first.

        $ cmake .
        $ make

2.  Then you can simply run server:

        $ ./air
        Listening on 127.0.0.1:60000

Press Ctrl + C or send SIGTERM to shutdown the server.

# Environment Variables

`SENDER_ADDR`, `SENDER_PORT` set address and port of the server.

eg:

    $ CONNECT_ADDR=127.0.0.1 CONNECT_PORT=40000 ./air
    Listening on 127.0.0.1:40000

    $ SENDER_ADDR=::1 SENDER_PORT=40000 ./air
    Listening on ::1:40000

`CONNECT_ADDR`, `CONNECT_PORT` set address and port of the client.

eg:

    $ CONNECT_ADDR=192.168.1.1 CONNECT_PORT=400 ./air

# Test

Please install ruby and nodejs first.

    $ npm install
    $ ./scripts/test_worker.sh
    $ ./scripts/test_server.ls
    $ ./scripts/test_client
