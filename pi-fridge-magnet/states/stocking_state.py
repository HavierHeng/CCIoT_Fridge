from base_state import State
from .consumption_state import ConsumptionState 
from .state_handlers import *
from .verify_text import *
from ..queues.queue_handler import QueueHandler
from ..audio.audio_handler import AudioHandler
import paho.mqtt.client as mqtt
from lcd.i2c_lcd import I2cLcd
import asyncio

class ItemSubState(State):
    """
    This state gets the name of the item from user by recording their input.
    On a valid user interaction, LCD screen with display the text and speaker plays the Speech to Text WAV file downloaded.
    This will then result in an "item_processed" transition to Expiry Substate.

    If a weight update occurs in this state, the state is transitioned to invalid substate due to invalid interaction ordering from the user.
    """
    def __init__(self, stocking_state):
        self.stocking_state = stocking_state

    async def execute(self):
        """
        Executes tasks for this state.
        """
        # Play Item Details starting audio - e.g "Please state item"
        await write_lcd(self.stocking_state.lcd_handle, "Item Details?")
        with self.stocking_state.audio_handler.player.open("item_details.wav", 'rb') as playfile:
            playfile.play()
            while playfile.is_active():
                await asyncio.sleep(0.1)

        # Handle MQTT messages - from TTS and from Weight
        tasks = []
        tasks.append(asyncio.create_task(handle_tts_mqtt(self.stocking_state.queue_handler, self.stocking_state.audio_handler, self.stocking_state.lcd_handle)))
        tasks.append(asyncio.create_task(handle_weight_msg(self.stocking_state.queue_handler)))

        while True: 
            # Wait, whoever is first to complete determines the transition 
            done, _ = await asyncio.wait(tasks, timeout=1, return_when=asyncio.FIRST_COMPLETED)

            if done:
                break
            else:
                await handle_upload_recording(self.stocking_state.queue_handler, self.stocking_state.audio_handler, self.stocking_state.mqtt_client, "espfridge/trigger/audio/item")

        first = done.pop()
        # TTS MQTT - extract relevant data and store into parent state
        if first is tasks[0]:
            msg = await first
            text = msg.get("text", None)
            if verify_item(text):
                self.stocking_state.item = text
                self.stocking_state.queue_handler.event_queue.put("item_processed")
                return
            else:
                # If invalid data for this state, just put a placeholder onto the queue to repeat state
                self.stocking_state.queue_handler.event_queue.put("repeat")
                return

        # Weight Message - queue change of state to event
        if first is tasks[1]:
            await first
            self.stocking_state.queue_handler.event_queue.put("weight_updated")
            return

    async def on_event(self, event):
        try:
            if event == "weight_updated":
                return InvalidSubState(self.stocking_state)
            elif event == "item_processed":
                return ExpirySubState(self.stocking_state)
            else:
                return self
        finally:
            self.stocking_state.queue_handler.event_queue.task_done()

class ExpirySubState(State):
    """
    This state gets the expiry of the item from user by recording their input.
    On a valid user interaction, LCD screen with display the text and speaker plays the Speech to Text WAV file downloaded.
    This will then result in an "expiry_processed" transition to Weight SubState.

    If a weight update occurs in this state, the state is transitioned to invalid substate due to invalid interaction ordering from the user.
    """
    def __init__(self, stocking_state):
        self.stocking_state = stocking_state
        return

    async def execute(self):
        """
        Executes tasks for this state. 
        """
        # Play Expiry Details starting audio - e.g "Please provide expiry"
        await write_lcd(self.stocking_state.lcd_handle, "Expiry Details?")
        with self.stocking_state.audio_handler.player.open("expiry_details.wav", 'rb') as playfile:
            playfile.play()
            while playfile.is_active():
                await asyncio.sleep(0.1)

        # Handle MQTT messages - from TTS and from Weight
        tasks = []
        tasks.append(asyncio.create_task(handle_tts_mqtt(self.stocking_state.queue_handler, self.stocking_state.audio_handler, self.stocking_state.lcd_handle)))
        tasks.append(asyncio.create_task(handle_weight_msg(self.stocking_state.queue_handler)))

        while True: 
            # Wait, whoever is first to complete determines the transition 
            done, _ = await asyncio.wait(tasks, timeout=1, return_when=asyncio.FIRST_COMPLETED)

            if done:
                break
            else:
                await handle_upload_recording(self.stocking_state.queue_handler, self.stocking_state.audio_handler, self.stocking_state.mqtt_client, "espfridge/trigger/audio/expiry")

        first = done.pop()
        # TTS MQTT - extract relevant data and store into parent state
        if first is tasks[0]:
            msg = await first
            text = msg.get("text", None)
            if verify_date(text):
                self.stocking_state.expiry = text
                self.stocking_state.queue_handler.event_queue.put("expiry_processed")
                return
            else:
                # If invalid data for this state, just put a placeholder onto the queue to repeat state
                self.stocking_state.queue_handler.event_queue.put("repeat")
                return

        # Weight Message - queue change of state to event
        if first is tasks[1]:
            await first
            self.stocking_state.queue_handler.event_queue.put("weight_updated")
            return

    async def on_event(self, event):
        if event == "weight_updated":
            return InvalidSubState(self.stocking_state) 
        elif event == "expiry_processed":
            return ExpirySubState(self.stocking_state)
        else:
            return self

