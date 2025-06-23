#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h> // Include the OTA library
//#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//#include <ArduinoJson.h>

AsyncWebServer server(80);  // Declare server globally
String serialOutput;  // Declare serialOutput globally

// Update these with your WiFi and MQTT broker settings
const char* ssid = "24Gigabit";
const char* password = "1234qwer";
const char* mqtt_server = "192.168.1.94";
const int mqtt_port = 1883;
const char* mqtt_user = "mqtt8266";  // MQTT username
const char* mqtt_password = "1234qwer"; // MQTT password

const unsigned long onTime = 500;  // Duration to keep the LED on in milliseconds
unsigned long previousMillis2 = 0; // Store the last time the LED was turned on
unsigned long previousMillisTimerCheck = 0;
bool ledOn = false;           // Store the current LED state
bool HAinControl = false;
bool pool_pump_serial_monitor = true;

long intervalLedFlash = 200;  // Interval at which to blink (milliseconds)
unsigned long previousMillis = 0;  // Will store the last time LED was updated
int ledState = LOW;  // ledState used to set the LED

String filterStatus = "auto";
String filterCommand = "stop";
String filterHighLow = "low";
String output = "";

// Initialize time strings to "00:00"
long starttime1 = 0;
long stoptime1 = 0;
long starttime2 = 0;
long stoptime2 = 0;

bool vfdReady = false;
unsigned long vfdStartTime = 0;
bool vfdStarting = false;

// Define NTP Client to get time
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -14400; // Adjust this to your local timezone
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 600000);

unsigned long lastNtpUpdate = 0;
const unsigned long ntpUpdateInterval = 60000;  // Update every 10 minutes
unsigned long lastEpochTime = 0;
unsigned long lastMillis = 0;
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 5000;  // Print every 5 seconds
const unsigned long intervalTimerCheck = 10000; // 10 seconds
int countdownHours = 2;
int test2 = 2;
int countdownMinutes = 0;

int countdownDisabled = 0;

String hoursString = "11";
String minutesString = "22";

unsigned long timerDuration = (countdownHours * 60 + countdownMinutes) * 60 * 1000; // Convert hours and minutes to milliseconds
unsigned long countdownTime = 0;
bool timerActive = false;

String firstPart = "";
String secondPart = "";

String publishString ="";
int minutesToStart = 0;
int currentTime24hourFormat = 0;


const int MAX_EVENTS = 10;
String eventTimes[MAX_EVENTS];
String eventLog[MAX_EVENTS];  // <-- declare this globally
int eventIndex = 0;

int previousState = HIGH;
int inputValue = HIGH;
unsigned long transitionCount = 0;

int previousStateInput = LOW;
int debouncedState = LOW; // <== Use this in the rest of your code
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 100;
int transitionCountInput = 0;

unsigned long lastLowDuration = 0;
unsigned long lastLongLowDuration = 0; 

const int inputPin = 4;  // D2 (GPIO 4)
const int outputLowPin = 14;  // D5 (GPIO 14)
const int outputHighPin = 12; // D6 (GPIO 12)
const int powerKillPin = 13;  // D7 (GPIO 13)
const int Relay_4 = 5;      // D1 (GPIO 5)

// Mode and State Variables
bool isLowSpeed = false;
bool isHighSpeed = false;
bool toggleState = false;
bool toggledOnce = false;  // Flag to track if toggle has occurred
bool lastInputState = LOW;  // Track the last state of the input pin
unsigned long lastChangeTime = 0;
unsigned long lastInputTime = 0;
unsigned long lastInputTimeHighLowToggle = 0;
unsigned long powerKillTime = 0;
unsigned long cnt1 = 0;
//bool HighLowMQTTlock = false;
bool isAuto = true;
bool oneShot = false;
unsigned long LOWtoHIGHorHIGHtoLOW_Counter = 0;
unsigned long isAutoCounter = 0;
unsigned long currentInputState_lastInputState_Count = 0;
//RTC_DS3231 rtc;  // Initialize RTC object

// Define the timer values (in 24-hour format)
int start_time_1 = 1200;
int stop_time_1 = 1300;
int start_time_2 = 800;
int stop_time_2 = 1700;

// Define variables to indicate if the timers are active
bool timerOneActive = false;
bool timerTwoActive = false;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define LED_PIN 2  // Built-in LED on ESP-12E module
int counter = 0;

unsigned long lastPublishTime = 0;   // global variable to store last publish time
const unsigned long publishInterval = 10000;  // 10 seconds in milliseconds

