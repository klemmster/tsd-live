#!/usr/bin/env python

import struct
import sys
import random

import serial

from autobahn.websocket import listenWS
from autobahn.websocket import connectWS
from autobahn.wamp import WampServerFactory
from autobahn.wamp import WampClientFactory
from autobahn.wamp import WampServerProtocol
from autobahn.wamp import WampClientProtocol
from twisted.internet import reactor
from twisted.internet.serialport import SerialPort
from twisted.internet.protocol import Protocol, Factory
from twisted.web.server import Site
from twisted.web.static import File
from twisted.python import log
from twisted.protocols.policies import TimeoutMixin

log.startLogging(sys.stdout)


class TSPublisher(WampServerProtocol):

    def onSessionOpen(self):
        ## register a URI and all URIs having the string as prefix as PubSub topic
        self.registerForPubSub("http://coding-reality.de/tsd-event", True)


class TSClient(WampClientProtocol):
    def onSessionOpen(self):
        print "Open Session"
        self._initSerialPort()

    def _initSerialPort(self):
        try:
            serialClient = SerialClient(self.sendTSDEvent, self.resetConnection)
            self._dataInput = MySerialPort(serialClient, 'COM4', reactor,
                    baudrate='9600')
            reactor.callLater(2, serialClient.setTimeout, 1)
        except Exception:
            log.err("Can't open COM4")
            reactor.callLater(5, self._initSerialPort)
            self._dataInput = None

    def sendTSDEvent(self, event):
        self.publish("http://coding-reality.de/tsd-event", event)

    def resetConnection(self):
        if self._dataInput:
            self._dataInput.loseConnection()
            self._dataInput.stopConsuming()
            self._dataInput.stopProducing()
            self._dataInput = None
        reactor.callLater(1, self._initSerialPort)


class Packet(object):
    def __init__(self, data):
        self.clientID = struct.unpack_from("<h", data, 0)[0]
        self.type_ = struct.unpack_from("B", data, 2)[0]
        self.timestamp = struct.unpack_from("f", data, 3)[0]
        self.data = struct.unpack_from("f", data, 7)[0]

    def __repr__(self):
        return "ID: %s; Typ: %s, time: %f; data: %f" % (self.clientID,
                self.type_, self.timestamp, self.data)

    def toDict(self):
        return {'id': self.clientID, 'type': self.type_, 'timestamp': self.timestamp, 'data': self.data}
    

class MySerialPort(SerialPort):
    def connectionLost(self, reason):
        print reason


class SerialClient(Protocol, TimeoutMixin):
    def __init__(self, publishCB, resetConnection):
        self._publish = publishCB
        self._timeoutCB = resetConnection

    def connectionFailed(self):
        log.err("Connection failed")

    def timeoutConnection(self):
        log.msg("Connection timeout")
        self._timeoutCB()
        self.setTimeout(None)

    def connectionMade(self):
        log.msg("Connected to MCP")
        self.sendCMD('GET')
        self.resetTimeout()

    def sendCMD(self, cmd, data=None):
        if not cmd in ['GET']:
            raise Exception("UNKNOWN COMMAND")
        self.transport.write(cmd)
        if data:
            self.transport.write(data)
        self.resetTimeout()

    def dataReceived(self, data):
        self.resetTimeout()
        print len(data)
        if len(data) == 11:
            packet = Packet(data)
            print packet
            self._publish(packet.toDict())
            reactor.callLater(0.1, self.sendCMD, 'GET')
        else:
           log.msg("Received Gargabe")
           log.msg("LEN: %s " % len(data))
           log.msg(data)

    def lineReceived(self, line):
        log.msg("Line Received")
        log.msg(line)


if __name__ == '__main__':
    ##Setup Server
    wampServerFactory = WampServerFactory("ws://localhost:9000")
    wampServerFactory.protocol = TSPublisher
    listenWS(wampServerFactory)

    ##Setup Client
    wampClientFactory = WampClientFactory("ws://localhost:9000")
    wampClientFactory.protocol = TSClient
    connectWS(wampClientFactory)

    reactor.run()
