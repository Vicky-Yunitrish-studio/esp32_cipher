import paho.mqtt.client as mqtt
from datetime import datetime
from cryptography.fernet import Fernet  # 需要安裝: pip install cryptography
import argparse

# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883

class MQTTGroupSubscriber:
    def __init__(self, group_name):
        self.group_name = group_name
        self.base_topic = f"esp32/{group_name}/+"
        self.temp_topic = f"{self.base_topic}/temp"
        self.hum_topic = f"{self.base_topic}/hum"
        
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
    def on_connect(self, client, userdata, flags, rc):
        print(f"Connected with result code {rc}")
        print(f"Subscribing to group: {self.group_name}")
        self.client.subscribe([(self.temp_topic, 0), (self.hum_topic, 0)])

    def on_message(self, client, userdata, msg):
        try:
            now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            topic_parts = msg.topic.split('/')
            if len(topic_parts) != 4:  # esp32/group/mac/type
                return
                
            group = topic_parts[1]
            if group != self.group_name:  # 額外的群組檢查
                return
                
            device_mac = topic_parts[2]
            measurement_type = topic_parts[3]
            
            encrypted_value = msg.payload
            value = decrypt_message(encrypted_value, msg.topic)
            
            print(f"[{now}] Group: {group}, Device: {device_mac}, {measurement_type}: {value}")
            
        except Exception as e:
            print(f"Error processing message: {e}")

    def start(self):
        try:
            print(f"Connecting to MQTT broker at {MQTT_BROKER}...")
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            print(f"Listening for messages in group '{self.group_name}'... (Press Ctrl+C to stop)")
            self.client.loop_forever()
        except KeyboardInterrupt:
            print("\nDisconnecting from broker")
            self.client.disconnect()
        except Exception as e:
            print(f"Error: {e}")

def generate_constants(topic):
    # 與ESP32端相同的常數生成邏輯
    hash_values = [0x61707865, 0x3320646e, 0x79622d32, 0x6b206574]
    for i, c in enumerate(topic):
        hash_values[i % 4] ^= (ord(c) << ((i % 4) * 8))
    return hash_values

def decrypt_message(encrypted_msg, topic):
    try:
        # 根據topic更新解密參數
        constants = generate_constants(topic)
        # 這裡需要實現對應的解密邏輯
        return decrypt_with_constants(encrypted_msg, constants)
    except:
        return encrypted_msg

def main():
    parser = argparse.ArgumentParser(description='MQTT Group Subscriber')
    parser.add_argument('--group', default='group1',
                      help='Group name to subscribe to (default: group1)')
    args = parser.parse_args()

    subscriber = MQTTGroupSubscriber(args.group)
    subscriber.start()

if __name__ == "__main__":
    main()
