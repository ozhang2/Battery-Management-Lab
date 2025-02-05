#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <esp_sleep.h>

// Wi-Fi and Firebase configuration
const char* ssid = "UW MPSK";
const char* password = "e7A{G^Nw!:";
#define DATABASE_SECRET "AIzaSyDns0W6QuVwz_TqaCJRoC1xSFrSDydOcl8"
#define DATABASE_URL "https://esp32-project-bb207-default-rtdb.firebaseio.com/"

// Ultrasonic sensor pin configuration
const int trigPin = 4;
const int echoPin = 5;

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

// Detection range and sleep duration
float lastDistance = 0.0;
const float threshold = 5.0;  // Upload data if change exceeds 5cm
const int sleepDuration = 30 * 1000000;  // Deep sleep for 30 seconds

void connectToWiFi() {
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi!");
    } else {
        Serial.println("\nFailed to connect to Wi-Fi!");
    }
}

void setupFirebase() {
    ssl.setInsecure();
    initializeApp(client, app, getAuth(dbSecret));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    client.setAsyncResult(result);
}

float measureDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    return duration * 0.034 / 2;
}

void uploadToFirebase(float distance) {
    String path = "/ultrasonic/distance";
    if (Database.set<float>(client, path, distance)) {
        Serial.println("Data uploaded successfully!");
    } else {
        Serial.println("Data upload failed.");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    connectToWiFi();
    setupFirebase();
}

void loop() {
    float distance = measureDistance();
    Serial.print("Measured Distance: ");
    Serial.println(distance);

    if (abs(distance - lastDistance) > threshold) {
        uploadToFirebase(distance);
        lastDistance = distance;
    }

    // Enter deep sleep mode
    esp_sleep_enable_timer_wakeup(sleepDuration);
    esp_deep_sleep_start();
}