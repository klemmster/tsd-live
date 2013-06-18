// Generated by CoffeeScript 1.6.1
(function() {
  var Wamp, graphData, i, setup, _i;

  graphData = [];

  for (i = _i = 1; _i <= 9; i = ++_i) {
    graphData.push(new TimeSeries);
  }

  Wamp = (function() {

    function Wamp() {
      var _this = this;
      ab.connect("ws://localhost:9000", function(session) {
        console.log("Connected");
        _this.session = session;
        return _this.session.subscribe("http://coding-reality.de/tsd-event", _this.onEvent);
      }, function(code, reason) {
        return console.log("Fail: " + code + ", " + reason);
      });
    }

    Wamp.prototype.onEvent = function(topicUri, event) {
      //console.log(event);
      return graphData[event.id - 1].append(new Date().getTime(), event.data);
    };

    return Wamp;

  })();

  setup = function() {
    var graph, id, _j, _results;
    _results = [];
    for (id = _j = 1; _j <= 9; id = ++_j) {
      graph = new SmoothieChart;
      graph.streamTo(document.getElementById("graph_" + id), 100);
      _results.push(graph.addTimeSeries(graphData[id-1]));
    }
    return _results;
  };

  window.onload = function() {
    var wamp;
    wamp = new Wamp;
    return setup();
  };

}).call(this);
