# TerraHTTP Library - Quick Start Guide

## 📦 Library Created Successfully!

Your TerraHTTP library has been created in:
```
/Users/markreyvind.espiritu/Documents/Arduino/libraries/TerraHTTP/
```

## 📋 Complete File Structure

```
TerraHTTP/
├── README.md                    # Full documentation
├── QUICK_START.md               # Quick reference guide
├── keywords.txt                 # Syntax highlighting keywords
├── library.properties           # Library metadata
├── src/
│   ├── TerraHTTP.h             # Main library header
│   ├── TerraHttpClient.h        # Core HTTP client class
│   ├── TerraHttpClient.cpp      # Core HTTP client implementation
│   ├── WebSocketClient.h        # WebSocket client class
│   ├── WebSocketClient.cpp      # WebSocket client implementation
│   ├── URLEncoder.h             # URL encoding utilities
│   ├── URLEncoder.cpp           # URL encoding implementation
│   ├── b64.h                    # Base64 encoding header
│   └── b64.cpp                  # Base64 encoding implementation
└── examples/
    ├── SimpleTake/             # GET request example
    │   ├── SimpleTake.ino
    │   └── arduino_secrets.h
    ├── SimplePush/             # POST request example
    │   ├── SimplePush.ino
    │   └── arduino_secrets.h
    └── SimpleSet/              # PUT request example
        ├── SimpleSet.ino
        └── arduino_secrets.h
```

## 🔄 Method Name Mapping

| TerraHTTP | HTTP Method | Purpose |
|-----------|------------|---------|
| `take()` | GET | Retrieve data |
| `push()` | POST | Create/send data |
| `set()` | PUT | Update data |
| `resolve()` | PATCH | Partial update |
| `remove()` | DELETE | Delete data |
| `scan()` | readHeader | Read response titles |
| `sendTitle()` | sendHeader | Send custom titles |

## 🚀 Quick Usage Examples

### TAKE (GET) Request
```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

WiFiClient wifi;
TerraHttpClient client = TerraHttpClient(wifi, "api.example.com");

void setup() {
  // ... WiFi setup ...
  client.take("/api/data");
  int status = client.responseStatusCode();
  String response = client.responseBody();
}
```

### PUSH (POST) Request
```cpp
String jsonData = "{\"temp\": 25.5}";
client.push("/api/sensor", "application/json", jsonData);
int status = client.responseStatusCode();
```

### SET (PUT) Request
```cpp
String updateData = "{\"status\": \"active\"}";
client.set("/api/device/123", "application/json", updateData);
```

### RESOLVE (PATCH) Request
```cpp
String patchData = "{\"name\": \"Updated\"}";
client.resolve("/api/resource/456", "application/json", patchData);
```

### REMOVE (DELETE) Request
```cpp
client.remove("/api/item/789");
int status = client.responseStatusCode();
```

## 📝 Key Methods

### Request Methods
- `take(path)` - Simple GET
- `push(path)` / `push(path, contentType, body)` - POST
- `set(path)` / `set(path, contentType, body)` - PUT
- `resolve(path)` / `resolve(path, contentType, body)` - PATCH
- `remove(path)` / `remove(path, contentType, body)` - DELETE

### Custom Headers/Titles
```cpp
client.beginRequest();
client.sendTitle("Authorization", "Bearer TOKEN");
client.sendTitle("Content-Type", "application/json");
client.endRequest();
```

### Response Handling
```cpp
int status = client.responseStatusCode();     // Get HTTP status
String body = client.responseBody();           // Get full response
long length = client.contentLength();          // Get content length
while (client.titleAvailable()) {             // Read each title
  String name = client.readTitleName();
  String value = client.readTitleValue();
}
```

## 🔧 Installation in Arduino IDE

1. **Library Already in Correct Location**
   - The library is at: `/Documents/Arduino/libraries/TerraHTTP/`

2. **Restart Arduino IDE**
   - Close and reopen Arduino IDE

3. **Include in Your Sketch**
   ```cpp
   #include <TerraHTTP.h>
   ```

4. **Select Examples**
   - Go to `File → Examples → TerraHTTP`
   - Choose from: SimpleTake, SimplePush, or SimpleSet

## ⚙️ Configuration

### Update WiFi Credentials
Edit `examples/*/arduino_secrets.h` in each example:
```cpp
#define SECRET_SSID "your_wifi_ssid"
#define SECRET_PASS "your_wifi_password"
```

### Connection Settings
```cpp
// Create client with default port 80
TerraHttpClient client(wifi, "api.example.com");

// Create client with custom port
TerraHttpClient client(wifi, "api.example.com", 8080);

// Create client with IP address
IPAddress serverIP(192, 168, 1, 100);
TerraHttpClient client(wifi, serverIP, 8080);
```

## 🔒 HTTPS Support

For secure HTTPS connections on port 443:

### ESP32 / ESP8266
```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure wifi;
TerraHttpClient client(wifi, "api.example.com", 443);

// For self-signed certificates, skip verification:
wifi.setInsecure();
```

### Arduino MKR (WiFiNINA)
```cpp
#include <WiFiNINA.h>

WiFiSSLClient wifi;
TerraHttpClient client(wifi, "api.example.com", 443);
```

### WebSocket Secure (WSS)
```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure wifi;
WebSocketClient ws(wifi, "websocket.example.com", 443);
wifi.setInsecure();
ws.begin("/ws");
```

## 📡 Additional Library Components

TerraHTTP includes all the important components from ArduinoHttpClient:

### WebSocketClient
For real-time WebSocket communication:

```cpp
#include <TerraHTTP.h>
#include <WiFi101.h>

WebSocketClient webSocket(wifi, "websocket.example.com");

void setup() {
  // ... WiFi setup ...
  
  webSocket.begin("/ws");
  
  if (webSocket.beginMessage(TYPE_TEXT) == 0) {
    webSocket.print("Hello WebSocket!");
    webSocket.endMessage();
  }
}

void loop() {
  if (webSocket.parseMessage() > 0) {
    String message = webSocket.readString();
    Serial.println(message);
  }
}
```

### URLEncoder
URL encoding utilities for special characters:

```cpp
#include <TerraHTTP.h>

String encoded = URLEncoder.encode("Hello World!");
// Result: "Hello%20World%21"
```

### Base64 Encoding
Built-in base64 encoding support (used internally for authentication).

## �📚 Keywords Included (Syntax Highlighting)

The `keywords.txt` file includes all new methods for proper syntax highlighting in Arduino IDE:
- `take`, `push`, `set`, `resolve`, `remove`
- `scan`, `sendTitle`, `titleAvailable`
- `readTitleName`, `readTitleValue`
- And all other public methods

## 🎯 Key Features

✅ Intuitive method names (take, push, set, resolve, remove)
✅ Full HTTP support (GET, POST, PUT, PATCH, DELETE)
✅ Custom headers/titles support
✅ JSON body support
✅ Response parsing (status code, body, content length)
✅ WiFi and Ethernet compatible
✅ Comprehensive documentation
✅ Working examples included

## 📖 For More Information

See the full `README.md` for:
- Detailed API reference
- Advanced usage examples
- Response code definitions
- Troubleshooting tips

## ✅ What's Been Created

- ✅ Complete library implementation
- ✅ Header files with all methods renamed
- ✅ Implementation file (.cpp)
- ✅ Library properties file
- ✅ Keywords.txt for syntax highlighting
- ✅ Comprehensive README.md
- ✅ 3 example sketches (Take, Push, Set)
- ✅ Arduino secrets files for examples

The library is ready to use in the Arduino IDE! Simply restart the IDE and start creating with TerraHTTP.
