graph1Data = new TimeSeries
class Wamp
  constructor: () ->
    ab.connect("ws://localhost:9000",
      (session) =>
        #TODO: subscribe on connect
        console.log "Connected"
        @session = session
        @session.subscribe("http://coding-reality.de/tsd-event", this.onEvent)
      ,
      (code, reason) =>
        console.log "Fail: #{code}, #{reason}"
      )

  onEvent: (topicUri, event) ->
    console.log topicUri
    console.log event
    graph1Data.append(new Date().getTime(), event);

setup = -> 
  graph1 = new SmoothieChart
  graph1.streamTo document.getElementById("graph1"), 100

  setInterval(( ->
    ), 100)

  graph1.addTimeSeries(graph1Data)

window.onload = ->
  wamp = new Wamp
  setup()
