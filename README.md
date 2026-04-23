# TerraHTTP

TerraHTTP is an Arduino HTTP client library that provides intuitive, domain-specific method names for REST API interactions. Instead of traditional HTTP verbs (GET, POST, PUT, PATCH, DELETE), TerraHTTP uses natural language method names that make your code more readable and intent-driven.

## Features

- **Intuitive Method Names**: Uses domain-friendly names instead of standard HTTP methods
  - `take()` instead of GET - retrieve data
  - `push()` instead of POST - send/create data  
  - `set()` instead of PUT - update data
  - `resolve()` instead of PATCH - partial update
  - `remove()` instead of DELETE - delete data
  - `scan()` instead of readHeader() - read response titles (headers)

- **Complete Library Suite**: Includes all components from ArduinoHttpClient
  - **TerraHttpClient**: Core HTTP client with renamed methods
  - **WebSocketClient**: WebSocket communication support
  - **URLEncoder**: URL encoding utilities
  - **Base64 encoding**: Built-in base64 support for authentication


| TerraHTTP Method | HTTP Method | Purpose |
|-----------------|------------|---------|
| `take()` | GET | Retrieve data |
| `push()` | POST | Send/create data |
| `set()` | PUT | Update entire resource |
| `resolve()` | PATCH | Partial resource update |
| `remove()` | DELETE | Delete resource |
| `scan()` | (readHeader) | Read response titles |
| `sendTitle()` | (sendHeader) | Send custom title |

## Usage Examples

### Basic TAKE (GET) Request

```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

char ssid[] = "your_ssid";
char pass[] = "your_password";
char server[] = "api.example.com";

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, server);

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Make a TAKE (GET) request
  client.take("/api/data");
  
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  
  Serial.print("Status: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}

void loop() {
}
```

### PUSH (POST) Request with Body

```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, "api.example.com");

void setup() {
  Serial.begin(9600);
  // ... WiFi setup ...
  
  String postData = "{\"temperature\": 25.5, \"humidity\": 60}";
  
  // Make a PUSH (POST) request with JSON body
  client.push("/api/sensor/data", "application/json", postData);
  
  int statusCode = client.responseStatusCode();
  Serial.println(statusCode);
}

void loop() {
}
```

### SET (PUT) Request with Custom Titles

```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, "api.example.com");

void setup() {
  Serial.begin(9600);
  // ... WiFi setup ...
  
  // Start a SET request with custom titles
  client.beginRequest();
  client.sendTitle("Authorization", "Bearer YOUR_TOKEN");
  client.sendTitle("Content-Type", "application/json");
  client.sendTitle("X-Custom-Header", "value");
  
  String body = "{\"status\": \"active\"}";
  client.startRequest("/api/device/123", "PUT", "application/json", body.length(), (const byte*)body.c_str());
  
  int statusCode = client.responseStatusCode();
  Serial.println(statusCode);
}

void loop() {
}
```

### RESOLVE (PATCH) Request

```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, "api.example.com");

void setup() {
  Serial.begin(9600);
  // ... WiFi setup ...
  
  // Partial update using RESOLVE (PATCH)
  String patchData = "{\"name\": \"New Name\"}";
  client.resolve("/api/user/42", "application/json", patchData);
  
  int statusCode = client.responseStatusCode();
  Serial.println(statusCode);
}

void loop() {
}
```

### REMOVE (DELETE) Request

```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, "api.example.com");

void setup() {
  Serial.begin(9600);
  // ... WiFi setup ...
  
  // Delete resource using REMOVE
  client.remove("/api/item/456");
  
  int statusCode = client.responseStatusCode();
  
  if (statusCode == 204) {
    Serial.println("Resource deleted successfully");
  } else {
    Serial.println("Delete failed");
  }
}

void loop() {
}
```

### Reading Response Titles and Body

```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, "api.example.com");

void setup() {
  Serial.begin(9600);
  // ... WiFi setup ...
  
  client.take("/api/info");
  
  int statusCode = client.responseStatusCode();
  Serial.print("Status: ");
  Serial.println(statusCode);
  
  // Read response titles (headers)
  while (client.titleAvailable()) {
    String name = client.readTitleName();
    String value = client.readTitleValue();
    Serial.print(name);
    Serial.print(": ");
    Serial.println(value);
  }
  
  // Get content length
  long length = client.contentLength();
  Serial.print("Content length: ");
  Serial.println(length);
  
  // Read response body
  String body = client.responseBody();
  Serial.println(body);
}

void loop() {
}
```

