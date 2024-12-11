import paho.mqtt.client as mqtt
import asyncio
import json
from .states.consumption_state import ConsumptionState
from .button.button import GpioListener
from awshttp.s3_operations import upload_file
from .queues.queue_handler import QueueHandler
from .audio.audio_handler import AudioHandler
from lcd.i2c_lcd import I2cLcd

# Get main event loop in main thread
loop = asyncio.get_event_loop()

def on_connect(client, userdata, flags, reason_code, properties):
    """
    Callback for generic MQTT connect event.
    """
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
        asyncio.run_coroutine_threadsafe(queue_handler.mqtt_queue.put(payload), loop)
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
        # Process weight messages - This is handled in state
        asyncio.run_coroutine_threadsafe(queue_handler.weight_queue.put(payload), loop)
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
        asyncio.run_coroutine_threadsafe(queue_handler.url_queue.put(payload), loop)
    except Exception as e:
        print(f"Error processing message: {e}")


async def upload_task(upload_queue):
    """
    Handle audio uploading to S3 bucket. Runs as a permanent task in the background to wait for stuff to upload.

    Upload queue jobs come as a dictionary in the following format:
    {
        "file_path": <recording to upload>,
        "signed_url": {
            "protocol": <http | https>,
            "hostname": <hostname e.g "fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com">,
            "path": <path to upload to e.g "/audio/item/31724ac7-5870-464a-ad1a-7ef7cff59f39.wav">,
            "query": <urlencoded query key and values> 
        }
    }
    """
    while True:
        upload_job = await upload_queue.get()
        await upload_file(**upload_job["signed_url"], file_path=upload_job["file_path"])

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
    # Queues required for concurrency
    # Holds incoming MQTT messages into its own queue
    mqtt_queue = asyncio.Queue()
    # Holds signed URLs from MQTT for uploading recordings
    url_queue = asyncio.Queue() 
    # Holds weight updates from load module from MQTT
    weight_queue = asyncio.Queue()

    # Holds uploading jobs for uploading recordings  - Jobs include a file path and a upload url
    upload_queue = asyncio.Queue()
    # Holds state events triggered by anything for state transitions
    event_queue = asyncio.Queue()

    # Short button Press Event - Has the ability to cancel all running tasks and force a state change
    short_button_event = asyncio.Event() 
    long_button_event = asyncio.Event()

    queue_handler = QueueHandler(mqtt_queue=mqtt_queue, url_queue=url_queue, upload_queue=upload_queue, weight_queue=weight_queue, event_queue=event_queue)

    audio_handler = AudioHandler()

    # MQTT Client setup 
    mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqttc.tls_set(ca_certs="certs/root_cert_auth.crt", certfile="certs/client.crt", keyfile="certs/client.key")
    mqttc.on_connect = on_connect
    mqttc.message_callback_add("espfridge/tts", )
    mqttc.user_data_set(queue_handler)  # Pass queues in for callback to use
    mqttc.connect("a1il69uezpqouq-ats.iot.ap-southeast-1.amazonaws.com", 8883, 60)
    mqttc.loop_start()  # Runs in another thread, non-blocking

    # GPIO Button Listener 
    button_listener = GpioListener(button_pin=17, short_button_event=short_button_event, long_button_event=long_button_event)

    # Initialize 2x16 LCD Screen with PCF8574 backpack on I2C address 0x27
    # On 40-pin Pi, Port 1 (I2C 1) is GPIO 2 and 3
    lcd_handle = I2cLcd(1, 0x27, 2, 16)

    # State machine setup
    state = ConsumptionState(mqtt_client=mqttc, lcd_handle = lcd_handle, queue_handler=queue_handler, audio_handler=audio_handler)

    # Init upload tasks - in case there are simultaneous recordings that are queued/backed up
    UPLOAD_TASKS = 1
    upload_tasks = [asyncio.create_task(upload_task(upload_queue)) for _ in range(UPLOAD_TASKS)]

    # Main event loop
    while True:
        # Perform all concurrent tasks together
        tasks = []
        # Execute state actions concurrently 
        tasks.append(asyncio.create_task(state.execute()))

        # Start button tasks to wait on button presses if any
        button_tasks = []
        button_tasks.append(asyncio.create_task(handle_short_button_press(short_button_event, event_queue, tasks)))
        button_tasks.append(asyncio.create_task(handle_long_button_press(long_button_event, event_queue, tasks)))

        # Everything that should be done concurrently
        # All tasks should reach the end of their execution before state changes
        if tasks:
            await asyncio.gather(*tasks)

        # Process events and state changes
        if not event_queue.empty():
            event = await event_queue.get()
            state = await state.on_event(event)

if __name__ == "__main__":
    asyncio.run(main_event_loop())
