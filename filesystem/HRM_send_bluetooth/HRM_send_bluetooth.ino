
// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3;


// analogue pin
#define analogPin A0 /* ESP8266 Analog Pin ADC0 = A0 */
int adcValue = 0;  /* Variable to store Output of ADC */
int maxThisWindow = 0;
float average = 512.0;
int sampleCount = 0;
int subsample = 4;
bool streaming = true;

// send out data via serial
void reportSerial() {
  Serial.println(String(maxThisWindow));// + "," + String(millis()));  
}
void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
}

void loop() { 
  if ((millis() - lastTime) > timerDelay) {
    
    if (streaming) {
      // Send Events to the client with the Sensor Readings Every timerDelay milliseconds
      // if client connected??
      adcValue = analogRead(analogPin); /* Read the Analog Input value */  
      average = average + adcValue;
      //Serial.println(String(adcValue));
      maxThisWindow = max(adcValue, maxThisWindow); 
  
      // only send out after 
      if (sampleCount > subsample) {
        Serial.println(String(average/subsample));
        sampleCount = 0;
        average = 512;
        maxThisWindow = 0;
      }
      sampleCount = sampleCount + 1;
    }
    
    lastTime = millis();
    
  }
}
