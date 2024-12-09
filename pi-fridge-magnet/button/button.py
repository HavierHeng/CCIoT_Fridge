import RPi.GPIO as GPIO
import asyncio
import time

class GpioListener:
    def __init__(self, button_pin, short_button_event, long_button_event, long_press_duration=2.0):
        self.button_pin = button_pin
        self.short_button_event = short_button_event  # For dealing with state change
        self.long_button_event = long_button_event  # For dealing with state change

        self.long_press_duration = long_press_duration  # In seconds
        self.press_start_time = None

        GPIO.setmode(GPIO.BCM)
        # PI has own internal pull up/down
        GPIO.setup(self.button_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

        GPIO.add_event_detect(self.button_pin, GPIO.FALLING, callback=self.on_button_press, bouncetime=200)
        GPIO.add_event_detect(self.button_pin, GPIO.RISING, callback=self.on_button_release, bouncetime=200)

    def on_button_press(self, channel):
        """Start press duration timer when the button is pressed on falling edge"""
        self.press_start_time = time.time()

    def on_button_release(self, channel):
        """End press duration timer when the button is released on rising edge"""
        if self.press_start_time is not None:
            press_duration = time.time() - self.press_start_time
            self.handle_button_press(press_duration)
            self.press_start_time = None  # Reset for next press

    def handle_button_press(self, press_duration):
        """Handle long or short button"""
        if press_duration < self.long_press_duration:
            self.handle_short_click()
        else:
            self.handle_long_press()


    def handle_short_click(self):
        """
        When button is short pressed, inform the main coroutine by setting an Event flag.
        This should run in the main thread.
        """
        asyncio.run_coroutine_threadsafe(self.short_button_event.set(), asyncio.get_event_loop())

    def handle_long_press(self):
        """
        When button is long pressed, inform the main coroutine by setting an Event flag.
        """
        asyncio.run_coroutine_threadsafe(self.long_button_event.set(), asyncio.get_event_loop())

