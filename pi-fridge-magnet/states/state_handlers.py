from ..audio.recorder import AudioRecorder
from ..audio.player import AudioPlayer
from ..audio.audio_handler import AudioHandler
from ..queues.queue_handler import QueueHandler
from datetime import datetime
from ..awshttp.s3_operations import download_file, parse_signed_url
import asyncio
import paho.mqtt.client as mqtt
from lcd.i2c_lcd import I2cLcd
from .state_handlers import *

def get_wav_name():
    now = datetime.now()
    formatted_time = now.strftime("%Y-%m-%d_%H-%M-%S")
    file_name = f"{formatted_time}.wav"
    return file_name

async def handle_download_playback(audio_handler: AudioHandler, signed_url: str):
    parsed_signed = parse_signed_url(signed_url)
    download_file_path = parsed_signed["path"]
    await download_file(**parsed_signed, file_path=download_file_path)

    # No recording while playing
    async with audio_handler.audio_lock:
        with audio_handler.player.open(download_file_path, 'rb') as playfile:
            playfile.play()
            while playfile.is_active():
                await asyncio.sleep(0.1)

async def handle_upload_recording(queue_handler: QueueHandler, audio_handler: AudioHandler, mqtt_client: mqtt.Client, topic: str):
    recording_path = get_wav_name()

    # No playing while recording
    async with audio_handler.audio_lock:
        with audio_handler.recorder.open(recording_path, "wb") as recfile:
            recfile.start_recording()
            await asyncio.sleep(5.0)
            recfile.stop_recording()

    # When done, trigger uploading by publishing to relevant trigger to record and upload
    mqtt_client.publish(topic, "Trigger!", qos=1)

    # Wait on signedurl from upload queue
    signed_url = await queue_handler.url_queue.get()

    # Upload recording to signed URL 
    await queue_handler.upload_queue.put(
        {"file_path": recording_path,
        "signed_url": signed_url}
    )

async def handle_tts_mqtt(queue_handler: QueueHandler, audio_handler: AudioHandler, lcd_handle: I2cLcd):
    msg = await queue_handler.mqtt_queue.get()
    signed_url = msg.get("audio", None)
    text = msg.get("text", None)
    tasks = []
    if signed_url:
        tasks.append(asyncio.create_task(handle_download_playback(audio_handler, signed_url)))
    if text:
        tasks.append(asyncio.create_task(write_lcd(lcd_handle, text)))
    await asyncio.gather(*tasks)
    return msg 

async def handle_weight_msg(queue_handler: QueueHandler):
    weight = await queue_handler.weight_queue.get()
    return weight


async def write_lcd(lcd_handle: I2cLcd, text):
    # Clear display
    await asyncio.to_thread(lcd_handle.clear)
    # LCD is blocking task - write in another thread
    await asyncio.to_thread(lcd_handle.putstr, text)

