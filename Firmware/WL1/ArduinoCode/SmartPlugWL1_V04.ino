//**************************************************
/*
  Author: Habid Rascon-Ramos
  Project: Smart Plug
  Date: May 7, 2016
  Version: 4
  Description: This project is a smart plug using the Arduino.
  Wifi shield with MQTT and JSON libraries are used.
*/
//***************************************************

// Include all libraries
#include <WiFi.h>
#include <PubSubClient.h>
#include <aJSON.h>

// Define all pins
#define RELAYPIN 2 // Selection pin 
#define LEDPIN 8

// Set boolean values
boolean mismatchFlag = true;
boolean cmdFlag = false;

// Set string values
String userInput = "";
String cmdType = "";
String cmdSubType = "";
String cmdBody = "";

enum {
  // States
  OFF = 1,
  ON,
  DUMMY_STATE
} STATES;

enum {
  // Events
  START = 0,
  HELP,
  SEND,
  END,
  NONE
} EVENTS;

// Set byte values
byte state = OFF; // Tracks current state
byte prevState = DUMMY_STATE; // Dummy state
byte event = NONE; // Triggers a particular action within current state

// State Machine
void updateState() {
  switch (state) {

    case OFF:
      if (prevState != state) {
        //Entry event
        Serial.println(F(""));
        Serial.println(F("Starting OFF state."));
        digitalWrite(RELAYPIN, LOW); // LED off
        digitalWrite(LEDPIN, LOW); 
        Serial.println(F("LED is now OFF."));
        event = NONE;
        prevState = state;
      }

      if (event == END) {
        Serial.println(F(""));
        Serial.println(F("LED is already OFF!"));
        Serial.println(F("Staying in OFF state."));
        event = NONE;
      }

      if (event == START) {
        Serial.println(F(""));
        Serial.println(F("LED is turning ON."));
        state = ON;
        event = NONE;
      }
      break;

    case ON:
      if (prevState != state) {
        //Entry
        Serial.println(F(""));
        Serial.println(F("Starting ON state."));
        digitalWrite(RELAYPIN, HIGH); // LED on
        digitalWrite(LEDPIN, HIGH);
        Serial.println(F("The LED is now ON."));
        event = NONE;
        prevState = state;
      }

      if (event == START) {
        Serial.println(F(""));
        Serial.println(F("LED is already ON!"));
        Serial.println(F("Staying in ON state."));
        event = NONE;
      }

      if (event == END) {
        Serial.println(F(""));
        Serial.println(F("LED is turning OFF."));
        Serial.println(F("Returning to OFF state."));
        state = OFF;
        event = NONE;
      }
      break;
  }
}

// Handles parsing command from the command line
void parseCommand (String cmd) {
  boolean typeFlag = false;
  int tempIndex;
  int index;
  byte section = 0;
  char delimeter = ' ';

  for (index = 0; index < cmd.length(); index++) {
    if (cmd.charAt(index) == delimeter) section++;
    else {
      switch (section) {
        case 0:
          cmdType.concat(cmd.charAt(index));
          tempIndex = index;
          break;
        case 1:
          cmdSubType.concat(cmd.charAt(index));
          break;
        case 2:
          cmdBody.concat(cmd.charAt(index));
          break;
      }
    }
  }

  //Serial.println(tempIndex);
  //Serial.println(cmdSubType);
  //Serial.println(cmdBody);
  //Serial.println(cmd.length());
  //Serial.println(index);
  //Serial.println(section);

  if (cmdType.equals("help") | cmdType.equals("Help") | cmdType.equals("HELP")) {
    if (section == 0) {
      typeFlag = true;
      event = HELP;
    }
    if (section > 0 && (cmdSubType.equals("") && cmdBody.equals(""))) {
      for (int i = tempIndex + 1; i < index; i++) {
        //Serial.println(cmd.charAt(i));
        if (cmd.charAt(i) == ' ') {
          typeFlag = true;
          event = HELP;
        }
        else {
          typeFlag = false;
          event = NONE;
          break;
        }
      }
    }
  }

  if (cmdType.equals("on") | cmdType.equals("On") | cmdType.equals("ON")) {
    if (section == 0) {
      typeFlag = true;
      event = START;
    }
    if (section > 0 && (cmdSubType.equals("") && cmdBody.equals(""))) {
      for (int i = tempIndex + 1; i < index; i++) {
        if (cmd.charAt(i) == ' ') {
          typeFlag = true;
          event = START;
        }
        else {
          typeFlag = false;
          event = NONE;
          break;
        }
      }
    }
  }

  if (cmdType.equals("off") | cmdType.equals("Off") | cmdType.equals("OFF")) {
    if (section == 0) {
      typeFlag = true;
      event = END;
    }
    if (section > 0 && (cmdSubType.equals("") && cmdBody.equals(""))) {
      for (int i = tempIndex + 1; i < index; i++) {
        if (cmd.charAt(i) == ' ') {
          typeFlag = true;
          event = END;
        }
        else {
          typeFlag = false;
          event = NONE;
          break;
        }
      }
    }
  }

  if (!typeFlag) {
    Serial.println(F(""));
    Serial.println(F("Invalid Input!"));
    Serial.println(F("Please Try Again"));
  }

  cmdType = "";
  cmdBody = "";
  cmdSubType = "";
}

