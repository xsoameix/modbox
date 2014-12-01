# Getted Started

1.  Configure server ip, client ip and port

        $ mv config_example.h config.h

    Please edit SERVER_PORT, CLIENT_IP, CLIENT_PORT in config.h

2.  Compile

    The [nitrogen compiler](https://github.com/xsoameix/nitrogen)
    is used to generate c code, please install it first.

        $ nitrogen server.c
        $ cmake .
        $ make

3.  Then you can simply run server:

        $ ./air
        Listening on port 9999
