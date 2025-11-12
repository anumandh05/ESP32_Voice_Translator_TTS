from flask import Flask, request, jsonify, send_file
import wave, io, time, os
import speech_recognition as sr
from deep_translator import GoogleTranslator
from gtts import gTTS
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

@app.route("/upload", methods=["POST"])
def upload():
    try:
        raw = request.data
        if not raw:
            return jsonify({"error": "no data"}), 400

        # wrap raw PCM into WAV
        wav_buf = io.BytesIO()
        with wave.open(wav_buf, "wb") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(16000)
            wf.writeframes(raw)
        wav_buf.seek(0)

        # Speech Recognition
        r = sr.Recognizer()
        with sr.AudioFile(wav_buf) as source:
            audio = r.record(source)

        try:
            text = r.recognize_google(audio)
        except sr.UnknownValueError:
            return jsonify({"error": "speech not recognized"}), 200
        except Exception as e:
            return jsonify({"error": f"STT error: {e}"}), 500

        # Translate
        translated = GoogleTranslator(source='auto', target='ta').translate(text)
        print(f"🎙 Detected: {text}")
        print(f"🌍 Translated: {translated}")

        # TTS generation (Tamil)
        tts = gTTS(translated, lang='ta')
        tts_path = "translated_tts.mp3"
        tts.save(tts_path)

        # Return both text + URL to download audio
        return jsonify({
            "text": text,
            "translated": translated,
            "audio_url": "http://YOUR_PC_IP:5000/audio"
        }), 200

    except Exception as e:
        print("❌ Error:", e)
        return jsonify({"error": str(e)}), 500


@app.route("/audio", methods=["GET"])
def audio():
    """Serves the Tamil speech audio."""
    return send_file("translated_tts.mp3", mimetype="audio/mpeg")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
