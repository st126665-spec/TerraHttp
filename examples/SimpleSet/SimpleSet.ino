/*
  Simple SET request example for TerraHTTP library
  Sends a SET (PUT) request to update a resource

  created 2024
  by Terra

  This example is in the public domain
*/

#include <TerraHTTP.h>
#include <WiFi101.h>

#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// WiFi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

char serverAddress[] = "192.168.0.3"; // server address
int port = 8080;

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;

void setup()
{
    Serial.begin(9600);
    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to Network named: ");
        Serial.println(ssid);

        // Connect to WPA/WPA2 network:
        status = WiFi.begin(ssid, pass);
    }

    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
}

void loop()
{
    // Create JSON body for update
    String jsonBody = "{\"status\": \"active\", \"mode\": \"auto\"}";

    Serial.println("making SET request");

    // Send SET (PUT) request to update resource
    client.set("/device/123", "application/json", jsonBody);

    // read the status code and body of the response
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);

    Serial.println("Wait five seconds");
    delay(5000);
}
