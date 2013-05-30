#!/usr/bin/env python

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

log.startLogging(sys.stdout)


class TSPublisher(WampServerProtocol):

    def onSessionOpen(self):
        ## register a URI and all URIs having the string as prefix as PubSub topic
        self.registerForPubSub("http://coding-reality.de/tsd-event", True)


class TSClient(WampClientProtocol):
    #TODO: Embedd serialClient
    def onSessionOpen(self):
        print "Open Session"
        self.sendTSDEvent(random.random())

    def sendTSDEvent(self, event):
        self.publish("http://coding-reality.de/tsd-event", event)
        reactor.callLater(0.1, self.sendTSDEvent, random.random())


class SerialClient(Protocol):
    def connectionFailed(self):
        log.err("Connection failed")
        reactor.stop()

    def connectionMade(self):
        log.msg("Connected to MCP")
        self.sendCMD('GET')

    def sendCMD(self, cmd):
        if not cmd in ['GET']:
            raise Exception("UNKNOWN COMMAND")
        self.transport.write(cmd)

    def dataReceived(self, data):
        log.msg("dataReceived")
        data_ = ""
        for val in data:
            data_ += "%s : " % ord(val)
        log.msg(data_)
        reactor.callLater(0, self.sendCMD, 'GET')

    def lineReceived(self, line):
        log.msg("Line Received")
        log.msg(line)


class SerialComm(object):
    pass


class TSD(object):
    pass


class Device(object):
    pass


if __name__ == '__main__':
    #SerialPort(SerialClient(),'COM4', reactor, baudrate='9600')
    ##Setup Server
    wampServerFactory = WampServerFactory("ws://localhost:9000")
    wampServerFactory.protocol = TSPublisher
    listenWS(wampServerFactory)

    ##Setup Client
    wampClientFactory = WampClientFactory("ws://localhost:9000")
    wampClientFactory.protocol = TSClient
    connectWS(wampClientFactory)

    reactor.run()
