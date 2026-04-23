/*
  Simple HTTPS PUSH request example for TerraHTTP library
  Sends a PUSH (POST) request with JSON body over HTTPS

  created 2024
  by Terra

  This example is in the public domain
*/

#include <TerraHTTP.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// WiFi Settings ///////
const char *ssid = "your_ssid";
const char *pass = "your_password";

const char *serverAddress = "api.example.com"; // server address
int port = 443;

WiFiClientSecure wifi;
TerraHttpClient client = TerraHttpClient(wifi, serverAddress, port);

void setup()
{
    Serial.begin(115200);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to Network named: ");
        Serial.println(ssid);

        // Connect to WPA/WPA2 network:
        WiFi.begin(ssid, pass);
        delay(5000);
    }

    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // Configure SSL (skip certificate verification for self-signed certificates)
    wifi.setInsecure();
}

void loop()
{
    // Create JSON body
    String jsonBody = "{\"temperature\": 25.5, \"humidity\": 60.0}";

    Serial.println("making HTTPS PUSH request with JSON body");

    // Send PUSH (POST) request with JSON content type
    client.push("/data", "application/json", jsonBody);

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