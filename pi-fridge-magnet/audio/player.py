import pyaudio
import wave

'''
Provides WAV playing functionality. Play is non-blocking - using another thread as per PyAudio stream documentation.

Non-blocking mode (play):
>>> player = AudioPlayer(channels=2)
>>> with player.open('nonblocking.wav', 'rb') as playfile:
...     playfile.play()

'''
import pyaudio
import wave

class AudioPlayer(object):
    '''
    A Player class for playing WAV file.
    Plays stereo WAV files at 44100Hz sample rate with 16 bit depth by default.
    '''
    def __init__(self, channels=2, rate=44100, frames_per_buffer=1024):
        self.channels = channels
        self.rate = rate
        self.frames_per_buffer = frames_per_buffer

    def open(self, fname, mode='wb'):
        return PlayingFile(fname, mode, self.channels, self.rate,
                            self.frames_per_buffer)


class PlayingFile(object):
    def __init__(self, fname, mode, channels, 
                rate, frames_per_buffer):
        self.fname = fname
        self.mode = mode
        self.channels = channels
        self.rate = rate
        self.frames_per_buffer = frames_per_buffer
        self._pa = pyaudio.PyAudio()
        self.wavefile = self._prepare_file(self.fname, self.mode)
        self._stream = None

    def __enter__(self):
        return self

    def __exit__(self, exception, value, traceback):
        self.close()

    def play(self):
        # Use a stream with a callback in non-blocking mode
        self._stream = self._pa.open(format=pyaudio.paInt16,
                                        channels=self.wavefile.getnchannels(),
                                        rate=self.wavefile.getframerate(),
                                        output=True,
                                        frames_per_buffer=self.frames_per_buffer,
                                        stream_callback=self.get_callback())
        self._stream.start_stream()
        return self

    def is_active(self):
        return self._stream.is_active()

    def get_callback(self):
        def callback(in_data, frame_count, time_info, status):
            self.wavefile.readframes(in_data)
            return in_data, pyaudio.paContinue
        return callback

    def close(self):
        self._stream.close()
        self._pa.terminate()
        self.wavefile.close()

    def _prepare_file(self, fname, mode='wb'):
        wavefile = wave.open(fname, mode)
        return wavefile
