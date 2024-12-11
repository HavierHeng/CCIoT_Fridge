from base_state import State
from .stocking_state import StockingState
from .state_handlers import *
from .verify_text import *
from ..queues.queue_handler import QueueHandler
from ..audio.audio_handler import AudioHandler
import paho.mqtt.client as mqtt
from lcd.i2c_lcd import I2cLcd
import asyncio

class ConsumeSubState(State):
    """
    Default substate of consumption. In this state, it updates espfridge/consume if there is any weight changes.

    Plays back any received audio and displays changes on screen.
    """
    def __init__(self, consumption_state):
        self.consumption_state = consumption_state

    async def execute(self):
        """
        Executes tasks for this state.
        """
        # Handle MQTT messages - from Weight
        weight = await handle_weight_msg(self.consumption_state.queue_handler)

        self.consumption_state.mqtt_client.publish("espfridge/consume", weight, qos=1)

        await handle_tts_mqtt(self.consumption_state.queue_handler, self.consumption_state.audio_handler, self.consumption_state.lcd_handle)

    async def on_event(self, event):
        if event == "button_held":  
            return RecipeSubState(self.consumption_state)
        else:
            return self

class RecipeSubState(State):
    def __init__(self, consumption_state):
        self.consumption_state = consumption_state

    async def execute(self):
        self.consumption_state.mqtt_client.publish("espfridge/recipe", "Recipe!", qos=1)
        await handle_tts_mqtt(self.consumption_state.queue_handler, self.consumption_state.audio_handler, self.consumption_state.lcd_handle)

    async def on_event(self, event):
        return ConsumeSubState(self.consumption_state)

class ConsumptionState(State):
    def __init__(self, mqtt_client: mqtt.Client, queue_handler: QueueHandler, audio_handler: AudioHandler, lcd_handle: I2cLcd, sub_state = None): 
        if sub_state is None:
            sub_state = ConsumeSubState(self)
        self.sub_state: State = sub_state
        self.mqtt_client = mqtt_client
        self.queue_handler = queue_handler
        self.audio_handler = audio_handler
        self.lcd_handle = lcd_handle

    async def execute(self):
        await self.sub_state.execute()

    async def on_event(self, event):
        if event == "button_pressed":
            return StockingState(self.mqtt_client, self.queue_handler, self.audio_handler, self.lcd_handle)

        next_sub_state = await self.sub_state.on_event(event)

        # Stay in same State but update its substate
        self.sub_state = next_sub_state
        return self
