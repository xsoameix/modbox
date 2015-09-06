#!/bin/bash

valgrind --track-origins=yes --leak-check=full --log-file=$HOME/vglog ./modbox &
pid=$!
sleep 1
read -r -d '' rubycode << RUBYCODE
require 'socket'
sock = 5.times.map { TCPSocket.new "127.0.0.1", 60000 }
sleep 1
sock.each &:close
RUBYCODE
echo "$rubycode" | ruby &
sleep 9
kill -s SIGINT $pid
sleep 2
if ps -p $pid > /dev/null; then
  kill -s SIGINT $pid
  wait $pid
fi
