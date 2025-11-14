from flask import Flask, request, jsonify
import speech_recognition as sr
from googletrans import Translator
import wave

app = Flask(__name__)

UPLOAD_FILE = "upload.pcm"
WAV_FILE = "upload.wav"

translator = Translator()


# ------------------------------------------------------
# STYLE 2 : NATURAL, SIMPLE, WHATSAPP-LIKE TANGLISH
# ------------------------------------------------------
def tamil_to_tanglish_style2(text):

    # Base letter mapping (simple)
    base = {
        "அ": "a", "ஆ": "aa", "இ": "i", "ஈ": "ee",
        "உ": "u", "ஊ": "oo", "எ": "e", "ஏ": "ae",
        "ஐ": "ai", "ஒ": "o", "ஓ": "oo", "ஔ": "au",

        "க": "ka", "ச": "sa", "ட": "da", "த": "tha", "ப": "pa",
        "ம": "ma", "ய": "ya", "ர": "ra", "ல": "la", "வ": "va",
        "ழ": "zha", "ள": "la", "ற": "ra",

        "ங": "nga", "ஞ": "nja", "ண": "na", "ந": "na", "ன": "na",
        "ஹ": "ha", "ஷ": "sha", "ஸ": "sa",

        "ா": "a", "ி": "i", "ீ": "ee", "ு": "u", "ூ": "oo",
        "ெ": "e", "ே": "ae", "ை": "ai", "ோ": "oo", "ௌ": "au",

        "்": ""  # pulli silent
    }

    # Step 1: Basic transliteration
    raw = ""
    for ch in text:
        if ch == "ம்":  # special handling
            raw += "m"
            continue
        raw += base.get(ch, ch)

    # Step 2: Natural simplification rules (STYLE 2)
    simp = raw

    # Convert strong sounds → soft sounds
    simp = simp.replace("tha", "tha")     # stay same
    simp = simp.replace("dha", "da")      # soften
    simp = simp.replace("ee", "i")        # ee → i (optional)
    simp = simp.replace("ae", "e")        # ae → e

    # Common Tamil → Tanglish simplifications
    simp = simp.replace("irukkenga", "irukenga")
    simp = simp.replace("irukkinga", "irukenga")
    simp = simp.replace("eppadi", "epdi")
    simp = simp.replace("appadi", "apdi")

    # Cleanup duplicated characters
    while "aaa" in simp:
        simp = simp.replace("aaa", "aa")
    while "ooa" in simp:
        simp = simp.replace("ooa", "oo")

    return simp.strip()



# ------------------------------------------------------
# PCM → WAV Conversion
# ------------------------------------------------------
def pcm_to_wav(pcm_file, wav_file, channels=1, sample_rate=16000, byte_width=2):
    with open(pcm_file, "rb") as pcm:
        pcm_data = pcm.read()

    with wave.open(wav_file, "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(byte_width)
        wav.setframerate(sample_rate)
        wav.writeframes(pcm_data)



# ------------------------------------------------------
# UPLOAD API
# ------------------------------------------------------
@app.route("/upload", methods=["POST"])
def upload_audio():
    try:
        # Save PCM file
        with open(UPLOAD_FILE, "wb") as f:
            f.write(request.data)

        # Convert raw PCM → WAV 16kHz
        pcm_to_wav(UPLOAD_FILE, WAV_FILE)

        # Recognize speech
        recognizer = sr.Recognizer()
        with sr.AudioFile(WAV_FILE) as source:
            audio = recognizer.record(source)

        try:
            text = recognizer.recognize_google(audio)
        except:
            text = ""

        if text.strip() == "":
            return jsonify({"text": "", "translated": ""})

        # Translate English → Tamil
        tamil = translator.translate(text, dest="ta").text

        # Tamil → Tanglish Style 2
        tanglish = tamil_to_tanglish_style2(tamil)

        return jsonify({
            "text": text,
            "translated": tanglish
        })

    except Exception as e:
        return jsonify({"error": str(e)})



# ------------------------------------------------------
# START SERVER
# ------------------------------------------------------
if __name__ == "__main__":
    print("\nVoice Translate Server Running on PORT 5000...\n")
    app.run(host="0.0.0.0", port=5000, debug=False)
