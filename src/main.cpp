#include <Arduino.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// TeamID A35
String teamId = "";

// Wi-Fi network parameters
String ssid = "";
String password = "";
bool connected_WiFi;
#define CONNECT_TIMEOUT 15000 // ms
long connectStart = 0;

// TODO -- BTC server name
#define btcServerName "ESP32_BTC_A35"

// Define the BluetoothSerial
BluetoothSerial SerialBT;

// Define a variable for the connection status
bool connected_BTC = false;

const char* url = "http://proiectia.bogdanflorea.ro/api/the-wolf-among-us/characters";
const char* url1 = "http://proiectia.bogdanflorea.ro/api/the-wolf-among-us/character?id=";
const char* api_key = "";

// Callback function for connection events
void deviceConnected(
esp_spp_cb_event_t event, 
esp_spp_cb_param_t *param
) { 
 if(event == ESP_SPP_SRV_OPEN_EVT){ 
 Serial.println("Device Connected"); 
 connected_BTC = true; 
 } 
 if(event == ESP_SPP_CLOSE_EVT ){ 
 Serial.println("Device disconnected");
 connected_BTC = false; 
 } 
}

// Received data via BTC
String data = "";

// The receivedData function is called when data is available on Bluetooth (see loop function)
void receivedData() {
  // Read the data using the appropriate SerialBT functions
  // according to the app specifications
  // The data is terminated by the new line character (\n)
  while (SerialBT.available()) {
    data = SerialBT.readStringUntil('\n');
  }

  Serial.println(data); // <-- This is the message sent from the app, according to the specs

  // Possible steps:
  // 1. Deserialize data using the ArduinoJson library
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, data);
    if (error) {
      Serial.println(F("Failed to parse JSON"));
    return;
    }
  

  // 2. Get the action from the JSON object
  // Access the values in the JSON data
  String message_action = doc["action"];
  Serial.print(F("Action request: "));
  Serial.println(message_action);

  // 3. Check action and perform the corresponding operations
  // IMPORTANT -- if it is an API project, connect to WiFi when the appropriate action is requested and 
  // make sure to set the http request timeout to 10s or more
  // IMPORTANT -- If using the ArduinoJson library, use a DynamicJsonDocument with the size of 15000
  // 5. Define the response structure, according to the app specifications
  // (Use JsonArray or JsonObject, depending on the response type)
  // IMPORTANT -- The capacity of the response array/object must not exceed 512B, especially for BLE
  // 6. Populate the response object according to the app specs
  // 7. Encode the response object as a JSON string
  // 8. Write value to the characteristic and notify the app
  // TODO -- Write your code
  
