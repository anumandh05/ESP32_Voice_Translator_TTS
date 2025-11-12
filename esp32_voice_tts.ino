/*
   ESP32 Voice Translator + TTS Player
   Mic: INMP441 (I2S0)
   Speaker: MAX98357A (I2S1)
   LCD: 16x2 I2C

   Server returns JSON:
   { "text": "Hello", "translated": "வணக்கம்", "audio_url": "http://<PC_IP>:5000/audio" }
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "driver/i2s.h"

const char* ssid = "REALMEC67";
const char* password = "12345678";
const char* serverIP = "10.64.29.96";   // change to your PC IP
const uint16_t serverPort = 5000;
const char* uploadPath = "/upload";
const char* audioPath = "/audio";

LiquidCrystal_I2C lcd(0x27, 16, 2);

// I2S mic pins (I2S0)
#define I2S_MIC_WS   15
#define I2S_MIC_SCK  14
#define I2S_MIC_SD   32
#define I2S_MIC_PORT I2S_NUM_0

// I2S speaker pins (I2S1)
#define I2S_SPK_BCLK 26
#define I2S_SPK_LRCL 25
#define I2S_SPK_DOUT 22
#define I2S_SPK_PORT I2S_NUM_1

// Recording configuration
const uint32_t SAMPLE_RATE = 16000;
const uint16_t RECORD_SECONDS = 3;
const uint32_t BYTES_PER_SAMPLE = 2;
const char* FILENAME = "/recording.pcm";
const char* AUDIO_FILE = "/tts.wav"; // downloaded Tamil TTS file

void i2s_mic_init() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD
  };

  i2s_driver_install(I2S_MIC_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_MIC_PORT, &pins);
}

void i2s_speaker_init() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_SPK_BCLK,
    .ws_io_num = I2S_SPK_LRCL,
    .data_out_num = I2S_SPK_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_SPK_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_SPK_PORT, &pins);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Mount SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  } else {
    Serial.println("SPIFFS mounted");
  }

  // LCD Init
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("WiFi Connecting");

  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(250);
    Serial.print(".");
    tries++;
  }

  lcd.clear();
  if(WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi OK");
    lcd.setCursor(0,1);
    lcd.print(WiFi.localIP().toString());
  } else {
    lcd.print("WiFi Failed");
    while(1) delay(1000);
  }

  i2s_mic_init();
  i2s_speaker_init();

  lcd.clear();
  lcd.print("Ready...");
  delay(1000);
}

void record_to_spiffs() {
  if(SPIFFS.exists(FILENAME)) SPIFFS.remove(FILENAME);
  File f = SPIFFS.open(FILENAME, FILE_WRITE);
  if(!f) return;

  size_t bytesToRecord = SAMPLE_RATE * RECORD_SECONDS * BYTES_PER_SAMPLE;
  uint8_t buf[1024];
  size_t bytesRead = 0, total = 0;

  unsigned long start = millis();
  while(total < bytesToRecord && (millis() - start) < (RECORD_SECONDS * 1300UL)) {
    i2s_read(I2S_MIC_PORT, buf, sizeof(buf), &bytesRead, portMAX_DELAY);
    if(bytesRead > 0) {
      f.write(buf, bytesRead);
      total += bytesRead;
    }
  }

  f.close();
  Serial.printf("Recorded %u bytes\n", (unsigned)total);
}

bool upload_and_get_reply(String &reply) {
  File f = SPIFFS.open(FILENAME, FILE_READ);
  if(!f) {
    Serial.println("No file to upload");
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String url = String("http://") + serverIP + ":" + String(serverPort) + String(uploadPath);
  http.begin(client, url);
  http.addHeader("Content-Type", "application/octet-stream");

  int code = http.sendRequest("POST", &f, f.size());
  f.close();

  if(code > 0) {
    reply = http.getString();
    http.end();
    return true;
  } else {
    Serial.printf("HTTP error: %d\n", code);
    http.end();
    return false;
  }
}

// Download Tamil TTS WAV and save to SPIFFS
bool download_tts_audio(String url) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Audio download failed: %d\n", httpCode);
    http.end();
    return false;
  }

  File f = SPIFFS.open(AUDIO_FILE, FILE_WRITE);
  if(!f) {
    Serial.println("Failed to open file for writing");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buf[512];
  int len = http.getSize();
  int total = 0;
  while (http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buf, ((size > sizeof(buf)) ? sizeof(buf) : size));
      f.write(buf, c);
      total += c;
      if (len > 0) len -= c;
    }
  }

  f.close();
  http.end();
  Serial.printf("Audio downloaded: %d bytes\n", total);
  return true;
}

// Simple WAV playback (16-bit mono)
void play_wav(const char* path) {
  File f = SPIFFS.open(path);
  if(!f) {
    Serial.println("No audio file found");
    return;
  }

  uint8_t buf[512];
  size_t bytesRead;
  // skip WAV header (first 44 bytes)
  f.seek(44);

  while(f.available()) {
    bytesRead = f.read(buf, sizeof(buf));
    size_t written;
    i2s_write(I2S_SPK_PORT, buf, bytesRead, &written, portMAX_DELAY);
  }

  f.close();
  Serial.println("Audio playback done");
}

void loop() {
  lcd.clear(); lcd.print("Recording...");
  record_to_spiffs();

  lcd.clear(); lcd.print("Uploading...");
  String reply;
  bool ok = upload_and_get_reply(reply);

  if(!ok) {
    lcd.clear(); lcd.print("Upload fail");
    delay(2000);
    return;
  }

  Serial.println("Server reply:");
  Serial.println(reply);

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, reply);
  if(err) {
    lcd.clear(); lcd.print("JSON err");
    return;
  }

  String text = doc["text"] | "";
  String translated = doc["translated"] | "";
  String audio_url = doc["audio_url"] | "";

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(text.substring(0,16));
  lcd.setCursor(0,1);
  lcd.print(translated.substring(0,16));

  Serial.println("Text: " + text);
  Serial.println("Translated: " + translated);
  Serial.println("Audio URL: " + audio_url);

  // Download TTS audio and play
  if(download_tts_audio(audio_url)) {
    play_wav(AUDIO_FILE);
  } else {
    Serial.println("Audio download failed");
  }

  delay(3000);
}
