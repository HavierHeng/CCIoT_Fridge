from asyncio import Queue

class QueueHandler:
    """
    Bundle up the shit ton of queues synchronizing each task. e.g for callbacks or state machine execution that needs all of them.
    """
    def __init__(self, mqtt_queue: Queue, url_queue: Queue, upload_queue: Queue, weight_queue: Queue, event_queue:Queue):
        # Holds MQTT Messages to be passed into State 
        self.mqtt_queue = mqtt_queue
        # Holds signed url messages from MQTT for uploading recordings
        self.url_queue = url_queue
        # Holds weight updates from load module from MQTT
        self.weight_queue = weight_queue

        # Holds uploading jobs for uploading recordings  - Jobs include a file_path and a signed_url
        self.upload_queue = upload_queue
        # Holds state events triggered by anything for state transitions
        self.event_queue = event_queue

