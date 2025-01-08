import sys
import os

# Add the project root directory to Python path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from testor.mqtt.subscriber import MQTTSubscriber
from testor.gui.main_window import MQTTUI

def main():
    subscriber = MQTTSubscriber()
    ui = MQTTUI(subscriber)
    ui.start()

if __name__ == "__main__":
    main()
