#include <Arduino.h>
#include "Config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "CurtainControl.h"

enum DeviceState { SETUP_BLE, CONNECTING_WIFI, WORKING_WS };
DeviceState currentState = SETUP_BLE;
Preferences preferences;
WebServer server(80);

void handleDiscovery();
void handleCapabilities();
void handleCurtainControl();

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() > 0) {
        // Ожидаем формат "SSID:PASS"
        int sep = value.indexOf(':');
        if (sep != -1) {
          String ssid = value.substring(0, sep);
          String pass = value.substring(sep + 1);
          
          Serial.println("Сохраняю настройки WiFi в память...");
          preferences.begin("wifi-creds", false);
          preferences.putString("ssid", ssid);
          preferences.putString("pass", pass);
          preferences.end();

          Serial.println("Получены настройки! Перезапуск на WiFi...");
          WiFi.begin(ssid.c_str(), pass.c_str());
          currentState = CONNECTING_WIFI;
        }
      }
    }
};
void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    initCurtain();

    // Пытаемся загрузить учетные данные из памяти
    preferences.begin("wifi-creds", true); // true = read-only
    String savedSsid = preferences.getString("ssid", "");
    String savedPass = preferences.getString("pass", "");
    preferences.end();

    if (savedSsid.length() > 0 && savedPass.length() > 0) {
        Serial.println("Найдены сохраненные настройки WiFi. Подключаюсь...");
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
        currentState = CONNECTING_WIFI;
    } else {
        // Если настроек нет, запускаем BLE
        Serial.println("Сохраненные настройки WiFi не найдены. Запускаю BLE...");
        BLEDevice::init(DEVICE_NAME);
        BLEServer *pServer = BLEDevice::createServer();
        BLEService *pService = pServer->createService(SERVICE_UUID);
        
        BLECharacteristic *pChar = pService->createCharacteristic(
                                     CHARACTERISTIC_UUID,
                                     BLECharacteristic::PROPERTY_WRITE
                                   );
                                   
        pChar->setCallbacks(new MyCallbacks());
        pService->start();
        BLEDevice::getAdvertising()->start();
        
        Serial.println("Система готова. Жду конфиг по Bluetooth...");
        currentState = SETUP_BLE; // Устанавливаем состояние явно
    }
}
void loop() {
    if (currentState == SETUP_BLE) {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
        return; 
    }

    if (currentState == CONNECTING_WIFI) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n--- WiFi подключен! ---");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());

            if (MDNS.begin("smart-curtain")) { 
                Serial.println("mDNS запущен: http://smart-curtain.local");
                MDNS.addService("http", "tcp", 80); 
            }

            server.on("/discovery", HTTP_GET, handleDiscovery);
            server.on("/capabilities", HTTP_GET, handleCapabilities);
            server.on("/controls/curtain", HTTP_POST, handleCurtainControl);
            server.begin();
            Serial.println("Веб-сервер запущен!");

            currentState = WORKING_WS;
        }
        
    }

    if (currentState == WORKING_WS) {
        server.handleClient(); 
    }
}

// --- Обработчики API ---

void handleDiscovery() {
  Serial.println("-> GET /discovery");
  JsonDocument doc;
  doc["type"] = "NodeControlDevice";
  doc["name"] = "Smart Curtain";

  String output;
  serializeJson(doc, output);
  
  server.send(200, "application/json", output);
}

void handleCapabilities() {
  Serial.println("-> GET /capabilities");
  JsonDocument doc;
  JsonArray capabilities = doc.to<JsonArray>();

  JsonObject curtain = capabilities.add<JsonObject>();
  curtain["id"] = "curtain_control";
  curtain["type"] = "toggle";
  curtain["label"] = "Штора";
  curtain["endpoint"] = "/controls/curtain";

  CurtainState status = getCurtainStatus();
  curtain["value"] = (status == OPENED); // true если открыто, false если закрыто или движется

  String output;
  serializeJson(doc, output);

  server.send(200, "application/json", output);
}

void handleCurtainControl() {
  Serial.println("-> POST /controls/curtain");
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  bool open = doc["value"];
  if (open) {
    openCurtain();
  } else {
    closeCurtain();
  }

  server.send(200, "application/json", "{\"success\":true}");
}