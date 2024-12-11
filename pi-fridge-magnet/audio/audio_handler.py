import asyncio
from .player import AudioPlayer
from .recorder import AudioRecorder

class AudioHandler:
    """
    Bundle up the audio recorder and player as well as an asyncio Event that serves as a flag to indicate if an audio operation is currently active
    """
    def __init__(self):
        self.recorder = AudioRecorder()
        self.player = AudioPlayer()
        self.audio_lock = asyncio.Lock()