char ssid[] = "TP-LINK_1AFD";     //  your network SSID (name)
char pass[] = "xxxxxxxxxxxx";  // your network password
//char serverURI[] = "m12.cloudmqtt.com";
//int port = 16186;
//char clientName[] = "SmartPlug";
//char username[] = "iajmzgae";
//char password[] = "xxxxxxxxxxxx";

const long connectionCheckInterval = 30000;
int status = WL_IDLE_STATUS;     // Wifi radio's status

WiFiClient wfClient;
PubSubClient client("m12.cloudmqtt.com", 16186, callback, wfClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(F(""));
  Serial.println("Message arrived:  topic: " + String(topic));
  Serial.println("Length: " + String(length, DEC));

  char newString[length];
  char p[length];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.println("Payload: " + message);

  // JSON input requires character array rather than string
  message.toCharArray(newString, length + 1);
  /*for (int i = 0; i < length; i++) {
    Serial.println(newString[i]);
    //delay(500);
  }
  Serial.println(newString);*/

  // Begin JSON section
  // Parse character array into an aJsonObject
  aJsonObject* jsonObject = aJson.parse(newString);

  // Grab JSON key
  aJsonObject* cmdType = aJson.getObjectItem(jsonObject, "command");
  String cmdTypeString = cmdType->valuestring;
  Serial.println(F(""));
  Serial.print(F("Command sent: "));
  Serial.println(cmdTypeString);

  if (cmdTypeString == "On") {
    event = START;
    //Serial.println(F("Parsing successful!"));
  }

  else if (cmdTypeString == "Off") {
    event = END;
    //Serial.print(F("Parsing successful!"));
  }
  else {
    Serial.println(F(""));
    Serial.println(F("Invalid Input!"));
    Serial.println(F("Please Try Again"));
  }
  aJson.deleteItem(jsonObject);
}

// Program startup
void setup() {

  // Set Outputs
  digitalWrite(RELAYPIN, LOW);
  digitalWrite(LEDPIN, LOW);

  // set pin modes
  pinMode(RELAYPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  
  delay(2000); // Wait 2 seconds before resuming

  // Display Information to user
  Serial.begin(9600);
  Serial.println(F("Welcome!"));
  Serial.println(F("This is the SmartPlug Arduino Program."));
  Serial.println(F("Enter 'on' command to turn LED ON."));
  Serial.println(F("Enter 'off' command to turn LED OFF."));
  Serial.println(F("Use the 'help' command for futher assistance. "));

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F(""));
    Serial.println(F("WiFi shield not present"));
    // don't continue:
    while (true);
  }
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.println(F(""));
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print(F("You're now connected to the network "));
  printCurrentNet();
  printWifiData();

  Serial.println(F(""));
  Serial.println("Connecting to MQTT Broker ..");
  if (client.connect("SmartPlug", "iajmzgae", "xxxxxxxxxxxx")) {
    Serial.println(F("Connected :-)"));
  } else {
    Serial.println(F("Connection failed :-("));
    // don't continue:
    while (true);
  }

  // publish/subscribe
  if (client.connected()) {
    client.publish("/outTopic", "hello world");
    client.subscribe("SmartPlug");
  }

}

// Help screen function
void helpscreen() {
  if (event == HELP) {
    Serial.println(F(""));
    Serial.println(F("Displaying Commands:"));
    if (state == OFF) {
      Serial.println(F("on: Turns LED on."));
    }
    if (state == ON) {
      Serial.println(F("off: Turns LED off."));
    }
    delay(3000);
    event = NONE;
  }
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print(F("signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
}

void printWifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);
}

void checkConnection()  {
  static long previousMillis = 0;
  long currentMillis = millis();

  if (currentMillis - previousMillis > connectionCheckInterval) {
    previousMillis = currentMillis;

    if (WiFi.status() == WL_CONNECTED)  {
      if (client.connected()) {
        Serial.println(F(""));
        Serial.println(F("Connection OK"));
      } else {
        Serial.println(F(""));
        Serial.println(F("Lost MQTT connection! Reconnecting.."));
        if (client.connect("ArduinoClient", "iajmzgae", "xxxxxxxxxxxx")) {
          Serial.println(F("You're re-connected to MQTT Broker"));
          if (client.connected()) {
            client.publish("/outTopic", "hello world");
            client.subscribe("SmartPlug");
          }
        }
        else {
          Serial.println(F("Connection failed"));
        }
      }
    } else {
      while (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("Lost WiFi connection! Reconnecting.."));
        status = WiFi.begin(ssid, pass);
        delay(10000);
      }
      Serial.println(F("You're re-connected to the network"));
    }
  }
}

// Main loop
void loop() {

  checkConnection();
  helpscreen();
  updateState();
  client.loop();

  if (Serial.available() > 0) {
    int inByte = Serial.read();
    if (inByte == 13) {  // carriage return
      cmdFlag = true;
      Serial.println(F(""));
      Serial.print(F("Command sent: "));
      Serial.println(userInput);
      inByte = 0;
    }
    else {
      userInput.concat(char(inByte));
    }
  }
  if (cmdFlag) {
    parseCommand(userInput);
    userInput = "";
    cmdFlag = false;
  }

}
