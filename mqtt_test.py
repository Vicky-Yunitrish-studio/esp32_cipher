import paho.mqtt.client as mqtt
from datetime import datetime

# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_TEMP = "yunitrish/esp32/temperature"
MQTT_TOPIC_HUM = "yunitrish/esp32/humidity"

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribe to both temperature and humidity topics
    client.subscribe([(MQTT_TOPIC_TEMP, 0), (MQTT_TOPIC_HUM, 0)])

def on_message(client, userdata, msg):
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    topic = msg.topic
    value = msg.payload.decode()
    
    if topic == MQTT_TOPIC_TEMP:
        print(f"[{now}] Temperature: {value}°C")
    elif topic == MQTT_TOPIC_HUM:
        print(f"[{now}] Humidity: {value}%")

def main():
    # Create MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        # Connect to MQTT broker
        print(f"Connecting to MQTT broker at {MQTT_BROKER}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        
        # Start the loop
        print("Waiting for messages... (Press Ctrl+C to stop)")
        client.loop_forever()
        
    except KeyboardInterrupt:
        print("\nDisconnecting from broker")
        client.disconnect()
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
