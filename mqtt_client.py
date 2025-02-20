import paho.mqtt.client as mqtt
import ssl
import certifi
import binascii
import time

# MQTT設定
MQTT_BROKER = "81a9af6ef0a24070877e2fdb6ce5adb9.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
MQTT_TOPIC = "duel_cipher32/#"
MQTT_USERNAME = "esp32-0002"
MQTT_PASSWORD = "Esp320002"

# 加密金鑰
ENCRYPTION_KEY = "DC3935B94F7069973B0AF683F2F219A1EF2408391254E632367944E7BDAA7B0F"

class ChaCha20:
    def __init__(self, key_hex):
        self.key = binascii.unhexlify(key_hex)
        self.counter = 0
        
    def decrypt(self, encrypted_msg):
        try:
            # 確保消息格式正確
            if not encrypted_msg:
                return None
                
            # 直接處理加密的十六進制字串
            # ciphertext = binascii.unhexlify(encrypted_msg)
            
            # 重置計數器
            self.counter = 0
            
            # 解密
            plaintext = self.decrypt(encrypted_msg)
            return plaintext
        except Exception as e:
            print(f"Decryption error: {e}")
            return None
            
def on_connect(client, userdata, flags, rc, properties=None):
    print(f"Connected with result code {rc}")
    if rc == 0:
        print(f"Subscribing to {MQTT_TOPIC}")
        client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    try:
        print("\n=== Received Message ===")
        print(f"Topic: {msg.topic}")
        payload = msg.payload.decode()
        print(f"Encrypted: {payload}")
        
        # 解密消息
        decrypted = cryptor.decrypt(payload)
        if decrypted:
            print(f"Decrypted: {decrypted}")
        print("=====================")
        
    except Exception as e:
        print(f"Error processing message: {e}")

def main():
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
        client.loop_forever()
        
    except Exception as e:
        print(f"Connection error: {e}")
        return

if __name__ == "__main__":
    # 初始化解密器
    cryptor = ChaCha20(ENCRYPTION_KEY)
    
    print("Starting MQTT client...")
    print(f"Broker: {MQTT_BROKER}")
    print(f"Port: {MQTT_PORT}")
    print(f"Topic: {MQTT_TOPIC}")
    
    main()
