#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include "Gsender.h"
#include <EEPROM.h>


#pragma region Globals
uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals

String ssid = "";
String ssid_password = "";
String email_login = "";
String email_password = "";
const String EMAIL_ADDRESS = "dqmcdonald@gmail.com";

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

void send_email( const String& address, const String& subject, const String& message ) {

  Gsender *gsender = Gsender::Instance(email_login, email_password );    // Getting pointer to class instance
  if (gsender->Subject(subject)->Send(address, message)) {
    Serial.println("Message sent.");
  } else {
    Serial.print("Error sending message: ");
    Serial.println(gsender->getError());
  }



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
  email_login = read_field(2);
  email_password = read_field(3);

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
        digitalWrite(TRANSMIT_LED_PIN, HIGH);
        delay(500);
        digitalWrite(TRANSMIT_LED_PIN, LOW);
        delay(250);
        digitalWrite(TRANSMIT_LED_PIN, HIGH);
        delay(500);
        digitalWrite(TRANSMIT_LED_PIN, LOW);
        delay(1000);
        send_email(EMAIL_ADDRESS, "Rat Trap Triggered", data_string);
        digitalWrite(TRANSMIT_LED_PIN, HIGH);
        delay(500);
        digitalWrite(TRANSMIT_LED_PIN, LOW);
        data_string = "";
        num_bytes = 0;
        bytes_read = 0;
      }
    }
  }


}





