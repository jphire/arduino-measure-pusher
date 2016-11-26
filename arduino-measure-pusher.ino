
#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <Arduino.h>

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte mac[] = { 0x20, 0x6A, 0x8A, 0x56, 0x94, 0x58 };

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(10,42,1,43);
EthernetClient client;

long lastReadTime = 0;                // Last time you read the sensor, in ms
long lastConnectionTime = 0;          // Last time you connected to the server, in milliseconds
boolean lastConnected = false;        // State of the connection last time through the main loop
const int postingInterval = 20*1000;  // Delay between updates to server

// Set the ids for the user's data series
char user_id[] = "11";
char channel_id[] = "-JRCS3Zw9H7aUPGCVKk2";
char temp_id[] = "-JRCSAE0hXqLCwCRPQ5S";
char hum_id[] = "-JRCSEN0YW-5rc8SKwti";
char dew_id[] = "-JRPRFJSDRzQxypL89MV";
char serverUrl[] = "exactumgh.herokuapp.com";
int serverPort = 80;

// Querystring part for the url
char queryString[92];
char serverQuery[] = "GET /index.php?user=%s&channel=%s&series=%s&value=%s";

char tempString[] = "00.00";
char t_str[33];

void setup() {
  Serial.begin(9600);
  dht.begin();
  delay(500);

  // Start ethernet
  Serial.println(F("Starting ethernet..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
//    // no point in carrying on, so do nothing forevermore:
//    // try to configure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }

  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  long currentTime = millis();
  
  // Check if interval time has passed after last data push
  if (currentTime > lastReadTime + postingInterval) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float dew_point = (float)dewPointFast(t, h);

    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    Serial.print("temp, humidity, dewpoint: ");
    Serial.print(t);
    Serial.print(" ");
    Serial.print(h);
    Serial.print(" ");
    Serial.println(dew_point);
    
    // If not connected, and the interval has passed since
    // last connection, connect again and send data
    if (!client.connected()) {
      sendData(user_id, channel_id, temp_id, t);
      sendData(user_id, channel_id, hum_id, h);
      sendData(user_id, channel_id, dew_id, dew_point);
    }
    // Update the time of the most current reading
    lastReadTime = millis();
  }

  // If there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  
  // If there's no connection, but there was one last time
  // through the loop, stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("Disconnecting.");
    client.stop();
  }

  // Save the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

byte sendData(char* user, char* channel, char* series, float value) {
  if(client.connected()) client.stop();

  if (client.connect(serverUrl, serverPort)) {
    Serial.println("Connected to remote host");
 
    // Convert the value to string
    sprintf(queryString, serverQuery, user, channel, series, ftoa(tempString, value, 2));
    Serial.println(queryString);
    
    // Close the connection immediately
    client.print(queryString);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverUrl);
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("Connection failed");
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