## API Reference

### Constructor

```cpp
TerraHttpClient(Client& client, const char* server, uint16_t port = 80);
TerraHttpClient(Client& client, const String& server, uint16_t port = 80);
TerraHttpClient(Client& client, const IPAddress& address, uint16_t port = 80);
```

### Request Methods

- `int take(const char* path)` - GET request
- `int take(const String& path)` - GET request
- `int push(const char* path)` - POST request
- `int push(const char* path, const char* contentType, const char* body)` - POST with body
- `int set(const char* path)` - PUT request
- `int set(const char* path, const char* contentType, const char* body)` - PUT with body
- `int resolve(const char* path)` - PATCH request
- `int resolve(const char* path, const char* contentType, const char* body)` - PATCH with body
- `int remove(const char* path)` - DELETE request
- `int remove(const char* path, const char* contentType, const char* body)` - DELETE with body

### Request Control Methods

- `void beginRequest()` - Start a complex request
- `void beginBody()` - Start sending request body
- `void endRequest()` - End the request
- `void sendTitle(const char* title)` - Send full title (header) line
- `void sendTitle(const char* name, const char* value)` - Send title with name and value
- `void sendTitle(const char* name, int value)` - Send title with integer value
- `void sendBasicAuth(const char* user, const char* password)` - Send basic authentication
- `int startRequest(const char* path, const char* method, const char* contentType, int length, const byte* body)` - Advanced request start

### Response Methods

- `int responseStatusCode()` - Get HTTP status code (200, 404, etc.)
- `bool titleAvailable()` - Check if a response title (header) is available
- `String readTitleName()` - Read the name of current title
- `String readTitleValue()` - Read the value of current title
- `int scan()` - Read next byte while scanning titles
- `int skipResponseTitles()` - Skip all response titles
- `bool endOfTitlesReached()` - Check if all titles have been read
- `long contentLength()` - Get Content-Length value
- `bool isResponseChunked()` - Check if response is chunked
- `String responseBody()` - Get entire response body as String
- `bool endOfBodyReached()` - Check if body is completely read

### Configuration Methods

- `void connectionKeepAlive()` - Enable connection keep-alive
- `void noDefaultRequestTitles()` - Disable default titles
- `void stop()` - Close connection
- `void setHttpResponseTimeout(uint32_t timeout)` - Set response timeout
- `void setHttpWaitForDataDelay(uint32_t delay)` - Set data wait delay

### Status Codes

- `HTTP_SUCCESS` (0) - Request successful
- `HTTP_ERROR_CONNECTION_FAILED` (-1) - Connection failed
- `HTTP_ERROR_API` (-2) - API usage error
- `HTTP_ERROR_TIMED_OUT` (-3) - Request timeout
- `HTTP_ERROR_INVALID_RESPONSE` (-4) - Invalid response

## HTTPS Support

TerraHTTP supports HTTPS connections by using secure client implementations. For ESP32 and ESP8266 boards, use `WiFiClientSecure` instead of `WiFiClient`:

```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure wifi;
TerraHttpClient client(wifi, "api.example.com", 443);

// For self-signed certificates or testing, skip verification:
wifi.setInsecure();
```

For Arduino MKR series with WiFiNINA, use `WiFiSSLClient`:

```cpp
#include <WiFiNINA.h>

WiFiSSLClient wifi;
TerraHttpClient client(wifi, "api.example.com", 443);
```

## WebSocket Support

TerraHTTP includes WebSocket support through the `WebSocketClient` class. For secure WebSocket (WSS) connections, use a secure client:

```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure wifi;
WebSocketClient ws(wifi, "api.example.com", 443);

void setup() {
  // ... WiFi setup ...
  
  wifi.setInsecure(); // For self-signed certificates
  ws.begin("/websocket");
  
  // Send a text message
  ws.beginMessage(TYPE_TEXT);
  ws.print("Hello WebSocket!");
  ws.endMessage();
}
```

## Requirements

- Arduino board with WiFi or Ethernet capability
- One of these libraries:
  - WiFi101
  - WiFiNINA
  - Ethernet
  - WiFi (for Arduino MKR series)
  - MKRGSM or MKRNB (for cellular)

## License

Released under Apache License, version 2.0

## Credits

Adapted from the Arduino HTTP Client library, with method names redesigned for clarity and domain-driven development.

## Support

For issues, questions, or contributions, please visit the project repository.
