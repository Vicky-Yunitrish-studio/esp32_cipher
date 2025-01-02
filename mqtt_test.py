import paho.mqtt.client as mqtt
from datetime import datetime
from cryptography.fernet import Fernet  # 需要安裝: pip install cryptography

# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_TEMP = "yunitrish/esp32/temperature"
MQTT_TOPIC_HUM = "yunitrish/esp32/humidity"

# Encryption settings - 使用與ESP32相同的金鑰
KEY = b'YOUR_ENCRYPTION_KEY_HERE'  # 替換成你的金鑰
cipher_suite = Fernet(KEY)

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

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribe to both temperature and humidity topics
    client.subscribe([(MQTT_TOPIC_TEMP, 0), (MQTT_TOPIC_HUM, 0)])

def on_message(client, userdata, msg):
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    topic = msg.topic
    encrypted_value = msg.payload
    
    # 使用topic作為解密參數
    value = decrypt_message(encrypted_value, topic)
    
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