if (message_action == "getNetworks") {
    String message_teamId = doc["teamId"];
    teamId = message_teamId;
    Serial.print(F("Team ID: "));
    Serial.println(message_teamId);
    
    // Scan for WiFi networks
    Serial.println("Start scan");
    int networksFound = WiFi.scanNetworks();
    Serial.println("Scan complete");
    if (networksFound == 0) {
        Serial.println("No networks found");
      } 
    else {
      Serial.print(networksFound);
      Serial.println(" Networks found");

      // Iterate the network list and display the information for 
      // each network
      for (int i = 0; i < networksFound; i++) {
        bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print("dB)");
        Serial.print(" - ");
        Serial.println((open) ? "Open" : "Protected");

        DynamicJsonDocument network(1024);
        network["ssid"] = WiFi.SSID(i);
        network["strength"] = WiFi.RSSI(i);
        network["encryption"] = WiFi.encryptionType(i);
        network["teamId"] = teamId;
        String jsonString;
        serializeJson(network, jsonString);
        Serial.println(jsonString);
        SerialBT.println(jsonString);
        delay(100);
     }
    }
}
  else if (message_action == "connect")
  {
    String message_ssid = doc["ssid"];
    String message_password = doc["password"];
    ssid = message_ssid;
    password = message_password;
    Serial.print(F("Wi-Fi network SSID: "));
    Serial.println(message_ssid);
    Serial.print(F("Wi-Fi network Password: "));
    Serial.println(message_password);
    
    // Initialize the WiFi network
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting");
    // Connect to the network
    // Check the connection status in a loop every 0.5 seconds
    // Since the connection can take some time and might fail,
    // it is necessary to check the connection status before
    // proceeding. To define a timeout for the action, the
    // current counter of the timer is stored
    connectStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
      // Check if the defined timeout is exceeded
      if (millis() - connectStart > CONNECT_TIMEOUT) {
          break;
      }
    }
    Serial.println("");
    connected_WiFi = false;
    // Check if connection was successful
    if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connection failed");
      } else {
          Serial.print("Connected to network: ");
          Serial.println(WiFi.SSID());
          Serial.print("Local IP address: ");
          Serial.println(WiFi.localIP());
          connected_WiFi = true;
      }
    DynamicJsonDocument doc1(200);
    doc1["ssid"] = ssid;
    doc1["connected"] = connected_WiFi;
    doc1["teamId"] = teamId;
    String output1;
    serializeJson(doc1, output1);
    SerialBT.println(output1);
  }
  
   else if (message_action == "getData") {
  	Serial.println("Retrieving data from server...");

  	HTTPClient http;
  	http.begin(url);

  	int httpCode = http.GET();

  	if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    DynamicJsonDocument jsonDoc(15000);
    DeserializationError error = deserializeJson(jsonDoc, payload);

    if (error) {
        Serial.println("Error parsing JSON message.");
        return;
    }

    if (!jsonDoc.is<JsonArray>()) {
        Serial.println("JSON message is not an array.");
        return;
    }

    JsonArray recordsArray = jsonDoc.as<JsonArray>();
    DynamicJsonDocument jsonDoc2(15000);

    for (JsonObject record : recordsArray) {
      int id = record["id"];
      String name = record["name"];
      String imagePath = record["imagePath"];
    
      Serial.print("Record ID: ");
      Serial.println(id);
      Serial.print("Name: ");
      Serial.println(name);
      Serial.print("Image URL: ");
      Serial.println(imagePath);
      Serial.print("Team ID: ");
      Serial.println(teamId);
      jsonDoc2["id"] = id;
      jsonDoc2["name"] = name;
      jsonDoc2["image"] = imagePath;
      jsonDoc2["teamId"] = teamId;

      String jsonResponse2;
      serializeJson(jsonDoc2, jsonResponse2);

      SerialBT.println(jsonResponse2); 
    }   
    
 	 } else {
    	Serial.println("Failed to retrieve data from server.");
 	 }

 	 http.end();
	
  }
  else if (message_action == "getDetails") {
    int message_id = doc["id"];
    Serial.print(F("ID Details: "));
    Serial.println(message_id);
    
    Serial.println("Sending GET request to remote server...");

    HTTPClient http1;
  
    // Add API key to URL query string
    String full_url = String(url1) + String(message_id);

    http1.begin(full_url);

    int httpCode1 = http1.GET();

    if (httpCode1 > 0) {
    String payload1 = http1.getString();

    // Parse JSON response
    DynamicJsonDocument doc3(2048);
    deserializeJson(doc3, payload1);

    // Get specific details from JSON
    const char* detail1 = doc3["name"];
    const char* detail2 = doc3["imagePath"];
    const char* detail3 = doc3["description"];
    DynamicJsonDocument jsonDoc3(2048);

    Serial.print("id: ");
    Serial.println(message_id);
    Serial.print("name: ");
    Serial.println(detail1);
    Serial.print("imagePath: ");
    Serial.println(detail2);
    Serial.print("description: ");
    Serial.println(detail3);
    Serial.print("teamID: ");
    Serial.println(teamId);
    jsonDoc3["id"] = message_id;
    jsonDoc3["name"] = detail1;
    jsonDoc3["image"] = detail2;
    jsonDoc3["description"] = detail3;
    jsonDoc3["teamId"] = teamId;
    // Serialize JSON message and send via BTC
    String jsonResponse3;
    serializeJson(jsonDoc3, jsonResponse3);

    SerialBT.println(jsonResponse3); 
    } else {
    Serial.println("Error sending GET request.");
    }
    http1.end();
  }
 // Reset the received data string after processing 
  data = "";
 }


void setup() {
  // put your setup code here, to run once:
  // No need to change the lines below
  Serial.begin(115200);
  // Initialize BTC
  SerialBT.begin(btcServerName); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");

  // Register the callback function for connect/disconnect events
  SerialBT.register_callback(deviceConnected);
  Serial.println("Device ready for pairing!");

  // Set WiFi to station mode and disconnect from an AP  
  // if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  }

void loop() {
  // put your main code here, to run repeatedly:

  // Check available Bluetooth data and perform read from the app
  if (SerialBT.available()) {
    Serial.println("Bluetooth active");
    receivedData();
  }
  // possible code to be executed for the actions which run periodically 
  // (possibly conditioned by an interrupt flag)
}