from base_state import State
from .stocking_state import StockingState

class ConsumeSubState(State):
    def __init__(self, consumption_state):
        self.consumption_state = consumption_state

    async def on_event(self, event):
        return RecipeSubState(self.consumption_state)

class WeightSubState(State):
    """
    State that is entered when there is a weight change
    """

class RecipeSubState(State):
    def __init__(self, consumption_state):
        self.consumption_state = consumption_state

    async def on_event(self, event):
        return ConsumeSubState(self.consumption_state)

class ConsumptionState(State):
    def __init__(self, mqtt_client, queue_handler, sub_state = None):
        if sub_state is None:
            sub_state = ConsumeSubState(self)
        self.sub_state: State = sub_state
        self.mqtt_client = mqtt_client
        self.queue_handler = queue_handler

    async def on_event(self, event):
        if event == "button_pressed":
            return StockingState(self.mqtt_client, self.queue_handler)
        await self.sub_state.on_event(event)
        return self

