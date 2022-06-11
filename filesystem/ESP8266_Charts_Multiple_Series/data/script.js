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
    setTimeout(initWebSocket, 3000);
  }

	function buf2hex(buffer) { // buffer is an ArrayBuffer
	  return [...new Uint8Array(buffer)]
		  .map(x => x.toString(16).padStart(2, '0'))
		  .join('');
	}

   
   var intArray;
  //https://stackoverflow.com/questions/42464569/javascript-convert-blob-to-float32array-or-other-typed-arrays
  function onMessage(event) {
	if (event.data instanceof ArrayBuffer) {
		intArray = new Uint32Array(event.data);
		// output hex of byte string for debugging
		//console.log(buf2hex(event.data));
		for (var i = 0; i < (intArray.length / 2); i++){
			plotTemperatureSingle(intArray[i*2], intArray[(i*2)+1]);
			//console.log("plot - timestamp: " + String(intArray[i*2]) + " value: " + String(intArray[(i*2)+1]));
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
	softMin: 400,
	softMax: 1000,
    title: {
      text: '&hearts;'
    }
  },
  credits: {
    enabled: false
  }
});


//Plot temperature in the temperature chart
function plotTemperatureSingle(timestamp, value) {
  var x = timestamp;
  var y = value;
  // window 2 seconds	
  chartT.xAxis[0].setExtremes(timestamp - 3000, timestamp);
	if(chartT.series[0].data.length > 200) {
    //  counter = 0;
    //  x = counter;	
	  chartT.series[0].addPoint([x, y], true, true, true);
	  
    } else {
      chartT.series[0].addPoint([x, y], true, false, true);
	  //chartT.series.addPoint([x, y], true, true, true);
    }
	
 
}




  