class WeightSubState(State):
    """
    This state gets the weight of the item from user by recording their input.
    On a valid user interaction, LCD screen with display the text and speaker plays the WAV file downloaded.
    This will then result in an "expiry_processed" transition back to Item Substate.
    """
    def __init__(self, stocking_state):
        self.stocking_state = stocking_state

    async def execute(self):
        """
        Executes tasks for this state. 
        """
        # Play Weight starting audio - e.g "Please put item"
        await write_lcd(self.stocking_state.lcd_handle, "Put item on\nweight module.")
        with self.stocking_state.audio_handler.player.open("weight_details.wav", 'rb') as playfile:
            playfile.play()
            while playfile.is_active():
                await asyncio.sleep(0.1)

        for _ in range(3):
            try:
                # Try to get user to put item 
                weight = await asyncio.wait_for(handle_weight_msg(self.stocking_state.queue_handler), 10)
                # Handle MQTT messages - from Weight
                self.stocking_state.weight = weight

                payload = {
                    "name": self.stocking_state.name,
                    "expiry": self.stocking_state.expiry,
                    "weight": self.stocking_state.weight
                }

                # Update database 
                self.stocking_state.mqtt_client.publish("espfridge/additem", payload, qos=1)

                # Weight Message - queue change of state to event
                await self.stocking_state.queue_handler.event_queue.put("weight_processed")

                return

            except asyncio.TimeoutError:
                # Harass the user to put things onto the scale
                with self.stocking_state.audio_handler.player.open("weight_details.wav", 'rb') as playfile:
                    playfile.play()
                    while playfile.is_active():
                        await asyncio.sleep(0.1)

        # Failure - user didn't put item on within 30 seconds
        await self.stocking_state.queue_handler.event_queue.put("weight_failure")
        return
        
    async def on_event(self, event):
        if event == "weight_processed":
            return ItemSubState(self.stocking_state)
        elif event == "weight_failure":
            return InvalidSubState(self.stocking_state)
        else:
            return self

    
class InvalidSubState(State):
    """
    This state handles invalid interactions whenever weight updates during states that are updating the item or expiry.
    LCD screen with display the text and speaker plays the WAV file indictating user error.
    This will then result in an "expiry_processed" transition back to Item Substate.

    If a weight update occurs in this state, the state is transitioned to invalid substate due to invalid interaction ordering from the user.
    """
    def __init__(self, stocking_state):
        self.stocking_state = stocking_state

    async def execute(self):
        """
        Executes tasks for this state. 
        """
        # Reset all known values
        self.stocking_state.item_name = None
        self.stocking_state.item_expiry = None
        self.stocking_state.item_weight = None

        # Screen shows invalid error
        await write_lcd(self.stocking_state.lcd_handle, "Invalid\n interaction.")

        # Play invalid interaction sound
        with self.stocking_state.audio_handler.player.open("invalid_interaction.wav", 'rb') as playfile:
            playfile.play()
            while playfile.is_active():
                await asyncio.sleep(0.1)

        await self.stocking_state.queue_handler.event_queue.put("invalid_processed")

    async def on_event(self, event):
        if event == "weight_updated":  # TODO: Not implemented
            return InvalidSubState(self.stocking_state)
        elif event == "invalid_processed":
            return ItemSubState(self.stocking_state)
        else:
            return self


class StockingState(State):
    def __init__(self, mqtt_client: mqtt.Client, queue_handler: QueueHandler, audio_handler: AudioHandler, lcd_handle: I2cLcd, sub_state = None): 
        if sub_state is None:
            sub_state = ItemSubState(self)
        self.sub_state: State = sub_state
        self.mqtt_client = mqtt_client
        self.queue_handler = queue_handler
        self.audio_handler = audio_handler
        self.lcd_handle = lcd_handle

        self.item_name = None
        self.item_expiry = None
        self.item_weight = None

    async def execute(self):
        await self.sub_state.execute()

    async def on_event(self, event):
        if event == "button_pressed":
            return ConsumptionState(self.mqtt_client, self.queue_handler, self.audio_handler, self.lcd_handle)

        next_sub_state = await self.sub_state.on_event(event)

        # Stay in same Stocking State but update its substate
        self.sub_state = next_sub_state
        return self
