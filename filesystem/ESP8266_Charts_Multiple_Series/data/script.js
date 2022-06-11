// Complete project details: https://randomnerdtutorials.com/esp8266-nodemcu-plot-readings-charts-multiple/


  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;

  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
	websocket.binaryType = 'arraybuffer';
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
   
   var intArray;
  //https://stackoverflow.com/questions/42464569/javascript-convert-blob-to-float32array-or-other-typed-arrays
  function onMessage(event) {
	if (event.data instanceof ArrayBuffer) {
		intArray = new Int16Array(event.data);
		for (var i = 0; i < (intArray.length ); i++){
			plotTemperatureSingle(intArray[i]);
			//console.log("plot: " + String(intArray[i]));
		}
		
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
  legend: {
          enabled: false
  },
   tooltip: {
          enabled: false
  },
   exporting: {
		buttons: {
			contextButton: {
				menuItems: ["viewFullscreen", "downloadPNG", "downloadJPEG"]
			}
		}
	},
  xAxis: {
    type: 'linear',
	//tickPixelInterval: 150
	min:0,
	max:150,
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
	min: 400,
	max: 1000,
    title: {
      text: '&hearts;'
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
  chartT.xAxis[0].setExtremes(counter - 150,counter);
  var x = counter;
  var y = Number(valueString);
	if(counter > 150) {
    //  counter = 0;
    //  x = counter;	
	  chartT.series[0].addPoint([x, y], true, true, true);
	  
    } else {
      chartT.series[0].addPoint([x, y], true, false, true);
	  //chartT.series.addPoint([x, y], true, true, true);
    }
}




  

