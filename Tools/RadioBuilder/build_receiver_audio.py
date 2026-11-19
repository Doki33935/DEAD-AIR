"""Deterministic local radio interference and STFT of the supplied temporary VO.
No external recordings. Re-run after replacing RadioSignal.wav.
"""
from pathlib import Path
import json
import wave
import numpy as np

BASE = Path(__file__).resolve().parent
AUDIO = BASE / 'Audio'
GENERATED = BASE.parent.parent / 'nto_dead_air' / 'Saved' / 'ReceiverGenerated'

def build():
    GENERATED.mkdir(parents=True, exist_ok=True)
    rate, seconds = 22050, 8
    count = rate * seconds
    rng = np.random.default_rng(271999)
    t = np.arange(count) / rate
    # Sum interpolated random octaves: broad / low frequency structures, not a pure sine.
    fractal = np.zeros(count)
    for octave in range(10):
        knots = 2 ** (octave + 3)
        values = rng.standard_normal(knots + 1); values[-1] = values[0]
        fractal += np.interp(np.arange(count) * knots / count, np.arange(knots + 1), values) / (1.6 ** octave)
    white = rng.standard_normal(count)
    high = white - np.convolve(white, np.ones(13) / 13, mode='same')
    burst = np.zeros(count)
    for point in rng.integers(0, count - 400, 110):
        length = int(rng.integers(20, 350))
        burst[point:point + length] += rng.standard_normal(length) * np.exp(-np.arange(length) / (length / 6))
    layers = {'ReceiverFractal': fractal, 'ReceiverHiss': high * (.5 + .5*np.abs(fractal)), 'ReceiverCrackle': burst}
    for name, signal in layers.items():
        signal = np.tanh(signal * .75)
        # Quiet crossings at the loop boundary prevent a manufactured loop click.
        signal[:256] *= np.linspace(0, 1, 256); signal[-256:] *= np.linspace(1, 0, 256)
        signal *= .45 / max(np.max(np.abs(signal)), .01)
        with wave.open(str(AUDIO / (name + '.wav')), 'wb') as out:
            out.setnchannels(1); out.setsampwidth(2); out.setframerate(rate)
            out.writeframes((signal * 32767).astype('<i2').tobytes())
    with wave.open(str(AUDIO / 'RadioSignal.wav'), 'rb') as src:
        sr = src.getframerate(); channels = src.getnchannels()
        pcm = np.frombuffer(src.readframes(src.getnframes()), dtype='<i2').astype(np.float64).reshape(-1, channels).mean(axis=1)/32768
    with wave.open(str(AUDIO / 'RadioSignal.wav'), 'rb') as src:
        reverse_params = src.getparams()
        reverse_frames = np.frombuffer(src.readframes(src.getnframes()), dtype='<i2').reshape(-1, src.getnchannels())[::-1].copy()
    with wave.open(str(AUDIO / 'TapeReverse.wav'), 'wb') as out:
        out.setparams(reverse_params)
        out.writeframes(reverse_frames.tobytes())
    width, height, window = 1024, 128, 1024
    image = np.zeros((height, width, 4), dtype=np.uint8); image[:,:,3] = 255
    padded = np.pad(pcm, (window//2, window//2), mode='wrap')
    frequencies = np.geomspace(90, min(8000, sr*.48), height)[::-1]
    bins = np.fft.rfftfreq(window, 1/sr)
    for col in range(width):
        at = int(col * len(pcm)/width)
        spectrum = np.abs(np.fft.rfft(padded[at:at+window] * np.hanning(window))) / window
        intensity = np.clip((20*np.log10(np.maximum(np.interp(frequencies,bins,spectrum),1e-7)) + 78)/62,0,1)
        image[:,col,0] = (intensity*255).astype(np.uint8)
    (GENERATED/'spectrogram.rgba').write_bytes(image.tobytes())
    rms = np.array([np.sqrt(np.mean(pcm[i:i+int(sr*.02)]**2)) for i in range(0,len(pcm),int(sr*.02))])
    silence=[]; start=None
    for i,v in enumerate(rms):
        if v<.005 and start is None:start=i*.02
        if v>=.005 and start is not None:
            if i*.02-start>.16:silence.append([round(start,2),round(i*.02,2)])
            start=None
    print(json.dumps({'duration':len(pcm)/sr,'silences':silence,'spectrogram':[width,height],'noise_layers':list(layers)},ensure_ascii=False))

if __name__ == '__main__':
    build()
