import paho.mqtt.client as mqtt
import ssl
import certifi
import binascii
from chacha20 import ChaCha20

# MQTT設定
MQTT_BROKER = "81a9af6ef0a24070877e2fdb6ce5adb9.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_TOPIC = "duel_cipher32/#"
MQTT_USERNAME = "esp32-0002"
MQTT_PASSWORD = "Esp320002"

# 加密金鑰
ENCRYPTION_KEY = "DC3935B94F7069973B0AF683F2F219A1EF2408391254E632367944E7BDAA7B0F"

def extract_mac_from_topic(topic):
    # 從 topic (duel_cipher32/test_group/34865DB6DF94/test) 提取 MAC
    parts = topic.split('/')
    if len(parts) >= 4:
        return parts[2]
    return None

def on_connect(client, userdata, flags, rc, properties=None):
    print(f"\nConnected with result code: {rc}")
    client.subscribe(MQTT_TOPIC)
    print(f"Subscribed to: {MQTT_TOPIC}")
    print(f"Using encryption key: {ENCRYPTION_KEY}")

def on_message(client, userdata, msg):
    try:
        print("\n=== Received MQTT Message ===")
        print(f"Topic: {msg.topic}")
        
        # 從 topic 提取 MAC 地址
        device_mac = extract_mac_from_topic(msg.topic)
        if not device_mac:
            print("Could not extract MAC from topic")
            return
            
        encrypted_hex = msg.payload.decode()
        print(f"Device MAC: {device_mac}")
        print(f"Encrypted (HEX): {encrypted_hex}")
        
        if encrypted_hex:
            # 使用提取的 MAC 初始化解密器
            key_bytes = binascii.unhexlify(ENCRYPTION_KEY)
            cryptor.init(device_mac, key_bytes, len(key_bytes))
            
            # 重置計數器確保同步
            cryptor.reset_counter()
            
            # 解密訊息
            decrypted = cryptor.decrypt(encrypted_hex)
            if decrypted:
                print(f"Decrypted: {decrypted}")
            else:
                print("Failed to decrypt message")
        print("=====================")
        
    except Exception as e:
        print(f"Message processing error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("Starting MQTT Client...")
    
    # 初始化解密器
    cryptor = ChaCha20()
    
    # 創建 MQTT client
    client = mqtt.Client(protocol=mqtt.MQTTv5)
    
    # 設定 TLS
    client.tls_set(
        ca_certs=certifi.where(),
        tls_version=ssl.PROTOCOL_TLSv1_2
    )
    client.tls_insecure_set(True)
    
    # 設定認證
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    # 設定回調
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        print(f"Connecting to {MQTT_BROKER}...")
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        print("Starting MQTT loop...")
        client.loop_forever()
        
    except Exception as e:
        print(f"Connection error: {e}")
        import traceback
        traceback.print_exc()
