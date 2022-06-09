// Complete project details: https://randomnerdtutorials.com/esp8266-nodemcu-plot-readings-charts-multiple/


  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;

  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
	websocket.send('startstream');
  }

  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  
  function onMessage(event) {
	var state;
    console.log("Received: " + event.data);
	if (Number.isInteger(Number(event.data))){
      plotTemperatureSingle(event.data);
    }
  }
  
  window.addEventListener('load', onLoad);
  function onLoad(event) {
    //initWebSocket();
	setTimeout(initWebSocket, 1000);
  }

// Get current sensor readings when the page loads
//window.addEventListener('load', getReadings);
var counter = 0;
// Create Temperature Chart
var chartT = new Highcharts.Chart({
  chart:{
	zoomType: 'x',
    renderTo:'chart-heartecg',
	//animation: Highcharts.svg, // don't animate in old IE
	type: 'spline',
	//type: 'line',
	//marginRight: 10,
  },
  series: [
    {
      name: 'Heart',
	  lineWidth: 1,
	  marker: {
            enabled: false
        },
      //type: 'line',
	  //type: 'spline',
      color: '#101D42',
      //marker: {
      //  symbol: 'circle',
      //  radius: 3,
      //  fillColor: '#101D42',
      //}
    },
  ],
  title: {
    text: 'Heart'
  },
  xAxis: {
    type: 'linear',
	//tickPixelInterval: 150
	min:0,
	max:200,
    //dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  plotOptions: {
    line: {
        marker: {
            enabled: false
        }
    }
  },
  yAxis: {
	//min: 450,
	//max: 650,
    title: {
      text: 'Measure'
    }
  },
  credits: {
    enabled: false
  }
});


//Plot temperature in the temperature chart
function plotTemperatureSingle(valueString) {
  //console.log(valueString);
  counter++;
  chartT.xAxis[0].setExtremes(counter - 200,counter);
  var x = counter;
  var y = Number(valueString);
	if(counter > 200) {
    //  counter = 0;
    //  x = counter;	
	  chartT.series[0].addPoint([x, y], true, true, true);
	  
    } else {
      chartT.series[0].addPoint([x, y], true, false, true);
	  //chartT.series.addPoint([x, y], true, true, true);
    }
}


//Plot temperature in the temperature chart
function plotTemperature(jsonValue) {

  var keys = Object.keys(jsonValue);
  console.log(keys);
  console.log(keys.length);

  for (var i = 0; i < keys.length; i++){
    var x = (new Date()).getTime();
    console.log(x);
    const key = keys[i];
    var y = Number(jsonValue[key]);
    console.log(y);

    if(chartT.series[i].data.length > 40) {
      chartT.series[i].addPoint([x, y], true, true, true);
    } else {
      chartT.series[i].addPoint([x, y], true, false, true);
    }

  }
}



  