///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  pinMode(16, OUTPUT);  // Initialize GPIO pin 16 as an output Onboard LED
  pinMode(LED_PIN, OUTPUT); //Onboard LED

  pinMode(inputPin, INPUT_PULLUP);
  pinMode(outputLowPin, OUTPUT);
  pinMode(outputHighPin, OUTPUT);
  pinMode(powerKillPin, OUTPUT);
  pinMode(Relay_4, OUTPUT);

  // Set all pins to LOW
  digitalWrite(outputLowPin, HIGH);
  digitalWrite(outputHighPin, HIGH);
  digitalWrite(powerKillPin, HIGH);
  digitalWrite(Relay_4, HIGH);

  testLED();

  toggleState = 1;

  Serial.begin(115200);
  
  // Setup WiFi
  setup_wifi();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><title>Pool Filter ESP8266 Serial Monitor</title>";
    html += "<style>";
    html += "body, html { margin: 0; padding: 0; height: 100%; overflow: hidden; }";
    html += "#container { display: flex; flex-direction: column; height: 100%; }";
    html += "#output { flex-grow: 1; overflow-y: scroll; }";
    html += "</style></head>";
    html += "<body><div id='container'>";
    html += "<h1>Pool Filter ESP8266 Serial Monitor</h1>";
    html += "<button onclick='clearScreen()'>Clear Screen</button>";
    html += "<pre id='output'></pre>";
    html += "</div>";
    html += "<script>";
    html += "function clearScreen() {";
    html += "  document.getElementById('output').innerHTML = '';";
    html += "}";
    html += "function scrollToBottom() {";
    html += "  var output = document.getElementById('output');";
    html += "  output.scrollTop = output.scrollHeight;";
    html += "}";
    html += "setInterval(function(){";
    html += "  fetch('/serial').then(response => response.text()).then(data => {";
    html += "    document.getElementById('output').innerHTML += data;";
    html += "    scrollToBottom();";  // Scroll to bottom after adding new data
    html += "  });";
    html += "}, 1000);";
    html += "</script></body></html>";
    request->send(200, "text/html", html);
  });
  // Route to fetch serial data
  server.on("/serial", HTTP_GET, [](AsyncWebServerRequest *request){
    output = serialOutput;
    serialOutput = ""; // Clear buffer after sending
    request->send(200, "text/plain", output);
  });




  server.begin();
  printlnEx("HTTP server started");
  // Setup OTA
  ArduinoOTA.begin();
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Set MQTT callback function
  reconnect(); // Attempt MQTT connection
  
  // Subscribe to MQTT topics
  client.subscribe("home/commands");

  // Read values from EEPROM
  readFromEEPROM(starttime1, stoptime1, starttime2, stoptime2);
  
  // Initialize NTPClient
  timeClient.begin();
  if (timeClient.update()) {
    lastEpochTime = timeClient.getEpochTime();
    lastMillis = millis();
  }
  lastInputState =  !digitalRead(inputPin);  // read ONCE and invert
  inputValue = lastInputState;
  previousState = lastInputState;
  previousStateInput = lastInputState;
  debouncedState = lastInputState;

  isAuto = true;
  HAinControl = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup_wifi() {
  delay(10);
  // Connect to WiFi
  printlnEx("");
  printEx(F("Connecting to "));
  printlnEx(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    printEx(F("."));
  }

  printlnEx("");
  printlnEx(F("WiFi connected"));
  printEx(F("Local IP address: ")); 
  printlnEx(WiFi.localIP().toString());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Call this to reset all logged times
void resetEventLog() {
  for (int i = 0; i < MAX_EVENTS; i++) {
    eventTimes[i] = "";
  }
  eventIndex = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle message arrived
  printEx(F("Message received on topic: "));
  printlnEx(topic);



  // Convert payload to string
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Print message payload
  printEx("Message payload: ");
  printlnEx(message);

  // Example: Toggle LED based on incoming message
  if (strcmp(topic, "home/commands") == 0) {
    int value = message.toInt();
    printEx("Message = ");
    printlnEx(message);
    // printEx("Value = ");
    // printlnEx(value);
    // if (value > 99 && value < 10001) {
    //   ledState = !ledState; // Toggle LED state
    //   interval = value; 

    if (message.startsWith("AutoDisableStatus:")) {
      String status = message.substring(strlen("AutoDisableStatus:"));
      status.trim();

      if (status == "on") {
        countdownDisabled = 1;
        digitalWrite(Relay_4, LOW);  // Turn relay ON
      } else if (status == "off") {
        countdownDisabled = 0;
        digitalWrite(Relay_4, HIGH);   // Turn relay OFF
      }
    }

    if (message == "start")  {
      filterCommand = "start";
      client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
    }
    if (message == "stop")  {
      filterCommand = "stop";
      client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
    }
    if (message == "run")  {
      filterCommand = "run";
      client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
    }  
    
    
    if (message == "auto")  {
      filterStatus = "auto";
      resetEventLog();
      client.publish("home/sensor/filterStatus", String(filterStatus).c_str());
      //client.publish("home/sensor/timer/countdown", "00:00:00");
      isAuto = true;
    }      
    if (message == "manual")  {
      filterStatus = "manual";
      client.publish("home/sensor/filterStatus", String(filterStatus).c_str());
      //client.publish("home/sensor/timer/countdown", "00:00:00");
      countdownDisabled - 0;
      isAuto = false;
      //delay(500);
    }
    

    if (message == "high")  {
      filterHighLow = "high";
      client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str());
    }      
    if (message == "low")  {
      filterHighLow = "low";
      client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str());
    }
    if (message == "writetimerstomemory")  {
      printlnEx(F("Writing to EEPROM..........................."));
      writeToEEPROM(starttime1, stoptime1, starttime2, stoptime2);  
      message = "";
      printlnEx("LED TEST...");

      //testLED();
        // Set each pin to HIGH with a delay in between
      digitalWrite(Relay_4, LOW);
      delay(200);  // Delay for 200 milliseconds
      digitalWrite(Relay_4, HIGH);
      delay(200);  // Delay for 200 milliseconds
      digitalWrite(Relay_4, LOW);
      delay(200);  // Delay for 200 milliseconds
      digitalWrite(Relay_4, HIGH);
      
      printlnEx(F("writeToEEPROM(starttime1, stoptime1, starttime2, stoptime2);"));
      printlnEx("EEPROM Writing Complete!");
      printlnEx("Pausing for 5 sec...");
      delay(1000);
    }
      
    // Process the message with 'time'
    if (message.indexOf("time") != -1) {
      int commaPos = message.indexOf(',');
      if (commaPos != -1) {
        firstPart = message.substring(0, commaPos);
        secondPart = message.substring(commaPos + 1);
        printlnEx(firstPart);
        printlnEx(secondPart);

        // Set the appropriate time variable based on the secondPart
        if (secondPart == "start_time_1") {
            starttime1 = parseTimeStringToInt(firstPart);
            printlnEx("Publish: " + firstPart + " to: " + "home/pool_pump/start_time_1");
            client.publish("home/pool_pump/start_time_1", firstPart.c_str());  
        } else if (secondPart == "stop_time_1") {
            stoptime1 = parseTimeStringToInt(firstPart);
            printlnEx("Publish: " + firstPart + " to: " + "home/pool_pump/stop_time_1");
            client.publish("home/pool_pump/stop_time_1", firstPart.c_str());        
        } else if (secondPart == "start_time_2") {
            starttime2 = parseTimeStringToInt(firstPart);
            printlnEx("Publish: " + firstPart + " to: " + "home/pool_pump/start_time_2");
            client.publish("home/pool_pump/start_time_2", firstPart.c_str());        
        } else if (secondPart == "stop_time_2") {
            stoptime2 = parseTimeStringToInt(firstPart);
            printlnEx("Publish: " + firstPart + " to: " + "home/pool_pump/stop_time_2");
            client.publish("home/pool_pump/stop_time_2", firstPart.c_str());           
        } else if (secondPart == "manual_run_time") {
              // Find the positions of the colons
            int firstColon = firstPart.indexOf(':');
            int secondColon = firstPart.indexOf(':', firstColon + 1);

            // Extract hours and minutes substrings
            hoursString = firstPart.substring(0, firstColon);
            minutesString = firstPart.substring(firstColon + 1, secondColon);

            // Convert substrings to integers
            countdownHours = hoursString.toInt();
            countdownMinutes = minutesString.toInt();
            timerDuration = (countdownHours * 60 + countdownMinutes) * 60 * 1000; // Convert hours and minutes to milliseconds
            countdownTime = timerDuration;
            //printlnEx("Publish: " + firstPart + " to: " + "home/pool_pump/stop_time_2");
            printEx("Count Down Hours: ");
            printlnEx(countdownHours);
            printEx("Count Down Minutes: ");
            printlnEx(countdownMinutes);
            //delay(3000);
            //client.publish("home/pool_pump/stop_time_2", firstPart.c_str());    
        }
        //clearSerialMonitor();
        // Print the updated times
        printlnEx("Updated times:");
        printEx("starttime1: ");
        printlnEx(starttime1);
        printEx("stoptime1: ");
        printlnEx(stoptime1);
        printEx("starttime2: ");
        printlnEx(starttime2);
        printEx("stoptime2: ");
        printlnEx(stoptime2);
        
        // Write initial values to EEPROM
        

      } else {
        printlnEx(F("No comma found in the string."));
      }
    } else {
      printlnEx(F("The string does not contain 'time'."));
    }

    if (message.indexOf("pool_pump_serial_monitor") != -1) {
      int commaPos = message.indexOf(',');
      if (commaPos != -1) {
        firstPart = message.substring(0, commaPos);
        secondPart = message.substring(commaPos + 1);
        printlnEx(firstPart);
        printlnEx(secondPart);
        if(firstPart == "on" ){
          printlnEx("Serial Monitor On");
          pool_pump_serial_monitor = true;
        }
        if(firstPart == "off" ){
          serialOutput = "";
          output = "";
          printlnEx("Serial Monitor Off");
          pool_pump_serial_monitor = false;
         
        }        
      }

    
    }
    if (message == "start" || message == "stop" || message == "run" || message == "auto" || message == "manual" || message == "high" || message == "low")  {
      HAinControl = true;
      printEx("Home Assistant Has Control: ");
      printlnEx("True");
      if (message != "auto" && message != "manual" )  {
        filterStatus = "manual";
        client.publish("home/sensor/filterStatus", String(filterStatus).c_str());
        isAuto = false;
        printlnEx("isAuto is Off");
      } 
      message = "";
    }

    digitalWrite(LED_PIN, HIGH);  // Turn the LED on
    delay(50);                   // Wait for 500ms
    digitalWrite(LED_PIN, LOW);   // Turn the LED off
    delay(50); 
    digitalWrite(LED_PIN, HIGH);  // Turn the LED on
    delay(50);                   // Wait for 500ms
    digitalWrite(LED_PIN, LOW);   // Turn the LED off
    delay(50); 
    digitalWrite(LED_PIN, HIGH);  // Turn the LED on
    delay(50);                   // Wait for 500ms
    digitalWrite(LED_PIN, LOW);   // Turn the LED off
    delay(50);  
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void reconnect() {

    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      printlnEx("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("home/commands");
    } else {
      printEx("failed, rc=");
      printlnEx(client.state());
      //printlnEx(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      ////delay(5000);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Call this to print the log
void printEventLog() {
  printlnEx("Logged Timestamps:");
  for (int i = 0; i < eventIndex; i++) {
    printEx("Event ");
    printEx(i);
    printEx(": ");
    printlnEx(eventTimes[i]);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void logEventTime(const String& label) {
  if (eventIndex < MAX_EVENTS) {
    String timestamp = 
      (millis() / 3600000 < 10 ? "0" : "") + String(millis() / 3600000) + ":" +
      ((millis() / 60000) % 60 < 10 ? "0" : "") + String((millis() / 60000) % 60) + ":" +
      ((millis() / 1000) % 60 < 10 ? "0" : "") + String((millis() / 1000) % 60);

    eventTimes[eventIndex++] = label + " @ " + timestamp;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int timeToMinutes(int hhmm) {
  int hours = hhmm / 100;
  int minutes = hhmm % 100;
  return hours * 60 + minutes;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to parse "HH:MM" time strings into an integer representation (e.g., 1345 for "13:45")
int parseTimeStringToInt(const String& timeStr) {
    int hours = timeStr.substring(0, 2).toInt();
    int minutes = timeStr.substring(3, 5).toInt();
    return hours * 100 + minutes; // Combine hours and minutes into a single integer
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String convertTime(int time) {
  int hours = time / 100;
  int minutes = time % 100;
  char buffer[9];
  sprintf(buffer, "%02d:%02d:00", hours, minutes);
  return String(buffer);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void clearSerialMonitor() {
    for (int i = 0; i < 50; i++) {
        printlnEx("");
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////                       LOOP                             ///////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  unsigned long currentMillis2 = millis();

  // Handle OTA updates
  ArduinoOTA.handle();

  // Update NTP time every 10 seconds
  unsigned long currentMillis3 = millis();
  int wfs = WiFi.status();
  int cln = currentMillis3 - lastNtpUpdate;
  if (wfs == WL_CONNECTED && currentMillis3 - lastNtpUpdate >= ntpUpdateInterval) {
    printlnEx(" ");
    printEx(F("Update NTP time every 10 seconds"));
    if (timeClient.update()) {
      lastEpochTime = timeClient.getEpochTime();
      lastMillis = millis();
      lastNtpUpdate = currentMillis3;
      printlnEx(" ");
      printEx("lastEpochTime : ");
      printlnEx(lastEpochTime);
    }else{
     printlnEx(" ");
     printEx("timeClient.update Failed : ");
     lastNtpUpdate = currentMillis3; 
    }
  }
  
  // Read the current state of the input pin

  inputValue = !digitalRead(inputPin);  // read ONCE and invert
  unsigned long now2 = millis();

  // Raw counter for debugging
  if (inputValue != previousState) {
    transitionCount++;
    printEx("[RAW] Transition #");
    printlnEx(transitionCount);
    previousState = inputValue;
  }

  // Debounced counter (your trusted one)
  if (inputValue != previousStateInput) {
    lastDebounceTime = now2;             // reset timer if state changed
    previousStateInput = inputValue;     // track what we just saw
  }

  if ((now2 - lastDebounceTime) > debounceDelay && debouncedState != inputValue) {
    debouncedState = inputValue;
    logEventTime( "609");
    transitionCountInput++;
    printEx("[DEBOUNCED] Transition #");
    printlnEx(transitionCountInput);
  }


  // // Measure how long the pin stays LOW (timeout: 5 sec)
  // unsigned long lowDuration = pulseIn(inputPin, LOW, 5000000);  // timeout 5 sec

  // if (lowDuration > 0) {
  //   lastLowDuration = lowDuration;
  //   Serial.print("Pin went LOW for ");
  //   Serial.print(lastLowDuration);
  //   Serial.println(" microseconds");

  //   if (lowDuration >= 100000) {  // 3,000,000 us = 3000 ms = 3 seconds
  //     lastLongLowDuration = lowDuration;
  //     Serial.print("  >> This LOW pulse is longer than 3 seconds: ");
  //     Serial.print(lastLongLowDuration);
  //     Serial.println(" microseconds");
  //   }
  // }






  bool currentInputState = debouncedState;

  // Check for a change in state from LOW to HIGH or HIGH to LOW
  if (currentInputState != lastInputState) {
    // State has changed, trigger one-shot action
    printlnEx("                                                                          **** ONE SHOT ****");
    HAinControl = false;
    printEx("Home Assistant Has Control: ");
    printlnEx(HAinControl);
    filterStatus = "manual";
    LOWtoHIGHorHIGHtoLOW_Counter = LOWtoHIGHorHIGHtoLOW_Counter + 1;
    client.publish("home/sensor/filterStatus", String(filterStatus).c_str());
    isAuto = false;
    currentInputState_lastInputState_Count = currentInputState_lastInputState_Count + 1;
    printlnEx("isAuto is Off");
    // Update lastInputState to the current state
    lastInputState = currentInputState;
    //delay(5000);
  }



  // Calculate the current epoch time based on the last update
  unsigned long currentEpochTime = lastEpochTime + (currentMillis3 - lastMillis) / 1000;

  // Convert epoch time to hours and minutes
  int hours = (currentEpochTime % 86400L) / 3600;
  int minutes = (currentEpochTime % 3600) / 60;

  // Combine hours and minutes into a single integer
  int timeInt = hours * 100 + minutes;

  // Print the current time every 5 seconds
  if (currentMillis3 - lastPrintTime >= printInterval) {
    lastPrintTime = currentMillis3;  // Update the last print time
    
    if (WiFi.status() == WL_CONNECTED) {
      printEx("Online Local time (HHMM): ");
    } else {
      printEx("Offline time (HHMM): ");
    }
    printlnEx(timeInt);
    printlnEx(" ");
    printEx("WiFi.status : ");
    printlnEx(wfs);
    // printEx(currentMillis3);
    // printEx("   ");
    // printEx(lastNtpUpdate);
    // printEx("   ");
    // printlnEx(ntpUpdateInterval);

    //printEx("currentMillis3 - lastNtpUpdate >= ntpUpdateInterval = : ");
    //printlnEx(cln);  
  }
  

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  // Attempt MQTT reconnection if not connected
  if (!client.connected()) {
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 15000) { // Try to reconnect every 5 seconds
      lastReconnectAttempt = millis();
      printlnEx("Attempting MQTT connection...");
      reconnect();
      printlnEx(" try again in 5 seconds");
    }
  }
    
  


  client.loop();

  // Publish counter value and blink LED
  unsigned long now = millis();
  if (now - lastMsg > 20000) {
    lastMsg = now;
    ++counter;
    //clearSerialMonitor();
    printlnEx(" ");
    printEx("Publishing counter count: ");
    printlnEx(counter);
    printEx(F("LOWtoHIGHorHIGHtoLOW_Counter count: "));
    printlnEx(LOWtoHIGHorHIGHtoLOW_Counter);
    printEx("isAutoCounter count: ");
    printlnEx(isAutoCounter);
    printEx("currentInputState_lastInputState_Count count: ");
    printlnEx(currentInputState_lastInputState_Count);    
    printlnEx(" ");
    //printlnEx(filterStatus);
    //printlnEx(filterCommand);
    //printlnEx(filterHighLow);

    printEx("Transision in input state ............: ");
    printlnEx(transitionCount);
    printEx("Transition Debounced Input ############ : ");
    printlnEx(transitionCountInput);
    printEx("lastLowDuration ############ : ");
    printlnEx(lastLowDuration);   
    //printEx("lastLongLowDuration ############ : ");
    //printlnEx(lastLongLowDuration);  

    printEventLog();
     
    printlnEx(" ");

    if (client.connected()) {
      
      //printlnEx("Publishing to MQTT topics...");    

      client.publish("home/sensor/filterStatus", String(filterStatus).c_str());
      client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
      client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str());
      //String payload = "{\"time_left\": 3600}";  // Replace 3600 with your actual time left in seconds
      //client.publish("home/sensor/timer/countdown", payload.c_str());
      
      // String message = "Hello, world!";
      // char msgBuffer[message.length() + 1];
      // message.toCharArray(msgBuffer, message.length() + 1);
      // client.publish("home/display/text", msgBuffer);


      printlnEx("Publish: {" + filterStatus + "} to: " + "home/sensor/filterStatus");
      printlnEx("Publish: {" + filterCommand + "} to: " + "home/sensor/filterCommand");
      printlnEx("Publish: {" + filterHighLow + "} to: " + "home/sensor/filterHighLow");

      printlnEx(" ");

      printEx("Home Assistant Has Control: ");
      printlnEx(HAinControl);
      printlnEx(" ");

      // Convert times and publish them
      String s1 = convertTime(starttime1);
      String s2 = convertTime(stoptime1);
      String s3 = convertTime(starttime2);
      String s4 = convertTime(stoptime2);
      String s5 = convertTime((countdownHours * 100) + countdownMinutes);
      

      printlnEx("Publish: {" + s1 + "} to: " + "home/pool_pump/start_time_1");
      client.publish("home/pool_pump/start_time_1", s1.c_str());

      printlnEx("Publish: {" + s2 + "} to: " + "home/pool_pump/stop_time_1");
      client.publish("home/pool_pump/stop_time_1", s2.c_str());

      printlnEx("Publish: {" + s3 + "} to: " + "home/pool_pump/start_time_2");
      client.publish("home/pool_pump/start_time_2", s3.c_str());

      printlnEx("Publish: {" + s4 + "} to: " + "home/pool_pump/stop_time_2");
      client.publish("home/pool_pump/stop_time_2", s4.c_str());

      printlnEx("Publish: {" + s5 + "} to: " + "home/pool_pump/manual_run_time");
      client.publish("home/pool_pump/manual_run_time", s5.c_str());     

      //printlnEx(toggledOnce);
    }
    digitalWrite(LED_PIN, HIGH);  // Turn the LED on
    //delay(100);                   // Wait for 500ms
    //digitalWrite(LED_PIN, LOW);   // Turn the LED off
    previousMillis2 = currentMillis2;
    ledOn = true;
  }
  
  if (ledOn && (currentMillis2 - previousMillis2 >= onTime)) {
  // Turn the LED off after the specified time has elapsed
  digitalWrite(LED_PIN, LOW);
  ledOn = false;
  }
  
  
  // Timer-based LED toggle (non-blocking)
  // Uses millis() to blink LED on GPIO 16 without pausing other tasks  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalLedFlash) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
      digitalWrite(16, ledState);  // Assuming GPIO 16 is used for LED
      //printlnEx("LED is ON");
    } else {
      ledState = LOW;
      digitalWrite(16, ledState);  // Assuming GPIO 16 is used for LED
      //printlnEx("LED is OFF");
    }
  }

  // Read input state
  bool inputState = debouncedState;
  unsigned long currentTimeMills = millis();
  // Get the current time in 24-hour format
  currentTime24hourFormat = timeInt;

  // Handle Manual input
  if (HAinControl == false && isAuto == false){
    //logEventTime("839");
    handleInput(inputState, currentTimeMills);
    //printEx("Home Assistant Has Control: ");
    //printlnEx("False");
  }
  // Handle handleHomeAssstant input
  if (HAinControl == true && isAuto == false){
    handleHomeAssistant(currentTimeMills);
    //printEx("Home Assistant Has Control: ");
    //printlnEx("True");
  }
  
 
  
  
  // Power kill delay
  handlePowerKill(currentTimeMills);

  // Handle Count Down Timer Till Auto Mode
  countDownTimer();

  // Get the current millis
  //unsigned long currentMillis = millis();
  // Check if 10 seconds have passed
  if (currentMillis - previousMillisTimerCheck >= intervalTimerCheck) {
    // Save the last time you updated
    previousMillisTimerCheck = currentMillis;
    // Call the functions to check if the timers are active
    timerOneActive = isTimerActive(currentTime24hourFormat, starttime1, stoptime1);
    timerTwoActive = isTimerActive(currentTime24hourFormat, starttime2, stoptime2);
    printEx("Timer One Active: ");
    printlnEx(timerOneActive ? "Yes" : "No");
    printEx("Timer Two Active: ");
    printlnEx(timerTwoActive ? "Yes" : "No");
    printEx("Is Auto Active: ");
    printlnEx(isAuto ? "Yes" : "No");
  }

  if (isAuto == true) {
    if(timerOneActive == true){
      if (vfdReady) {
        digitalWrite(outputLowPin, LOW);
        digitalWrite(outputHighPin, HIGH);
      }  
      isLowSpeed = true;
      isHighSpeed = false;
      filterCommand = "run";
      filterHighLow = "low";
      //oneShot = true;
      if(oneShot == true){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
        printlnEx("client.publish <low>");
        oneShot = false;
      }
    }
  }else{
    isAutoCounter = isAutoCounter + 1;
    filterStatus = "manual";
    
  }
  if (isAuto == true) {
    if(timerTwoActive == true){
      if (vfdReady) {
        digitalWrite(outputLowPin, LOW);
        digitalWrite(outputHighPin, HIGH);
      }  
      isLowSpeed = true;
      isHighSpeed = false;
      filterCommand = "run";
      filterHighLow = "low";
      //oneShot = true;      
      if(oneShot == true){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
        printlnEx("client.publish <low>");
        oneShot = false;
      }     
    }
  }
  if (isAuto == true){
    if(timerOneActive == false && timerTwoActive == false){
      digitalWrite(outputLowPin, HIGH);
      digitalWrite(outputHighPin, HIGH);
      isLowSpeed = false;
      isHighSpeed = false;
      filterCommand = "stop";
      filterHighLow = "low";
      //oneShot = true;      
      if(oneShot == false){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
        printlnEx("client.publish <low>");
        oneShot = true;
      }           
    }
  }

} /////////////////////                  END OF LOOP()        //////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void writeToEEPROM(long starttime1, long stoptime1, long starttime2, long stoptime2) {
  EEPROM.begin(512); // Initialize EEPROM with a size of 512 bytes

  int addr = 0;

  // Write starttime1 to EEPROM
  EEPROM.put(addr, starttime1);
  addr += sizeof(starttime1);

  // Write stoptime1 to EEPROM
  EEPROM.put(addr, stoptime1);
  addr += sizeof(stoptime1);

  // Write starttime2 to EEPROM
  EEPROM.put(addr, starttime2);
  addr += sizeof(starttime2);

  // Write stoptime2 to EEPROM
  EEPROM.put(addr, stoptime2);
  addr += sizeof(stoptime2);

  // Write countdownHours to EEPROM
  EEPROM.put(addr, countdownHours);
  addr += sizeof(countdownHours);

  // Write countdownMinutes to EEPROM
  EEPROM.put(addr, countdownMinutes);
  //addr += sizeof(countdownMinutes);

  EEPROM.commit(); // Commit changes to EEPROM
  EEPROM.end(); // End EEPROM access

  // Print the updated times
  printlnEx("Writing to EEPROM:");
  printEx("starttime1: ");
  printlnEx(starttime1);
  printEx("stoptime1: ");
  printlnEx(stoptime1);
  printEx("starttime2: ");
  printlnEx(starttime2);
  printEx("stoptime2: ");
  printlnEx(stoptime2);

  delay(5000); 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readFromEEPROM(long &starttime1, long &stoptime1, long &starttime2, long &stoptime2) {
  EEPROM.begin(512); // Initialize EEPROM with a size of 512 bytes

  int addr = 0;

  // Read starttime1 from EEPROM
  EEPROM.get(addr, starttime1);
  addr += sizeof(starttime1);
  if (starttime1 < 1){
    starttime1 = 1000;
    printEx("EEPROM ERROR"); 
  }

  // Read stoptime1 from EEPROM
  EEPROM.get(addr, stoptime1);
  addr += sizeof(stoptime1);
  if (stoptime1 < 1){
    stoptime1 = 1600;
    printEx("EEPROM ERROR"); 
  } 

  // Read starttime2 from EEPROM
  EEPROM.get(addr, starttime2);
  addr += sizeof(starttime2);
  if (starttime2 < 1){
    starttime2 = 2200;
    printEx("EEPROM ERROR"); 
  } 

  // Read stoptime2 from EEPROM
  EEPROM.get(addr, stoptime2);
  addr += sizeof(stoptime2);
  if (stoptime2 < 1){
    stoptime2 = 400;
    printEx("EEPROM ERROR"); 
  } 
  // Read countdownHours from EEPROM
  EEPROM.get(addr, countdownHours);
  addr += sizeof(countdownHours);
  if (countdownHours < 1){
    countdownMinutes = 10;
    printEx("EEPROM ERROR");
  }
  // Read countdownMinutes from EEPROM
  EEPROM.get(addr, countdownMinutes);
  addr += sizeof(countdownMinutes);
  if (countdownMinutes < 0){
    countdownMinutes = 5;
    printEx("EEPROM ERROR"); 
  }
  
  EEPROM.end(); // End EEPROM access

  // Check if values are uninitialized (-1) and set to 0 if they are
  if (starttime1 == -1) starttime1 = 0;
  if (stoptime1 == -1) stoptime1 = 0;
  if (starttime2 == -1) starttime2 = 0;
  if (stoptime2 == -1) stoptime2 = 0;

  // Print the updated times
  printlnEx("Reedings from EEPROM:");
  printEx("starttime1: ");
  printlnEx(starttime1);
  printEx("stoptime1: ");
  printlnEx(stoptime1);
  printEx("starttime2: ");
  printlnEx(starttime2);
  printEx("stoptime2: ");
  printlnEx(stoptime2);

  delay(5000); 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Custom function to handle Serial print without newline and store in serialOutput
void printEx(const String &message) {
  Serial.print(message);    // Print to Serial Monitor without newline
  serialOutput += message;  // Store in serialOutput
  truncateSerialOutput(3000);
}

// Custom function to handle Serial print with newline and store in serialOutput
void printlnEx(const String &message) {
  Serial.println(message);    // Print to Serial Monitor with newline
  serialOutput += message;    // Store in serialOutput
  serialOutput += "\n";       // Append newline to serialOutput
  truncateSerialOutput(3000);
  
  //Serial.println(serialOutput.length()); 
}

// Overloaded functions for different data types
void printEx(int value) {
  Serial.print(value);    // Print to Serial Monitor without newline
  serialOutput += String(value);    // Store in serialOutput
  truncateSerialOutput(3000);
}

void printlnEx(int value) {
  Serial.println(value);    // Print to Serial Monitor with newline
  serialOutput += String(value);    // Store in serialOutput
  serialOutput += "\n";       // Append newline to serialOutput
  truncateSerialOutput(3000);
}

// Assuming serialOutput is a global String variable
// Function to truncate serialOutput to maxLength characters
void truncateSerialOutput(int maxLength) {
  if (serialOutput.length() > maxLength) {
    serialOutput = serialOutput.substring(serialOutput.length() - maxLength);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

void handleInput(bool inputState, unsigned long currentTime) {
  // If input is turned on
  if (inputState == HIGH) {
    if(toggleState == 0){
      if (vfdReady) {
        logEventTime("1105");
        digitalWrite(outputLowPin, LOW);
        digitalWrite(outputHighPin, HIGH);
      }
      filterHighLow = "low";      
      if(isLowSpeed == true){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        printlnEx("client.publish <low>");
      }
      isLowSpeed = false;
      isHighSpeed = true;
    }
    if(toggleState == 1){
      if (vfdReady) {
        logEventTime("1120");
        digitalWrite(outputLowPin, LOW);
        digitalWrite(outputHighPin, LOW);
      }  
      filterHighLow = "high";      
      if(isLowSpeed == false){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        printlnEx("client.publish <high>");
      }     
      isLowSpeed = true;
      isHighSpeed = false;
    }
  } 

  // Check if input state has changed and it's true
  if (inputState == HIGH && !toggledOnce) {
    // Check if the change occurred within 1 second
    if (millis() - lastChangeTime < 1000) {
      // Toggle the state variable
      toggledOnce = true;
      toggleState = !toggleState;
      printEx("Toggle state: ");
      printlnEx(toggleState);
    }
    filterCommand = "start";
    client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
  }

  if (inputState == HIGH){
    lastChangeTime = millis();
    //filterCommand = "start";
  } 

  // If input is off for more than 1 second, turn off both speeds
  if (inputState == LOW) {
    toggledOnce = false;
    //
    if (currentTime - lastInputTime > 1000) {
      digitalWrite(outputLowPin, HIGH);
      digitalWrite(outputHighPin, HIGH);
      filterCommand = "stop";
      filterHighLow = "low";
      if(isLowSpeed == true || isHighSpeed == true){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        printlnEx("client.publish <low>");
        client.publish("home/sensor/filterCommand", String(filterCommand).c_str()); 
      }
      
      isLowSpeed = false;
      isHighSpeed = false;
      toggleState = 1;
    }
      delay(500);
    }else{
      lastInputTime = currentTime; // Update last input change time
    }
  
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handleHomeAssistant(unsigned long currentTime) {

  if (filterCommand == "start"){
    if (filterHighLow == "low"){
      if(isLowSpeed == true){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        printlnEx("client.publish <low>");
      }
      if (vfdReady) {
        digitalWrite(outputLowPin, LOW);
        digitalWrite(outputHighPin, HIGH);
      }
      isLowSpeed = false;
      isHighSpeed = true;
      
    }
      if (filterHighLow == "high"){
      if(isLowSpeed == false){
        client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
        printlnEx("client.publish <high>");
      }      
      if (vfdReady) {
        digitalWrite(outputLowPin, LOW);
        digitalWrite(outputHighPin, LOW);
      }  
      isLowSpeed = true;
      isHighSpeed = false;
    }
  }
  if (filterCommand == "stop"){
    digitalWrite(outputLowPin, HIGH);
    digitalWrite(outputHighPin, HIGH);
    filterHighLow = "low";
    if(isLowSpeed == true){
      client.publish("home/sensor/filterHighLow", String(filterHighLow).c_str()); // One shot publish
      printlnEx("client.publish <low>");
    }
    isLowSpeed = false;
    isHighSpeed = false;
  }
}

////////////////////////////////////////////////////////////////
void handlePowerKill(unsigned long currentTime) {
  // Check if both speeds have been off for 6 seconds
  if (!isLowSpeed && !isHighSpeed && (currentTime - powerKillTime >= 60000)) {
    digitalWrite(powerKillPin, HIGH); // Turn off power kill relay
    //filterCommand = "stop";
    vfdStarting = false;
    vfdReady = false;
  } 
  if (isLowSpeed || isHighSpeed) {
    digitalWrite(powerKillPin, LOW); // Turn on power kill relay
    powerKillTime = currentTime;
    filterCommand = "start";

    
      // Start delay timer (one-shot)
    if (!vfdStarting && !vfdReady) {
      vfdStartTime = millis();
      vfdStarting = true;
    }    
    
    vfdStarting = true;
    //vfdReady = false;
    
    // Handle startup delay for VFD to be Ready
    if (vfdStarting && millis() - vfdStartTime >= 3000) {
      vfdReady = true;
      vfdStarting = false;
      Serial.println("VFD is ready. Outputs can now be used.");
      //delay(100);
    }
    Serial.print("vfdReady = ");
    Serial.println(vfdReady);
    Serial.print("vfdStarting = ");
    Serial.println(vfdStarting); 
    Serial.print("vfdStartTime = ");
    Serial.println(vfdStartTime);     
    //delay(100);
    Serial.println("VFD started, waiting 2s before allowing outputs...");
    //client.publish("home/sensor/filterCommand", String(filterCommand).c_str());
  }
}

// Function to check if the timer is active
bool isTimerActive(int currentTime, int startTime, int stopTime) {
  // Print the values for debugging
  printEx("Current Time: ");
  printlnEx(currentTime);
  printEx("Start Time: ");
  printlnEx(startTime);
  printEx("Stop Time: ");
  printlnEx(stopTime);

  if (startTime < stopTime) {
    // Normal case: timer does not span over midnight
    return (currentTime >= startTime && currentTime < stopTime);
  } else {
    // Spanning midnight case
    return (currentTime >= startTime || currentTime < stopTime);
  }

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void testLED(){
    // Set all pins to LOW
  digitalWrite(outputLowPin, HIGH);
  digitalWrite(outputHighPin, HIGH);
  digitalWrite(powerKillPin, HIGH);
  
  delay(200);  // Delay for 200 milliseconds

  // Set each pin to HIGH with a delay in between
  digitalWrite(outputLowPin, LOW);
  delay(200);  // Delay for 200 milliseconds

  digitalWrite(outputHighPin, LOW);
  delay(200);  // Delay for 200 milliseconds

  // Set each pin back to LOW with a delay in between
  digitalWrite(outputLowPin, HIGH);
  delay(200);  // Delay for 200 milliseconds

  digitalWrite(outputHighPin, HIGH);
  delay(200);  // Delay for 200 milliseconds
  // Set each pin to HIGH with a delay in between
  digitalWrite(outputLowPin, LOW);
  delay(200);  // Delay for 200 milliseconds

  digitalWrite(outputHighPin, LOW);
  delay(200);  // Delay for 200 milliseconds

  // Set each pin back to LOW with a delay in between
  digitalWrite(outputLowPin, HIGH);
  delay(200);  // Delay for 200 milliseconds

  digitalWrite(outputHighPin, HIGH);
  delay(200);  // Delay for 200 milliseconds
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void countDownTimer() {

  //printEx("countDownTimer(); Called : isAuto = ");
  //printlnEx(isAuto);

  if (isAuto) {
    // Check if .5 seconds have passed since last publish
    unsigned long currentTime = millis();
    if (currentTime - lastPublishTime >= publishInterval) {
      lastPublishTime = currentTime;

      timerDuration = (countdownHours * 60 + countdownMinutes) * 60 * 1000;
      countdownTime = timerDuration;
      timerActive = false;

      minutesToStart = timeToMinutes(starttime1) - timeToMinutes(currentTime24hourFormat);

      if (timerOneActive || timerTwoActive) { 
        publishString = "Automatic Mode is Active - Pump Running";
        client.publish("home/sensor/timer/countdown", publishString.c_str());
      } else {
        if (minutesToStart < 0) {
          minutesToStart = timeToMinutes(starttime2) - timeToMinutes(currentTime24hourFormat);
          if (minutesToStart < 0) {
            minutesToStart += 1440;  // wrap to next day
          }
        }

        int hours = minutesToStart / 60;
        int minutes = minutesToStart % 60;

        char timeFormatted[10];
        sprintf(timeFormatted, "%d:%02d", hours, minutes);  // ensures leading zero on minutes

        printEx("currentTime = ");
        printlnEx(currentTime24hourFormat);

        //publishString = String("Automatic Mode is Active - Starting Pump in ") + timeFormatted + " minutes";
        publishString = "Automatic Mode is Active - Starting Pump in " + String(hours) + " hours " + String(minutes) + " minutes";
        printlnEx(publishString);
        client.publish("home/sensor/timer/countdown", publishString.c_str());
        
      }
    }
    return;
  }

  static unsigned long previousMillisCDT = 0;
  timerDuration = (countdownHours * 60 + countdownMinutes) * 60 * 1000; // Convert hours and minutes to milliseconds

  //printlnEx(test2);
 // printlnEx(countdownHours);
 // printlnEx(countdownMinutes);
//  printlnEx(timerDuration);
 // printlnEx(hoursString);
 // printlnEx(minutesString);

  // Simulate setting and starting the timer
  if (!timerActive) {
    timerActive = true;
    countdownTime = timerDuration;
    printlnEx("Timer started.");
  }

  if (timerActive) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisCDT >= 5000) {
      previousMillisCDT = currentMillis;
      if(countdownDisabled == 0){
        countdownTime -= 5000; // Decrease countdown by 1 second
      }

      if (countdownTime <= 0) {
        timerActive = false;
        countdownTime = 0;
        printlnEx(F("Timer reached zero, switching to Auto Mode."));
        // Perform your action here
        filterStatus = "auto";
        if (isAuto == false){
          client.publish("home/sensor/filterStatus", String(filterStatus).c_str());
          if(countdownDisabled == 0){
            client.publish("home/sensor/timer/countdown", "00:00:00");
            printlnEx(F("client.publish(home/sensor/timer/countdown, 00:00:00)"));
          }else{
            client.publish("home/sensor/timer/countdown", "Disabled");
            printlnEx(F("publish( home/sensor/timer/countdown , Disabled"));
          }
        }
        isAuto = true;
        
        printlnEx(" Auto Mode Active.");

      } else {
        // Calculate hours and minutes
        unsigned int hours = countdownTime / (60 * 60 * 1000);
        unsigned int minutes = (countdownTime % (60 * 60 * 1000)) / (60 * 1000);

        // Print time remaining
        printEx("Time remaining: ");
        printEx(hours);
        printEx(":");
        if (minutes < 10) {
          printEx("0"); // Ensure minutes are two digits
        }
        printlnEx(minutes);
        // Convert hours and minutes to strings
        String hoursStr = String(hours);
        String minutesStr = String(minutes);
        if (minutes < 10) {
          minutesStr = "0" + minutesStr; // Ensure minutes are two digits
        }

        // Concatenate hours and minutes into a time string
        String timeString = hoursStr + ":" + minutesStr;

        // Publish the time string
        if(countdownDisabled == 0){
          client.publish("home/sensor/timer/countdown", timeString.c_str());
          printlnEx(F("client.publish(home/sensor/timer/countdown, timeString.c_str());"));
        }
        else { 
          client.publish("home/sensor/timer/countdown", "Disabled");
          printlnEx(F("client.publish(home/sensor/timer/countdown, Disabled"));
        }  
      }
    }
  }
}
