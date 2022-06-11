#include <AsyncPrinter.h>
#include <async_config.h>
#include <DebugPrintMacros.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncTCPbuffer.h>
#include <SyncClient.h>
#include <tcp_axtls.h>

/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp8266-nodemcu-plot-readings-charts-multiple/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#if !defined(ESP8266)
  #error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif

// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         2
#define _TIMERINTERRUPT_LOGLEVEL_     0

// Select a Timer Clock
#define USING_TIM_DIV1                false           // for shortest and most accurate timer
#define USING_TIM_DIV16               true           // for medium time and medium accurate timer
#define USING_TIM_DIV256              false            // for unsigned longest timer but least accurate. Default

#include "ESP8266TimerInterrupt.h"

// stuff for timer interupt sampling
#define TIMER_INTERVAL_MS         3

// Init ESP8266 timer 1
ESP8266Timer ITimer;

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>

// Replace with your network credentials
const char* ssid = "tacticalspace";
const char* password = "uranium232";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");




/* Create a double buffer for data samples. */
//#define NUMBER_OF_DATA_SAMPLES 256
#define NUMBER_OF_DATA_SAMPLES 32
#define SAMPLE_AVERAGE 4 // must be a div of samples
unsigned long data_samples[ NUMBER_OF_DATA_SAMPLES * 2 ];
unsigned long sample_timestamps[ NUMBER_OF_DATA_SAMPLES * 2 ];

// make sending variable for byte string
byte dataToSend[ NUMBER_OF_DATA_SAMPLES * 8 ];
unsigned long *data_samples_real;
unsigned long *sample_timestamps_real;
/* Indexes used by the interrupt service routine. */
int  isr_current_data_index;
/* Semaphor indicating that a frame of geophone samples is ready. */
bool data_buffer_full;
/* Flag that indicates that a report with amplitude information was
   created.  It is used by the report LED blinking. */
bool report_was_created;

// analogue pin
#define analogPin A0 /* ESP8266 Analog Pin ADC0 = A0 */
int connections = 0;



 
// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

// reply with led state --- we won't use this
// changed to analog pin
void notifyClients() {
//  ws.textAll(String(adcValue));
//    ws.textAll(String(maxThisWindow));
}


/*
 * Interrupt service routine for sampling the data.  The data analog
 * pin is sampled at each invokation of the ISR.  If the buffer is full, a
 * pointer is passed to the main program and a semaphor is raised to indicate
 * that a new frame of samples is available, and future samples are written
 * to the other buffer.
 *
 * Inspired by https://github.com/olewolf/geophone/blob/master/Geosampler.ino
 */
void IRAM_ATTR TimerHandler()
{
  if (connections > 0) {
    /* Read a sample and store it in the data buffer. */  
    int readValue = analogRead( analogPin );
    unsigned long data_sample = (unsigned long) readValue;
    unsigned long sample_timestamp = millis();
    
    data_samples[ isr_current_data_index ] = data_sample;
    // timestamp record too
    sample_timestamps[ isr_current_data_index ] = sample_timestamp;
    
    isr_current_data_index++;
  
    /* Raise a semaphor if the buffer is full and tell which buffer
       is active. */
    if( isr_current_data_index == NUMBER_OF_DATA_SAMPLES )
    {
      data_samples_real     = &data_samples[ 0 ];
      sample_timestamps_real = &sample_timestamps[ 0 ];
      data_buffer_full      = true;
    }
    else if( isr_current_data_index == NUMBER_OF_DATA_SAMPLES * 2 )
    {
      data_samples_real      = &data_samples[ NUMBER_OF_DATA_SAMPLES ];
      sample_timestamps_real = &sample_timestamps[ NUMBER_OF_DATA_SAMPLES ];
      isr_current_data_index = 0;
      data_buffer_full       = true;
    }
  }
}

/**
 * Setup the timer interrupt and prepare the geodata sample buffers for
 * periodic sampling.  
 *
 * This function is board specific; if other board than the Arduino Mega
 * or the Arduino Due are used the code must be updated.
 */
void start_sampling( )
{
  /* Prepare the buffer for sampling. */
  isr_current_data_index = 0;
  data_buffer_full       = false;
  Serial.print(F("\nStarting sampling on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP8266_TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));

  // Interval in microsecs
  if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print(F("Starting ITimer OK, millis() = ")); Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer. Select another freq. or timer"));
    
  report_was_created = false;
 
}



/**
 * Send the samples in the most recent buffer over the serial port.
 *
 * @param [in] freq_real Array of samples.
 * @param [in] length Number of samples.
 * 
 * Inspired by https://github.com/olewolf/geophone/blob/master/Geosampler.ino
 */
void report( const unsigned long *samples, const unsigned long *timestamps, int length )
{
  
  /* Send all the samples in the buffer to the serial port. */
  int entryCount = 0;
  for( int index = 0; index < length; index = index + SAMPLE_AVERAGE )
  {
     unsigned long myValue = 0;
     unsigned long myTimestamp = timestamps[ index ];
    // unsigned long into 4 bytes https://forum.arduino.cc/t/how-to-convert-long-int-to-byte/42270
    // timestamp

    for (int averageIndex = 0; averageIndex < SAMPLE_AVERAGE; averageIndex++) {
      myValue = max(myValue, samples[ index + averageIndex]);
    }
        
    // little endian
    dataToSend[(entryCount * 8) + 0] = myTimestamp;
    dataToSend[(entryCount * 8) + 1] = myTimestamp >> 8;
    dataToSend[(entryCount * 8) + 2] = myTimestamp >> 16;
    dataToSend[(entryCount * 8) + 3] = myTimestamp >> 24;
    dataToSend[(entryCount * 8) + 4] = myValue;
    dataToSend[(entryCount * 8) + 5] = myValue >> 8;
    dataToSend[(entryCount * 8) + 6] = myValue >> 16;
    dataToSend[(entryCount * 8) + 7] = myValue >> 24;

    entryCount++;
   }
  ws.binaryAll(dataToSend, ((length * 8) / SAMPLE_AVERAGE));
  /* Indicate to the report LED blinking that the report was submitted. */
  report_was_created = true;
}







// as titled -- also change
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  //AwsFrameInfo *info = (AwsFrameInfo*)arg;
  //Serial.print("received WS message");
  //if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;
    //if (strcmp((char*)data, "startstream") == 0) {
      //ledState = !ledState;
      //streaming = true;
      //notifyClients();
    //}
  //}
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        connections++;
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        connections--; 
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initWebSocket();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();

  /* Setup the data sampling buffers and sampling interrupt. */
  start_sampling( );
}

void loop() {
  ws.cleanupClients();

  /* Analyze the ECG output data once it's available. */
  if( data_buffer_full == true )
  {
    data_buffer_full = false;
    if (connections > 0) {
      /* Transmit the samples over websocket. */
      report( data_samples_real, sample_timestamps_real, NUMBER_OF_DATA_SAMPLES );
    }
  }
}
