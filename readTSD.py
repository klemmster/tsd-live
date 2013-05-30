#!/usr/bin/env python

import sys

import serial

from autobahn.websocket import listenWS
from autobahn.wamp import WampServerFactory
from autobahn.wamp import WampServerProtocol
from twisted.internet import reactor
from twisted.internet.serialport import SerialPort
from twisted.internet.protocol import Protocol, Factory
from twisted.web.server import Site
from twisted.web.static import File
from twisted.python import log

log.startLogging(sys.stdout)

class TSPublisher(WampServerProtocol)

	def onSessionOpen(self):
	    ## register a URI and all URIs having the string as prefix as PubSub topic
		self.registerForPubSub("http://example.com/event#", True)

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
	
class TSD(object):
	pass
	
class Device(object):
	pass
	
if __name__ == '__main__':
	SerialPort(SerialClient(),'COM4', reactor, baudrate='9600')
	reactor.run()