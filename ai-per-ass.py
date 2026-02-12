from flask import Flask, send_file
import os
import time
from pydub import AudioSegment
from kokoro import KPipeline
import soundfile as sf
from pydub.playback import play as pydub_play
import numpy as np
import librosa
#import simpleaudio as sa



# CONFIG - fixed phrase
PHRASE = "Welcome home sir."

app = Flask(__name__)

pipeline = KPipeline(lang_code='b')

# Folder to store generated files
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_FOLDER = os.path.join(BASE_DIR, "uploads")
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

#def play_audio(file_path):
    
    # Play the audio
    #try:
        #wave_obj = sa.WaveObject.from_wave_file(file_path)
        #play_obj = wave_obj.play()
        #play_obj.wait_done()
    #except Exception as e:
    #    print(f"Error playing audio: {e}")


@app.route("/play", methods=["GET", "POST"])
def play_phrase():
    """Generate WAV from fixed PHRASE and save it"""
    response_wav = os.path.join(UPLOAD_FOLDER, "response.wav")

    # Remove old files if they exist
    for f in [response_wav]:
        if os.path.exists(f):
            os.remove(f)

    audio_data = []
    kokoro_sample_rate = 24000
    target_sample_rate = 44100
    generator = pipeline(PHRASE, voice='bm_daniel')
    for i, (gs, ps, audio) in enumerate(generator):
        audio_data.append(audio)

    full_audio = np.concatenate(audio_data)
    full_audio = librosa.resample(full_audio, orig_sr=kokoro_sample_rate, target_sr=target_sample_rate)

    file_path = UPLOAD_FOLDER + "/response.wav"

    # full_audio is shape (N,)
    stereo_audio = np.column_stack((full_audio, full_audio))


    # Save as WAV using soundfile
    sf.write(file_path, stereo_audio, target_sample_rate)

    return "OK", 200


@app.route("/response.wav")
def serve_wav():
    """Serve the generated WAV file, wait briefly if it is not ready"""
    wav_path = os.path.join(UPLOAD_FOLDER, "response.wav")
    timeout = 5.0  # maximum 5 seconds
    waited = 0.0
    while waited < timeout and not os.path.exists(wav_path):
        time.sleep(0.1)
        waited += 0.1

    if not os.path.exists(wav_path):
        return "File not found", 404
    
    #play_audio(wav_path)
    return send_file(wav_path, mimetype="audio/wav")


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)

