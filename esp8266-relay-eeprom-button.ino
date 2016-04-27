#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>

// wifi creds 
const char* ssid = "your_ssid";
const char* password = "your_password";

// gpio 
const int relayPin = 5;
const int buttonPin = 0;

// webserver 
ESP8266WebServer server(80);
// http updater
ESP8266HTTPUpdateServer httpUpdater;

// eeprom - keep button state across reboots
const int  ep_addr = 0;
int ep_val = 0;

// relay state
int relayState = LOW;

// button debounce 
int buttonState = HIGH; // INPUT PULL UP
int lastButtonState;
long lastDebounceTime = 0;
long debounceDelay = 50; // adjust this when needed

// html contents
String message = "";

// button - software debounce
boolean soft_debounce() {
  
  boolean retVal = false;
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        retVal = true;
      }
    }
  }
  lastButtonState = reading;
  return retVal;
}

// commit relay state to eeprom 
void eeprom_commit(int rstate) {
  
  if (ep_val != rstate) {
    EEPROM.write(ep_addr, rstate);
    EEPROM.commit();
  }
  ep_val = rstate;
}


void handleRelay() {
  
  String cmd = "";
  if ((server.uri() == "/relay") and (server.hasArg("state"))){
    cmd = server.arg("state");
    if (cmd.equals("ON")){
      digitalWrite(relayPin, HIGH);
      relayState = HIGH;
    }
    else if (cmd.equals("OFF")){
      digitalWrite(relayPin, LOW);
      relayState = LOW;
    }
    else if (cmd.equals("PON")){
      digitalWrite(relayPin, HIGH);
      relayState = HIGH;
      eeprom_commit(1);
    }
    else if (cmd.equals("POFF")){
      digitalWrite(relayPin, LOW);
      relayState = LOW;
      eeprom_commit(0);
    }
  }

  message = "<html><body>";
  message += "Relay is :";
  if(relayState == HIGH) {
    message += "ON";  
  } else {
    message += "OFF";
  }
  message += "<br><ul>";
  message += "<li><a href=\"/relay?state=ON\">Set relay ON</a></li>";
  message += "<li><a href=\"/relay?state=OFF\">Set relay OFF</a></li>";
  message += "<li><a href=\"/relay?state=PON\">Set relay ON --- persistent</a></li>";
  message += "<li><a href=\"/relay?state=POFF\">Set relay OFF --- persistent</a></li>";
  message += "<li><a href=\"/\">Refresh</a></li>";
  message += "</ul>";
  message += "Last cmd: " + cmd;
  message += "</body></html>";
  server.send(200, "text/html", message);
}


// init 
void setup() {

  // set relay gpio pin and state
  pinMode(relayPin, OUTPUT);

  // set buton gpio pin
  pinMode(buttonPin, INPUT_PULLUP);

  // eeprom init
  EEPROM.begin(512);
  ep_val = EEPROM.read(ep_addr);
  if (ep_val == 1){
    relayState = HIGH;
  }
  digitalWrite(relayPin, relayState); // restore state from default/eeprom

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
   
  // web server related
  httpUpdater.setup(&server);  // /update handler
  
  // uri handlers
  server.on("/", handleRelay);
  server.on("/relay", handleRelay); 
  server.onNotFound(handleRelay);
  server.begin();
}

// main loop 
void loop() {

  delay(30); //slow down the cpu a bit in case the loop is too fast
  
  boolean pressed = soft_debounce();
  if ((pressed == true) && (relayState == HIGH)){
    digitalWrite(relayPin, LOW);
    relayState = LOW;
  }
  else if ((pressed == true) && (relayState == LOW)){
    digitalWrite(relayPin, HIGH);
    relayState = HIGH;
  }
  
  server.handleClient();
}
