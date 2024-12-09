from base_state import State
from .consumption_state import ConsumptionState 
import asyncio
from ..audio.recorder import AudioRecorder
from ..audio.player import AudioPlayer
from datetime import datetime
from ..awshttp.s3_operations import upload_file, download_file

recorder = AudioRecorder()
player = AudioPlayer()

def get_wav_name():
    now = datetime.now()
    formatted_time = now.strftime("%Y-%m-%d_%H-%M-%S")
    file_name = f"{formatted_time}.wav"
    return file_name

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
            self.stocking_state.event_queue.task_done()

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

    def execute(self):
        """
        Executes tasks for this state. 
        """
        return

    async def on_event(self, event):
        if event == "weight_updated":
            return InvalidSubState(self.stocking_state)
        elif event == "weight_processed":
            # TODO: Clear out all stored variables
            return ItemSubState(self.stocking_state)
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

        # Play invalid interaction sound

        # Screen shows invalid error
        return

    async def on_event(self, event):
        if event == "weight_updated":
            return InvalidSubState(self.stocking_state)
        elif event == "item_processed":
            return ItemSubState(self.stocking_state)
        else:
            return self


class StockingState(State):
    def __init__(self, mqtt_client, queue_handler, sub_state = None): 
        if sub_state is None:
            sub_state = ItemSubState(self)
        self.sub_state: State = sub_state
        self.mqtt_client = mqtt_client
        self.queue_handler = queue_handler
        self.item_name = None
        self.item_expiry = None
        self.item_weight = None

    async def execute(self):
        await self.sub_state.execute()

    async def on_event(self, event):
        if event == "button_pressed":
            return ConsumptionState(self.mqtt_client, self.queue_handler)

        next_sub_state = await self.sub_state.on_event(event)

        # Stay in same Stocking State but update its substate
        self.sub_state = next_sub_state
        return self


