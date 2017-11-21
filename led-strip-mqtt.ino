/*

   Controller for LED strip in studio.  Listens to MQTT broker
   and responds to messages from home assistant

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>

#define     GREENPIN  4
#define     REDPIN    5
#define     BLUEPIN   14

#define     FADESPEED 20

const char* ssid = "ssidgoeshere";
const char* password = "putpasswordhere";
const char* mqtt_server = "192.168.1.210";

WiFiClient espClient;
PubSubClient client(espClient);

int mode = 1; // start in color fading mode

// when holding a solid color, maintain the percent values here
float redPercent = 0.0;
float bluePercent = 0.0;
float greenPercent = 0.0;

const char* outTopic = "hass/ledstrip";
const char* inTopic = "hass/+";

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    for(int i = 0; i<500; i++){
      delay(1);
    }
    Serial.print(".");
  }
  analogWrite(BLUEPIN, 0);
  delay(500);
  analogWrite(BLUEPIN, 255);
  delay(500);
  analogWrite(BLUEPIN, 0);
  delay(500);
  analogWrite(BLUEPIN, 255);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void allRed() {
  analogWrite(REDPIN, 255);
  analogWrite(BLUEPIN, 0);
  analogWrite(GREENPIN, 0);
}


void allBlue() {
  analogWrite(BLUEPIN, 255);
  analogWrite(REDPIN, 0);
  analogWrite(GREENPIN, 0);  
}


void allGreen() {
  analogWrite(GREENPIN, 255);
  analogWrite(REDPIN, 0);
  analogWrite(BLUEPIN, 0);    
}


/**
  This should be called frequently when fading lights to see if we need to bail out and change modes
*/
boolean checkMessagesAndInterrupt() {
  delay(50); // FIXME, might be too fast
  int oldMode = mode;
  client.loop();
  if (mode != oldMode) {
    Serial.print("modes have changed");
    return true;
  } else {
    return false;
  }
}


/**
  COLOR01
  rotate through all colors starting with solid blue
*/
void color01() {
  int r, g, b;

  allBlue();

  // fade from blue to violet
  for (r = 0; r < 256; r++) {
    analogWrite(REDPIN, r);
    if (checkMessagesAndInterrupt())
      return;
  }
  // fade from violet to red
  for (b = 255; b > 0; b--) {
    analogWrite(BLUEPIN, b);
    if (checkMessagesAndInterrupt())
      return;
  }
  // fade from red to yellow
  for (g = 0; g < 256; g++) {
    analogWrite(GREENPIN, g);
    if (checkMessagesAndInterrupt())
      return;
  }
  // fade from yellow to green
  for (r = 255; r > 0; r--) {
    analogWrite(REDPIN, r);
    if (checkMessagesAndInterrupt())
      return;
  }
  // fade from green to teal
  for (b = 0; b < 256; b++) {
    analogWrite(BLUEPIN, b);
    if (checkMessagesAndInterrupt())
      return;
  }
  // fade from teal to blue
  for (g = 255; g > 0; g--) {
    analogWrite(GREENPIN, g);
    if (checkMessagesAndInterrupt())
      return;
  }
}


/**
  Called by main loop when all we need to do is maintain the current RGB percent values.
*/
void maintainPercentages() {
  analogWrite(GREENPIN, 255*greenPercent);
  analogWrite(REDPIN, 255*redPercent);
  analogWrite(BLUEPIN, 255*bluePercent);
}


/**
  The main callback that gets called when MQTT messages arrive
*/
void callback(char* topic, byte* payload, unsigned int length) {
  char charPayload[4] = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  if (length > 3) {
    Serial.print("Invalid too long value received ");
    Serial.println();
    return;
  }

  for (int i = 0; i < length; i++) {
    charPayload[i] = payload[i];
  }

  Serial.println();

  String strPayload = String(charPayload);
  int strInt = strPayload.toInt();
  float percent = strInt / 100.0;

  Serial.print("strPayload ");
  Serial.print(strPayload);
  Serial.println();

  Serial.print("strInt ");
  Serial.print(strInt);
  Serial.println();

  Serial.print("percent ");
  Serial.print(percent);
  Serial.println();

  String topicStr = String(topic);

  if (topicStr.equals("hass/green")) {
    Serial.print("setting green to ");
    Serial.print(255 * percent);
    Serial.println();
    greenPercent = 255 * percent;
    mode = 50;
  } else if (topicStr.equals("hass/blue")) {
    Serial.print("setting blue to ");
    Serial.print(255 * percent);
    Serial.println();
    bluePercent = 255 * percent;
    mode = 50;
  } else if (topicStr.equals("hass/red")) {
    redPercent = 255 * percent;
    mode = 50;
  } else if (topicStr.equals("hass/color01")) {
    // set mode to 1 so that color01 routine runs (called from loop)
    mode = 1;
  } else {
    Serial.print("Defaulting to color01 mode");
    Serial.println("");
    mode = 1;
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "booted");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for(int i = 0; i<5000; i++){
       
        delay(1);
      }
    }
  }
}


void setup() {
  // pin 13 is the build-in blue LED
  pinMode(13, OUTPUT);
    
  analogWrite(BLUEPIN, 255);
  delay(500);
  analogWrite(BLUEPIN, 0);
  delay(500);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  analogWrite(BLUEPIN, 255);
  analogWrite(REDPIN, 0);
  analogWrite(GREENPIN, 0);
}


void loop() {

  if (!client.connected()) {
    reconnect();
  }

    Serial.print("mode is ");
    Serial.print(mode);
    Serial.println();


  switch (mode) {
    // in the default mode, no change to the LED state needs to happen

    case 1:
      color01();
      break;

    case 50:
      maintainPercentages();
      break;

    default:
      color01();
      break;
  }
  
  client.loop();
}
