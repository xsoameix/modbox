#!/usr/bin/env lsc
require! <[net]>
sprintf = require \sprintf-js .sprintf

print = (buf, separator) -> for x, i in buf
  sepa = if i == buf.length - 1 then separator else ' '
  process.stdout.write sprintf \%02X%s, x, sepa
funcs = for x til 5
  ->
    sock = new net.Socket!
    recv = new Buffer 0
    send = []
    sock.on \data ->
      if recv.length < 2 || recv.length < 2 + recv[1]
        recv := Buffer.concat [recv, it]
      if recv.length >= 2 && recv.length >= 2 + recv[1]
        print send.shift!, ' => '
        print recv.slice(0, 2 + recv[1]), "\n"
        recv := recv.slice 2 + recv[1]
    sock.on \close -> console.log 'app disconnected'
    sock.on \error -> console.error it
    sock.connect 60000, '127.0.0.1', ->
      console.log 'app connected'
      send.push new Buffer "\x00\x06\x01\x03\x01\x10\x00\x01", \binary
      send.push new Buffer "\x00\x06\x01\x03\x01\x11\x00\x01", \binary
      send.for-each -> sock.write it
      sock.write new Buffer "\x00\x00", \binary
funcs.for-each -> it!
set-timeout ->
  funcs.for-each -> it!
, 4000
