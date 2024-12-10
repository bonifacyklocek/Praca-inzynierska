#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* ssid = ""; // DODAĆ SSID
const char* password = ""; // DODAĆ HASŁO
String serverName = ""; // DODAĆ NAZWĘ SERWERA

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wi-Fi connected");
}

void sendDataToServer(uint8_t *buf, size_t total_size, String format, String name) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverName + "?name=" + name + "." + format);

    http.addHeader("Content-Type", "image/" + format);

    int httpResponseCode = http.POST(buf, total_size);

    if (httpResponseCode > 0)
    {
      String response = http.getString();
      Serial.println("Server response: " + response);
    }
    else
    {
      Serial.println("Error. Response code: " + String(httpResponseCode));
    }
    http.end();
}