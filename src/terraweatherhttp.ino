#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <TerraHTTP.h> // TerraHTTP Library with HTTPS support
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22

const char *ssid = "iot-ict-lab24g";
const char *password = "iot#labclass";

// Server Configuration - HTTPS on port 443
const char *serverAddress = "203.154.11.225";
const int serverPort = 443; // HTTPS port (blocked by professor, only 443 is open)
const char *serverPath = "/data";

const char *DEVICE_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkZXZpY2VfaWQiOiJkZXZfZWI3NjM1YmMiLCJwcm9qZWN0X2lkIjoicHJval9iODI2NjgyZiIsImV4cCI6MTgwODExOTc2NiwiaWF0IjoxNzc2NTgzNzY2fQ.4ZvgN61XwlSUA4fabsuFkWAdjhsu9YHZPjKERYat31c";

DHT dht(DHTPIN, DHTTYPE);

// TerraHTTP HTTPS Client
WiFiClientSecure wifiClient;
TerraHttpClient client(wifiClient, serverAddress, serverPort);

unsigned long lastMsg = 0;

void setup()
{
    Serial.begin(115200);
    dht.begin();

    WiFi.begin(ssid, password);
    Serial.print("Connecting WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    // Configure SSL/TLS for HTTPS on port 443
    // Skip certificate verification for self-signed or restricted certificates
    wifiClient.setInsecure();
}

void loop()
{
    unsigned long now = millis();

    if (now - lastMsg > 5000)
    {
        lastMsg = now;

        float hum = dht.readHumidity();
        float temp = dht.readTemperature();

        if (isnan(hum) || isnan(temp))
        {
            Serial.println("❌ Failed to read from DHT sensor!");
            return;
        }

        // Prepare JSON Payload
        String json = "{";
        json += "\"temperature\": " + String(temp) + ",";
        json += "\"humidity\": " + String(hum) + ",";
        json += "\"noise\": 0,";
        json += "\"pm25\": 0,";
        json += "\"pm10\": 0,";
        json += "\"pressure\": 0,";
        json += "\"lux\": 0";
        json += "}";

        // --- Send HTTPS PUSH request using TerraHTTP ---
        Serial.println("[HTTPS] Sending PUSH request via TerraHTTP...");

        // Start request with content type header
        client.beginRequest();
        client.sendHeader("Content-Type", "application/json");
        client.sendHeader("Authorization", "Bearer " + String(DEVICE_TOKEN));

        // Send PUSH (POST) request with JSON body
        int httpCode = client.push(serverPath, "application/json", json);

        if (httpCode == 0)
        {
            // Read response
            int statusCode = client.responseStatusCode();
            String response = client.responseBody();

            Serial.printf("[HTTPS] Status Code: %d\n", statusCode);
            if (statusCode == 200 || statusCode == 201)
            {
                Serial.println("Response: " + response);
            }
        }
        else
        {
            Serial.printf("[HTTPS] PUSH failed, error code: %d\n", httpCode);
        }

        Serial.println("Waiting 5 seconds before next request...");
    }
}