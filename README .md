# 🎙️ ESP32 Voice Translator with TTS

This project allows an ESP32 to record human speech using an INMP441 I2S microphone, send it to a Python Flask server for **Speech-to-Text (STT)** and **Text-to-Speech (TTS)** translation, and play back the translated Tamil voice using a MAX98357A amplifier.

---

## 🧩 Hardware Used
- ESP32 Dev Board  
- INMP441 Microphone (I2S0)
- MAX98357A Speaker Amplifier (I2S1)
- 16x2 I2C LCD Display
- Breadboard & Jumper Wires

---

## ⚙️ Pin Connections

| Component | ESP32 Pin | Function |
|------------|------------|----------|
| INMP441 | WS = 15 | Word Select |
|          | SCK = 14 | Bit Clock |
|          | SD = 32 | Serial Data |
| MAX98357A | DIN = 22 | Data Input |
|            | BCLK = 26 | Bit Clock |
|            | LRC = 25 | LR Clock |
| LCD (I2C) | SDA = 21 | I2C Data |
|            | SCL = 22 | I2C Clock |

---

## 💻 Setup Instructions

### 1. ESP32 Side
- Open `esp32_voice_tts.ino` in Arduino IDE.
- Install required libraries:
  - `LiquidCrystal_I2C`
  - `ArduinoJson`
  - `HTTPClient`
  - `WiFi`
  - `SPIFFS`
- Update your Wi-Fi credentials in the code.
- Change `serverIP` to your computer’s IP.
- Upload to ESP32.

### 2. Python Server
Install dependencies:
```bash
pip install flask flask-cors speechrecognition deep-translator gTTS pydub

