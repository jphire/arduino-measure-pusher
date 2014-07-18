
#include <SPI.h>
#include <Ethernet.h>
#include <dht11.h>
#include <Arduino.h>

dht11 DHT11;
#define DHT11PIN 2

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte mac[] = { 0x20, 0x6A, 0x8A, 0x56, 0x94, 0x58 };

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(10,42,1,43);
int serverPort = 80;
EthernetClient client;

long lastReadTime = 0;        // the last time you read the sensor, in ms
long lastConnectionTime = 0;        // last time you connected to the server, in milliseconds
boolean lastConnected = false;      // state of the connection last time through the main loop
const int postingInterval = 20*1000;  //delay between updates to server

// for the push-url
char queryString[92];

// set the ids for the user's data series
char user_id[] = "11";
char channel_id[] = "-JRCS3Zw9H7aUPGCVKk2";
char temp_id[] = "-JRCSAE0hXqLCwCRPQ5S";
char hum_id[] = "-JRCSEN0YW-5rc8SKwti";
char dew_id[] = "-JRPRFJSDRzQxypL89MV";

char tempString[] = "00.00";
char t_str[33];

void setup() {
  Serial.begin(9600);
  delay(500);

  // Start ethernet
  Serial.println(F("Starting ethernet..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }

  Serial.println(Ethernet.localIP());
}

void loop() {
  long currentTime = millis();

  if (currentTime > lastReadTime + postingInterval) {
    checkSensor();
    float temperature = (float)DHT11.temperature;
    float humidity = (float)DHT11.humidity;
    float dew_point = (float)dewPointFast(temperature, humidity);
    Serial.print("dewpoint: ");
    Serial.println(dew_point);
    // if you're not connected, and the interval has passed since
    // your last connection, then connect again and send data:
    if(!client.connected()) {
      sendData(user_id, channel_id, temp_id, temperature);
      sendData(user_id, channel_id, hum_id, humidity);
      sendData(user_id, channel_id, dew_id, dew_point);
    }
    // update the time of the most current reading:
    lastReadTime = millis();
  }

  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  
  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

byte sendData(char* user, char* channel, char* series, float value) {
  if(client.connected()) client.stop();

  if(client.connect("exactumgh.herokuapp.com",80)) {
    Serial.println("connecting..");
 
    // convert the value to string
    sprintf(queryString,"GET /index.php?user=%s&channel=%s&series=%s&value=%s",user,channel,series,ftoa(tempString,value,2));
    Serial.println(queryString);
    client.print(queryString);
    client.println(" HTTP/1.1");
    client.println("Host: exactumgh.herokuapp.com");
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}

void checkSensor(){
  Serial.println("\n");

  int chk = DHT11.read(DHT11PIN);

  Serial.print("Read sensor: ");
  switch (chk)
  {
    case DHTLIB_OK: 
		Serial.println("OK"); 
		break;
    case DHTLIB_ERROR_CHECKSUM: 
		Serial.println("Checksum error"); 
		break;
    case DHTLIB_ERROR_TIMEOUT: 
		Serial.println("Time out error"); 
		break;
    default: 
		Serial.println("Unknown error"); 
		break;
  }
}

double dewPointFast(double celsius, double humidity) {
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

char *ftoa(char *a, double f, int precision) {
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};  
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}
