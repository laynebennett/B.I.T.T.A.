#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include "BluetoothA2DPSource.h"

const char* ssid = "TP-Link_9BB8";
const char* password = "Perchance...";
const char* fileURL  = "http://192.168.0.103:8080/response.wav";
const char* filePath = "/response.wav";
const char* btDeviceName = "JBL Flip 4";

BluetoothA2DPSource a2dp_source;
WebServer server(80);  // Web server on port 80
File audioFile;
bool playbackDone = false;
bool shouldPlay = false;

bool downloadFile(const char* url, const char* path) {
    HTTPClient http;
    http.setTimeout(10000);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();
        File file = SPIFFS.open(path, FILE_WRITE);
        if (!file) { http.end(); return false; }
        uint8_t buff[1024];
        int len = http.getSize(), total = 0;
        while (http.connected() && (len > 0 || len == -1)) {
            size_t avail = stream->available();
            if (avail) {
                int c = stream->readBytes(buff, min(avail, sizeof(buff)));
                file.write(buff, c);
                total += c;
                if (len > 0) len -= c;
            }
            delay(1);
        }
        Serial.printf("Downloaded: %d bytes\n", total);
        file.close();
        http.end();
        return true;
    } else {
        Serial.printf("HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
}

int32_t get_sound_data(uint8_t *data, int32_t len) {
    if (playbackDone || !shouldPlay) return 0;
    
    if (!audioFile) {
        audioFile = SPIFFS.open(filePath, "r");
        if (!audioFile) { 
            Serial.println("Error opening audio file"); 
            playbackDone = true; 
            shouldPlay = false;
            return 0; 
        }
        audioFile.seek(44, SeekSet);
        Serial.println("Started playback");
    }
    
    int bytesRead = audioFile.read(data, len);
    
    if (bytesRead <= 0) { 
        Serial.println("Playback finished!"); 
        playbackDone = true; 
        shouldPlay = false;
        audioFile.close(); 
        return 0; 
    }
    
    return bytesRead;
}

void restartPlayback() {
    if (audioFile) {
        audioFile.close();
    }
    playbackDone = false;
    shouldPlay = true;
    Serial.println("Playback triggered");
}

// Web server handlers
void handlePlay() {
    Serial.println("Web request: Play audio");
    restartPlayback();
    server.send(200, "text/plain", "Playing audio");
}

void handleDownloadAndPlay() {
    Serial.println("Web request: Download and play");
    
    // Optional: get custom URL from query parameter
    String url = server.hasArg("url") ? server.arg("url") : fileURL;
    
    if (downloadFile(url.c_str(), filePath)) {
        restartPlayback();
        server.send(200, "text/plain", "Downloaded and playing");
    } else {
        server.send(500, "text/plain", "Download failed");
    }
}

void handleStatus() {
    String status = "{\n";
    status += "  \"bluetooth\": \"" + String(a2dp_source.is_connected() ? "connected" : "disconnected") + "\",\n";
    status += "  \"playing\": \"" + String(shouldPlay && !playbackDone ? "yes" : "no") + "\",\n";
    status += "  \"wifi\": \"connected\"\n";
    status += "}";
    server.send(200, "application/json", status);
}

void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>ESP32 Audio Controller</title></head><body>";
    html += "<h1>ESP32 Audio Controller</h1>";
    html += "<p>Bluetooth: " + String(a2dp_source.is_connected() ? "Connected" : "Disconnected") + "</p>";
    html += "<button onclick=\"fetch('/play')\">Play Current Audio</button><br><br>";
    html += "<button onclick=\"fetch('/download-and-play')\">Download & Play</button><br><br>";
    html += "<button onclick=\"fetch('/status').then(r=>r.json()).then(d=>alert(JSON.stringify(d)))\">Check Status</button>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, -1); // RX = 16, TX unused
    delay(1000);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500); 
        Serial.print("."); 
    }
    Serial.println(" Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return;
    }

    // Connect to Bluetooth FIRST
    Serial.println("Initializing Bluetooth...");
    a2dp_source.set_data_callback(get_sound_data);
    a2dp_source.start(btDeviceName);
    
    // Wait for Bluetooth connection
    for (int i = 0; i < 10; i++) {
        delay(1000);
        if (a2dp_source.is_connected()) {
            Serial.println("✓ Bluetooth connected!");
            break;
        }
        Serial.print(".");
    }
    
    if (!a2dp_source.is_connected()) {
        Serial.println("✗ Bluetooth failed to connect");
    }

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/play", handlePlay);
    server.on("/download-and-play", handleDownloadAndPlay);
    server.on("/status", handleStatus);
    
    server.begin();
    Serial.println("Web server started");
    Serial.printf("Visit http://%s in your browser\n", WiFi.localIP().toString().c_str());
}

void loop() {
    server.handleClient();  // Handle web requests

    while (Serial2.available()) {
        char c = Serial2.read();
        //Serial.printf("%c\n", c);
        if (c == 'a') {
            Serial.println("Trigger received!");
            handlePlay();
        }
  }
}