#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include <esp_sleep.h>

#define WIFI_SSID "UW MPSK"
#define WIFI_PASSWORD "e7A{G^Nw!:"

#define DATABASE_SECRET "AIzaSyDns0W6QuVwz_TqaCJRoC1xSFrSDydOcl8"
#define DATABASE_URL "https://esp32-project-bb207-default-rtdb.firebaseio.com/"

#define TRIG_PIN 4  
#define ECHO_PIN 5  
#define SOUND_SPEED 0.034  

#define UPLOAD_INTERVAL 2000 // Upload data every 2 seconds
#define SLEEP_INTERVAL 30000000 // 30 seconds Deep Sleep (in microseconds)

WiFiClientSecure ssl;
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

unsigned long previousMillis = 0;
bool object_far_away = false; // Record whether the target object is far away
unsigned long far_start_time = 0; // Record the time when the object starts moving away

void connectToWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void initFirebase() {
    ssl.setInsecure();
    initializeApp(client, app, getAuth(dbSecret));
    Database.url(DATABASE_URL);
}

float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * SOUND_SPEED / 2;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    return distance;
}

void sendDataToFirebase(float distance) {
    String path = "/distance";
    Database.set<float>(client, path, distance, [](AsyncResult &res) {});
}

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    connectToWiFi();
    initFirebase();
}

void loop() {
    float distance = measureDistance();

    unsigned long currentMillis = millis();
    if (distance > 50) { // Check if the object is moving away
        if (!object_far_away) {
            object_far_away = true;
            far_start_time = currentMillis;
        } else if (currentMillis - far_start_time > 30000) { // If more than 30 seconds
            Serial.println("Object far away for 30s, entering deep sleep...");
            WiFi.disconnect();
            esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL);
            esp_deep_sleep_start();
        }
    } else {
        object_far_away = false; // Object is not far away, reset timer
        far_start_time = 0;
    }

    if (currentMillis - previousMillis >= UPLOAD_INTERVAL) {
        previousMillis = currentMillis;
        sendDataToFirebase(distance);
    }

    delay(500); // Short delay to avoid excessive looping
}
