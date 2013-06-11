graphData = []
for i in [1..10]
  graphData.push(new TimeSeries)

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
    graphData[event.id-1].append(event.timestamp, event.data)

setup = ->
  graphs = []
  for id in [1..10]
    graph = new SmoothieChart
    graph.streamTo document.getElementById("graph_#{id}"), 100
    graph.addTimeSeries(graphData[id])

window.onload = ->
  wamp = new Wamp
  setup()
