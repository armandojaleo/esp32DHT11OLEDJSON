#include <WiFi.h>
#include "DHT.h"
#include "SSD1306Wire.h"

#define DHTPIN 14
#define DHTTYPE DHT11
#define SCLPIN 21
#define SDAPIN 22

const char* ssid     = "yourssid";
const char* password = "yourssidpass";
const int led1 =  12;

WiFiServer server(80);
DHT dht(DHTPIN, DHTTYPE);
SSD1306Wire  display(0x3c, SCLPIN, SDAPIN);

char linebuf[80];
int charcount = 0;

void setup() {
  Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  pinMode(led1, OUTPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    display.clear();
    display.drawString(10, 10, "Conectando...");
    display.display();
  }
  digitalWrite(led1, HIGH);
  server.begin();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  display.clear();
  display.drawString(10, 10, "IP: " + String(WiFi.localIP().toString()));
  display.drawString(10, 20, "Hu: " + String(h));
  display.drawString(10, 30, "Te: " + String(t));
  display.display();

  memset(linebuf, 0, sizeof(linebuf));
  charcount = 0;
  boolean currentLineIsBlank = true;
  boolean json = false;
  String contentType = "Content-Type: text/html";

  WiFiClient client = server.available();
  if (client)
  {
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        linebuf[charcount] = c;
        if (charcount < sizeof(linebuf) - 1) charcount++;

        if (c == '\n' && currentLineIsBlank)
        {
          client.println("HTTP/1.1 200 OK");
          client.println(contentType);
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          if (json)
          {
            client.print("{\"humidity\":\"");
            client.print(h);
            client.print("\",\"temperature\":\"");
            client.print(t);
            client.print("\",\"heatindex\":\"");
            client.print(hic);
            client.print("\"}");
          }
          else
          {
            client.println("<!DOCTYPE HTML><html><head>");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.1.0/css/bootstrap.min.css' integrity='sha384-9gVQ4dYFwwWSjIDZnLEWnxCjeSWFphJiwGPXr1jddIhOegiu1FwO5qRGvFXOdJZ4' crossorigin='anonymous'>");
            client.println("<script src='https://code.jquery.com/jquery-3.3.1.slim.min.js' integrity='sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo' crossorigin='anonymous'></script>");
            client.println("<script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.0/umd/popper.min.js' integrity='sha384-cs/chFZiN24E4KMATLdqdvsezGxaGsi4hLGOzlXwp5UZB1LY//20VyM2taTB4QvJ' crossorigin='anonymous'></script>");
            client.println("<script src='https://stackpath.bootstrapcdn.com/bootstrap/4.1.0/js/bootstrap.min.js' integrity='sha384-uefMccjFJAIv6A+rW+L4AHf99KvxDjWSu1z9VI8SKNVmz4sk7buKt/6v9KI65qnm' crossorigin='anonymous'></script>");
            client.println("</head><body class='bg-light'>");
            client.println("<div class='container'>");
            client.println("<h1>Arduino Server</h1>");
            client.println("<p>");
            client.println("<a href='/'><button type='button' class='btn btn-primary'>LOAD DATA</button></a> ");
            client.println("<a href='/json' target='_new'><button type='button' class='btn btn-secondary'>LOAD DATA JSON</button></a> ");
            client.println("<a href='/lon'><button type='button' class='btn btn-success'>LED ON</button></a> ");
            client.println("<a href='/loff'><button type='button' class='btn btn-warning'>LED OFF</button></a>");
            client.println("</p>");
            client.println("<h3>Data</h3>");
            client.println("<p>Humidity: ");
            client.println(h);
            client.println(" %<br>");
            client.println("Temperature: ");
            client.println(t);
            client.println(" *C - ");
            client.println(f);
            client.println(" *F<br>");
            client.println("Heat index: ");
            client.println(hic);
            client.println(" *C - ");
            client.println(hif);
            client.println(" *F</p>");
            client.println("</div></body></html>");
          }
          break;
        }
        if (c == '\n')
        {
          currentLineIsBlank = true;
          if (strstr(linebuf, "GET /lon") > 0)
          {
            digitalWrite(led1, HIGH);
          }
          else if (strstr(linebuf, "GET /loff") > 0)
          {
            digitalWrite(led1, LOW);
          }
          else if (strstr(linebuf, "GET /json") > 0)
          {
            contentType = "Content-Type: application/json;charset=utf-8";
            json = true;
          }
          currentLineIsBlank = true;
          memset(linebuf, 0, sizeof(linebuf));
          charcount = 0;
        }
        else if (c != '\r')
        {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }
}
