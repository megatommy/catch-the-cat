//#include <WiFi.h>
//#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "index.h"

const char *ssid = "Hello world";
const char *password = "thomas12345";

const char *ap_ssid = "Catch the Cat";
const char *ap_password = "12345678";
IPAddress ap_IP(192,168,1,1);
IPAddress gateway(192,168,1,2);
IPAddress subnet(255,255,255,0);



const int trigPin = 14;
const int echoPin = 12;
const int led = 13;
// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
//define sound speed in cm/uS
#define SOUND_SPEED 0.034



short readings_above_threshold = 0; //check reading, if > treshhold, add 1. at 5 send mail or so. if one < treshold, reset to 0
short readings_to_check = 3;

bool data_sent = false;
bool alert_sent = false;
bool sms_enabled = false;

int threshold_cm;
int check_every;

// SIM card PIN (leave empty, if not defined)
char simPIN[10]   = "";
String sms_number = "";
String sms_message = "";
String than = "";

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb
#include <TinyGsmClient.h>

//DEBUG STUFF
// Set serial for debug console (to Serial Monitor, default speed 115200)
#define Serial Serial
// Set serial for AT commands (to SIM800 module)
#define SerialAT  Serial1

// Define the serial console for debug prints, if needed
#define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, Serial);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif
//END DEBUG STUFF

WebServer server(80);


void setup(void) {
  Serial.begin(115200);
  Serial.println("SETUP");

  pinMode(led, OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  delay(2000);
    
  
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  //------------------ CLIENT -----------------------
//  WiFi.mode(WIFI_STA);
//  WiFi.disconnect();
//  delay(1000);
//  
//  WiFi.begin(ssid, password);
//
//  // Wait for connection
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//
//  Serial.println("");
//  Serial.print("Connected to ");
//  Serial.println(ssid);
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());
  //------------------ CLIENT -----------------------

  //------------------ AP -----------------------
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAPConfig(ap_IP, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //------------------ AP -----------------------

  // http://esp32.local
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
    MDNS.addService("http", "tcp", 80);
  }

  server.onNotFound(handleNotFound);

  server.on("/", []() {
    server.send(200, "text/html", PAGE_INDEX);
  });

  server.on("/simulate", []() {
    delay(10000);
    server.send(200, "text/plain", "OK");
  });


  server.on("/get-data", handleGetData);
  server.on("/save-data", handleSaveData);
  server.on("/test-sms", handleTestSMS);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  delay(2);
  if(!data_sent){
    server.handleClient();
    return;
  }

  if(alert_sent){ //did its duty, time to sleep forever
    delay(2);
    return;
  }

  delay(check_every * 1000); //every X seconds, check
  
  float distance = read_distance_sensor();
  Serial.print("Distance: "); Serial.println(distance);

  if(
      (threshold_cm < distance && than == "less-than")
      || (threshold_cm > distance && than == "greater-than")
  ) {
    //maybe last one was false alarm, resetting to 0
    readings_above_threshold = 0;
  } else {
      readings_above_threshold++;
  }
  
  Serial.print("count: "); Serial.print(readings_above_threshold);
  if(readings_above_threshold >= readings_to_check){
    //more than X times the reading has been above the threshold, alerting!
    if(sms_enabled){
      send_alert();  
    }
    Serial.println("SMS sent, alert_sent = true");
    //close_cage()
    digitalWrite(led, 1);
    delay(3000);
    digitalWrite(led, 0);
    alert_sent = true;
  }
}


void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}



void handleGetData(){
  float distanceCm = read_distance_sensor();
  String uptime = get_uptime();
  
  char result[25];
  sprintf(result, "%s---%.1f", uptime, distanceCm);
  server.send(200, "text/plain", result);
}

void handleSaveData(){
  check_every = server.arg("check_every").toInt();
  threshold_cm = server.arg("threshold_cm").toInt();
  readings_to_check = server.arg("readings_to_check").toInt();
  
  sms_number = server.arg("sms_number");
  sms_message = server.arg("sms_message");
  than = server.arg("than");

  if(sms_number != ""){
    sms_enabled = true;
  }

  snprintf(simPIN, sizeof(simPIN), "%s", server.arg("sim_pin"));

  String response = "<meta name='viewport' content='width=device-width, initial-scale=1.0'><h1>Data sent!</h1>";
  for (uint8_t i = 0; i < server.args(); i++) {
    response += "<p><b>" + server.argName(i) + ":</b> " + server.arg(i) + "</p>";
  }

  response += "<br/><br/><p>Prendi uno screenshot di questa schermata per poter consultare i valori inseriti</p>";

  
  server.send(200, "text/html", response);
  data_sent = true;
  delay(2000);
  WiFi.softAPdisconnect(true);
}

void handleTestSMS(){
  snprintf(simPIN, sizeof(simPIN), "%s", server.arg("sim_pin"));
  sms_number = server.arg("sms_number");
  sms_message = server.arg("sms_message");
  
  Serial.println("Test SMS received:");
  Serial.print("sim_pin: "); Serial.println(simPIN);
  Serial.print("sms_number: "); Serial.println(sms_number);
  Serial.print("sms_message: "); Serial.println(sms_message);
  
  short sms_sent = send_alert();

  //removing values
  snprintf(simPIN, sizeof(simPIN), "%s", "");
  sms_number = "";
  sms_message = "";
  server.send(200, "text/plain", String(sms_sent));
}

String get_uptime(){
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  char result[12];
  sprintf(result, "%02d:%02d:%02d", hr, min % 60, sec % 60);
  return result;
}

float read_distance_sensor(){
  //return 0;
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);


  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  float distanceCm = duration * SOUND_SPEED/2;
  return distanceCm;
}

short send_alert(){
  Serial.println("send_alert();");

  
  //delay(5000);
  //return random(0,4);
  // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  //0: none succesfull, 1: first one OK
  short sms_sent = 0;

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Restart SIM800 module, it takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();
  // use modem.init() if you don't need the complete restart

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }

  // To send an SMS, call modem.sendSMS(SMS_TARGET, smsMessage)
  //String smsMessage = "Hello from ESP32!";
  if(modem.sendSMS(sms_number, sms_message)){
    Serial.println(sms_message);
    sms_sent = 1;
  }
  else{
    Serial.println("SMS failed to send");
  }

  delay(3000);


  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, LOW);
  digitalWrite(MODEM_POWER_ON, LOW);

  return sms_sent;
}
