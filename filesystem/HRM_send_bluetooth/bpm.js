document.addEventListener("DOMContentLoaded", async function () {
    let chart = Highcharts.chart('container', {
        chart: { type: 'line', animation: false },
        title: { text: 'Heart Monitor Signal' },
        xAxis: { type: 'datetime' },
        yAxis: { title: { text: 'Signal Value' }, min: 500, max: 1500 },
        series: [{ name: 'Heart Signal', data: [] }]
    });
    
    let port;
    let reader;
    let lastPeakTime = null;
    let bpm = 0;
    
    async function connectSerial() {
        try {
            port = await navigator.serial.requestPort();
            await port.open({ baudRate: 9600 });
            reader = port.readable.getReader();
            readSerialData();
        } catch (error) {
            console.error('Serial connection error:', error);
        }
    }
    
    async function readSerialData() {
        let decoder = new TextDecoderStream();
        let inputStream = port.readable.pipeThrough(decoder);
        let lineReader = inputStream.getReader();
    
        while (true) {
            const { value, done } = await lineReader.read();
            if (done) break;
            if (value) processSerialData(value.trim());
        }
    }
    
    function processSerialData(value) {
        let signal = parseInt(value, 10);
        if (isNaN(signal) || signal < 500 || signal > 1500) return;
        
        let now = Date.now();
        
        // Detect peaks crossing 1000
        if (signal > 1000 && lastPeakTime) {
            let timeDiff = (now - lastPeakTime) / 1000; // Convert to seconds
            bpm = Math.round(60 / timeDiff);
            lastPeakTime = now;
        } else if (signal > 1000) {
            lastPeakTime = now;
        }
        
        chart.series[0].addPoint([now, signal], true, chart.series[0].data.length > 100);
        document.getElementById('bpm').innerText = `BPM: ${bpm}`;
    }
    
    document.getElementById('connect').addEventListener('click', connectSerial);
});
