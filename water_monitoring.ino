#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
int serial = 1;
const char* ssid     = "EXELWLAN";
const char* password = "excel01082011";
ESP8266WebServer server(80);
WiFiClient client;
MDNSResponder mdns; //multicast Domain Name System

int f = 0;
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
float averageVoltage = 0, tdsValue = 0, temperature = 25;

float temp = 0, do1 = 0, flow_rate;
#define SensorPin A0
#define MUX_A 2
#define MUX_B 0
#define MUX_C 4

void setup (void)
{
  pinMode(SensorPin, INPUT);
  pinMode(MUX_A , OUTPUT);
  pinMode(MUX_B , OUTPUT);
  pinMode(MUX_C , OUTPUT);
  Serial.begin(115200);
  Serial.println("PH sensor experiment begins here");         // Test the serial  monitor
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.begin();
  Serial.println("HTTP server started");

}
void loop (void)
{
  changemux(LOW, LOW, LOW);
  float  phsensed = analogRead(SensorPin);
  float voltage = phsensed * 5.0 / 1024;
  float pHValue = 3.5 * voltage ;
  Serial.print(" pH value : ");
  Serial.println(pHValue, 2);
  delay(3000);

  changemux(LOW, LOW, HIGH);
  float tdssensor = analogRead(SensorPin);    //read the analog value and store into the buffer
  averageVoltage = tdssensor * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compensationVolatge = averageVoltage / compensationCoefficient; //temperature compensation
  tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
  Serial.print("TDS Value:");
  Serial.print(tdsValue, 0);
  Serial.println("ppm");
  delay(3000);

  changemux(LOW, HIGH, LOW);
  delay(1000);
  float flowsensor = analogRead(SensorPin);
  flow_rate = (1024.0 - flowsensor) * 30.0 / 1024.0;
  Serial.print("Flow_Rate:");
  Serial.print(flow_rate, 0);
  Serial.println("L/min");
  delay(3000);

  double sensorInput = analogRead(SensorPin);        //read the analog sensor and store it
  double temp = (double)sensorInput / 1024;   //find percentage of input reading
  temp = temp * 5;                     //multiply by 5V to get voltage
  temp = temp * 100;                    //Convert to degrees
  Serial.print("Temperature:");
  Serial.print(temp, 1);
  Serial.println("deg. C");
  delay(3000);

  float dosensor = analogRead(SensorPin);    //read the analog value and store into the buffer
  float averageVoltage = dosensor * (float)VREF / 1024.0; // read the value more stable by the median filtering algorithm
  float doValue = ( 14.46 + (int)(25.0 + 0.5) ) * averageVoltage / 1127.6; //calculate the do value, doValue = Voltage / SaturationDoVoltage * SaturationDoValue(with temperature compensation)
  Serial.print(F(",  DO Value:"));
  Serial.print(doValue, 2);
  Serial.println(F("mg/L"));
  delay(3000);
  
  f = f + 1;
  server.handleClient();
  if ( f < 3 ) {
    Serial.println("\nStarting connection to server...");
  }

  if (client.connect("192.168.2.152", 80)) {
    if ( f < 3 ) {
      Serial.println("connected to server");
    }

    client.print("GET /writer.php?"); // This
    client.print("serial="); // This
    client.print(serial);
    client.print("&temperature=");
    client.print(temp);
    client.print("&ph=");
    client.print(pHValue);
    client.print("&flow=");
    client.print(flow_rate);
    client.print("&tds=");
    client.print((int)tdsValue);
    client.print("&do=");
    client.print(doValue);// And this is what we did in the testing section above. We are making a GET request just like we would from our browser but now with live data from the sensor
    client.println(" HTTP/1.1"); // Part of the GET request
    client.println("Host: 127.0.0.1"); // IMPORTANT: If you are using XAMPP you will have to find out the IP address of your computer and put it here
    client.println("Connection: close"); // Part of the GET request telling the server that we are over transmitting the message
    client.println(); // Empty line
    client.println(); // Empty
    client.stop();    // Closing connection to server
  }
  else {
    // If Arduino can't connect to the server (your computer or web page)
    Serial.println("--> connection failed\n");
  }
  serial++;
  delay(15*1000);
}
void changemux(int c, int b, int a) {
  digitalWrite(MUX_A, a);
  digitalWrite(MUX_B, b);
  digitalWrite(MUX_C, c);
}
