import abc

class State(abc.ABC):
    """
    State object which provides some utility functions for the individual states within the state machine.
    """

    def __init__(self):
        return

    @abc.abstractmethod
    async def on_event(self, event) -> "State":
        """
        Handle events that are delegated to this State.
        """
        pass

    @abc.abstractmethod
    async def execute(self):
        """
        Execute actions for this state.
        """
        pass

    def __repr__(self):
        """
        Leverages the __str__ method to describe the State.
        """
        return self.__str__()

    def __str__(self):
        """
        Returns the name of the State.
        """
        return self.__class__.__name__

