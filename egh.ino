
#include <SPI.h>
#include <Ethernet.h>
#include <dht11.h>
#include <Arduino.h>
#include <DateTime.h>

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
const int postingInterval = 10*1000;  //delay between updates to server

char queryString[84];

char tempString[] = "00.00";
char humString[] = "00.00";
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
    time_t t = DateTime.now();

    Serial.println(ftoa(tempString,temperature,2));
    
    if(!client.connected()) {
      sendData(t, temperature, humidity); 
    }
    lastReadTime = millis();
  }

  if (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

  lastConnected = client.connected();
}

byte sendData(time_t cur_time, float temperature, float humidity) {
  if(client.connected()) client.stop();

  if(client.connect("exactumgh.herokuapp.com",80)) {
    Serial.println("connecting..");
 
    sprintf(queryString,"GET /index.php?id=-JPnv3s_R0BjJ6_Lxkd7&time=%s&value=%s",itoa(cur_time,t_str,10),ftoa(tempString,temperature,2));
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
