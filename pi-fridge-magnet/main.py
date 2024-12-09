import paho.mqtt.client as mqtt
import asyncio
import json
from .states.consumption_state import ConsumptionState
from .states.stocking_state import StockingState
# from .button.button import GpioListener

# Get main event loop in main thread
loop = asyncio.get_event_loop()

class QueueHandler:
    """
    Bundle up the shit ton of queues synchronizing each task. e.g for callbacks or state machine execution that needs all of them.
    """
    def __init__(self, audio_queue, lcd_queue, mqtt_queue, upload_queue, weight_queue, event_queue):
        # Holds audio URLs for downloading and playback
        self.audio_queue = audio_queue
        # Holds text for displaying on the LCD
        self.lcd_queue = lcd_queue
        # Holds MQTT Messages to be published
        self.mqtt_queue = mqtt_queue
        # Holds uploading URLs for uploading recordings
        self.upload_queue = upload_queue
        # Holds weight from load cell
        self.weight_queue = weight_queue
        # Holds state events triggered by anything for state transitions
        self.event_queue = event_queue

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")

def tts_on_message(client, userdata, msg):
    """
    Callback for espfridge/tts
    Messages come in the format:
    {
        "audio": <signedurl>,
        "text": <display text for lcd screen>
    }
    """
    queue_handler = userdata 
    try:
        payload = json.loads(msg.payload.decode())
        # Process tts messages from espfridge/tts
        signed_url = payload.get("audio", None)
        text = payload.get("text", None)
        if signed_url:
            # Enqueue events for audio download 
            asyncio.run_coroutine_threadsafe(queue_handler.audio_queue.put(signed_url), loop)
        if text:
            # Enqueue LCD to display text 
            asyncio.run_coroutine_threadsafe(queue_handler.lcd_queue.put(text), loop)
    except Exception as e:
        print(f"Error processing message: {e}")

def weight_on_message(client, userdata, msg):
    """
    Callback for espweight/weight
    Messages come in the format:
    <weight difference>
    """
    queue_handler = userdata
    try:
        payload = float(msg.payload.decode())
        # Process weight messages
        asyncio.run_coroutine_threadsafe(queue_handler.weight_queue.put(payload), loop)
        asyncio.run_coroutine_threadsafe(queue_handler.event_queue.put("weight_updated"), loop)
    except Exception as e:
        print(f"Error processing message: {e}")


def url_on_message(client, userdata, msg):
    """
    Callback for espfridge/signedurl
    Messages come in the format:
    {
        "protocol": <http | https>,
        "hostname": <hostname e.g "fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com">,
        "path": <path to upload to e.g "/audio/item/31724ac7-5870-464a-ad1a-7ef7cff59f39.wav">,
        "query": <urlencoded query key and values> 
    }
    """
    queue_handler = userdata
    try:
        payload = msg.payload.decode()
        # Process audio upload path 
    except Exception as e:
        print(f"Error processing message: {e}")

async def handle_mqtt_event(mqtt_queue, mqtt_client):
    """
    Handle MQTT publish events
    """
    signed_url = await mqtt_queue.get()

async def handle_download_playback(audio_queue):
    """
    Handle download followed by audio playback events from audio_queue
    """
    audio_url = await audio_queue.get()

async def handle_update_lcd(lcd_queue):
    """
    Handle LCD display events from LCD Queue
    """
    # TODO: LCD Library is blocking - run in to_thread
    # TODO: Remember to task_done()
    try:
        # TODO: Handle LCD in another thread
    except asyncio.CancelledError:
        print("Cancelling LCD task")


async def handle_upload_recording(upload_queue):
    """
    Handle audio recording and uploading to S3 bucket
    """
    upload_url = await upload_queue.get()

async def cancel_tasks(tasks):
    """
    Helper method to kill all tasks
    """
    for task in tasks:
        task.cancel()
    done, _ = await asyncio.wait(tasks, return_when=asyncio.ALL_COMPLETED)

    for task in done:
        try:
            await task
        except asyncio.CancelledError:
            print(f"Task {task.get_name()} was cancelled.")
        else:
            print(f"Task {task.get_name()} completed successfully.")

async def handle_short_button_press(button_event, event_queue, tasks):
    """
    On button press, prematurely cancels all other long-running async tasks provided.
    This is useful for forcing the awaiting systems to do a state change on button press.
    Also puts a "button_pressed" state for the state to transition.
    Unsets the button event.
    """
    await button_event.wait()
    await cancel_tasks(tasks)
    event_queue.put("button_pressed")
    button_event.clear()

async def handle_long_button_press(button_event, event_queue, tasks):
    """
    On long button press, puts "button_held" state onto queue for state to transition to recipe.
    """
    await button_event.wait()
    await cancel_tasks(tasks)
    event_queue.put("button_held")
    button_event.clear()

async def main_event_loop():
    # State machine setup
    state = ConsumptionState()

    # Queues required for concurrency
    # Holds MQTT Messages to be published
    mqtt_queue = asyncio.Queue()
    # Holds audio URLs for downloading and playback
    audio_queue = asyncio.Queue()
    # Holds text for displaying on the LCD
    lcd_queue = asyncio.Queue()
    # Holds uploading URLs for uploading recordings
    upload_queue = asyncio.Queue()
    # Holds weight from MQTT
    weight_queue = asyncio.Queue()
    # Holds state events triggered by anything for state transitions
    event_queue = asyncio.Queue()

    # Short button Press Event - Has the ability to cancel all running tasks and force a state change
    short_button_event = asyncio.Event() 
    long_button_event = asyncio.Event()

    queue_handler = QueueHandler(audio_queue, lcd_queue, mqtt_queue, upload_queue, weight_queue, event_queue)
       
    # MQTT Client setup 
    mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqttc.tls_set(ca_certs="certs/root_cert_auth.crt", certfile="certs/client.crt", keyfile="certs/client.key")
    mqttc.on_connect = on_connect
    mqttc.message_callback_add("espfridge/tts", )
    mqttc.user_data_set(queue_handler)  # Pass queues in for callback to use
    mqttc.connect("a1il69uezpqouq-ats.iot.ap-southeast-1.amazonaws.com", 8883, 60)
    mqttc.loop_start()  # Runs in another thread, non-blocking

    # GPIO Button Listener 
    # button_listener = GpioListener(button_pin=17, short_button_event=short_button_event, long_button_event=long_button_event)

    # TODO: Init I2C screen add to update_lcd

    # Keep main event loop alive
    while True:
        # Perform all concurrent tasks together
        tasks = []
        tasks.append(asyncio.create_task(handle_mqtt_event(mqtt_queue, mqttc)))
        tasks.append(asyncio.create_task(handle_download_playback(audio_queue)))
        tasks.append(asyncio.create_task(handle_update_lcd(lcd_queue)))
        tasks.append(asyncio.create_task(handle_upload_recording(upload_queue)))

        # Execute state actions concurrently as well
        tasks.append(asyncio.create_task(state.execute()))

        # Start button tasks to wait on button presses if any
        button_tasks = []
        button_tasks.append(asyncio.create_task(handle_short_button_press(short_button_event, event_queue, tasks)))
        button_tasks.append(asyncio.create_task(handle_long_button_press(long_button_event, event_queue, tasks)))

        # Everything that should be done concurrently
        # All tasks should reach the end of their execution
        if tasks:
            await asyncio.gather(*tasks)

        # Process events and state changes
        if not event_queue.empty():
            event = await event_queue.get()
            state = await state.on_event(event)

if __name__ == "__main__":
    asyncio.run(main_event_loop())
