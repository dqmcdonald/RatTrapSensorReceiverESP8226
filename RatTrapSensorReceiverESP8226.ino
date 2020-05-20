
/*
    WIFI enabled LoRa Gateway for handling rat trap sensor data
    and soil moisture sensor data. Communication with LORA is via Serial
    connnection to Arduino Pro Mini

    Quentin McDonald
    2018
*/


#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>


#pragma region Globals
uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals

String ssid = "";
String ssid_password = "";


String data_string = "";
int num_bytes = 0;
int bytes_read = 0;

const int FIELD_LENGTH = 64; // Fields stored in EEPROM are 64 bits long


#define CONNECTION_LED_PIN D5
#define TRANSMIT_LED_PIN D6

/* Pin definitions: */
#define RX D1
#define TX D2
SoftwareSerial softSerial = SoftwareSerial(RX, TX);




String read_field( int idx ) {
  String fld = "";
  for ( int i = 0; i < FIELD_LENGTH; i++ ) {
    fld += char(EEPROM.read(i + idx * FIELD_LENGTH));

  }
  Serial.print("Field: ");
  Serial.print(idx);
  Serial.print(" is: ");
  Serial.println( fld );

  return fld;
}


void post_data(const String& dstring) {


  Serial.println("Posting data string:");
  Serial.println(dstring);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const char* host = "192.168.1.13";
  const int httpPort = 8080;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  //  // We now create a URI for the request
  String url = "/input/";


  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Accept: */*\r\n" +
               "Content-Length: " + dstring.length() + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n\r\n");
  client.println(dstring);

  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");



}


uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
  static uint16_t attempt = 0;
  Serial.print("Connecting to ");
  if (nSSID) {
    WiFi.begin(nSSID, nPassword);
    Serial.println(nSSID);
  } else {
    WiFi.begin(ssid.c_str(), ssid_password.c_str());
    Serial.println(ssid);
  }

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 50)
  {
    delay(200);
    Serial.print(".");
  }
  ++attempt;
  Serial.println("");
  if (i == 51) {
    Serial.print("Connection: TIMEOUT on attempt: ");
    Serial.println(attempt);
    if (attempt % 2 == 0)
      Serial.println("Check if access point available or SSID and Password\r\n");
    return false;
  }
  Serial.println("Connection: ESTABLISHED");
  Serial.print("Got IP address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(CONNECTION_LED_PIN, HIGH);
  return true;
}

void Awaits()
{
  uint32_t ts = millis();
  while (!connection_state)
  {
    delay(50);
    if (millis() > (ts + reconnect_interval) && !connection_state) {
      connection_state = WiFiConnect();
      ts = millis();
    }
  }
}



void send_rat_trap_data(String& data_string ) {

  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
  delay(250);
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
  delay(1000);
  String station = data_string.substring( 12);
  String ds = "LORAT=" + station;
  Serial.print("Sending rat trap data: ");
  Serial.println(ds);
  post_data(ds);
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
}

bool is_moisture_data( const String& data_string ) {
  return (data_string[0] == 'M' && data_string[1] == 'S' && data_string[2] == ':');
}

bool is_temperature_data( const String& data_string ) {
  return (data_string[0] == 'T' && data_string[1] == 'P' && data_string[2] == ':');
}

void send_moisture_data( String data_string ) {
  /* Split up the moisture data and send to the database server */
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
  delay(250);
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
  delay(1000);
  Serial.print("Parsing Moisture Data: ");
  Serial.println(data_string );
  String station = data_string.substring( 4, 10 );
  String value = data_string.substring( 11 );
  String ds = "MOISTURE=" + value;
  ds += "&STATION=";
  ds += station;
  Serial.print("Sending moisture data: ");
  Serial.println(ds);
  post_data( ds );
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
}

void sendTemperatureData( String data_string ) {
  /* Split up the soil temperature data and send to the database server */
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
  delay(250);
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);
  delay(1000);
  Serial.print("Parsing Temperature Data: ");
  Serial.println(data_string );
  String station = data_string.substring( 4, 10 );
  String value = data_string.substring( 11 );
  String ds = "SOILTEMP=" + value;
  ds += "&STATION=";
  ds += station;
  Serial.print("Sending temperature data: ");
  Serial.println(ds);
  post_data( ds );
  digitalWrite(TRANSMIT_LED_PIN, HIGH);
  delay(500);
  digitalWrite(TRANSMIT_LED_PIN, LOW);

}

void setup()
{
  Serial.begin(115200);

  pinMode(CONNECTION_LED_PIN, OUTPUT);
  digitalWrite(CONNECTION_LED_PIN, LOW);

  pinMode(TRANSMIT_LED_PIN, OUTPUT);
  digitalWrite(TRANSMIT_LED_PIN, LOW);

  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();


  // Read fields from EEPROM:
  ssid = read_field(0);
  ssid_password = read_field(1);

  connection_state = WiFiConnect();
  if (!connection_state) // if not connected to WIFI
    Awaits();          // constantly trying to connect



}



void loop() {

  if ( softSerial.available()) {

    if ( num_bytes == 0 ) {
      num_bytes = (int)softSerial.read();
      Serial.print("Num bytes to read from Arduino =");
      Serial.println(num_bytes);
    } else {
      char c = softSerial.read();
      data_string += c;
      bytes_read++;
      // If we have read the whole string post it to server
      if ( num_bytes == bytes_read ) {
        if ( is_moisture_data( data_string ) ) {
          send_moisture_data( data_string );
        } else if ( is_temperature_data( data_string ) ) {
          sendTemperatureData( data_string );
        } else {
          send_rat_trap_data( data_string );
        }
        data_string = "";
        num_bytes = 0;
        bytes_read = 0;
      }
    }
  }
}
