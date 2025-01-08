import paho.mqtt.client as mqtt
import ssl
import certifi
from datetime import datetime
import threading
import queue
from threading import Thread, Lock
from collections import deque
import json
import time
from testor.utils.config import *
from testor.utils.encryption import Encryption  # 修改這行

class MQTTSubscriber:
    def __init__(self):
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.tls_set(
            ca_certs=certifi.where(),
            tls_version=ssl.PROTOCOL_TLSv1_2
        )
        self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_subscribe = self.on_subscribe
        self.client.on_disconnect = self.on_disconnect
        
        self.message_queue = queue.Queue()
        self.data_lock = Lock()
        self.device_data = {}
        self.data_history = {}
        self.history_length = 50
        self.decryptors = {mac: Encryption() for mac in DEVICE_MACS}
        self.default_decryptor = Encryption()  # 使用 Encryption 而不是 Decryption
        self.running = False
        self.data_queue = None
        self.thread = None
        self.logger = print  # Default logger
        
        # Move initialization messages to logger
        self.logger(f"Starting MQTT subscriber for group: {GROUP_NAME}")
        self.logger(f"Monitoring devices: {DEVICE_MACS}")
        self.logger(f"Monitoring topics: duel_cipher32/{GROUP_NAME}/#")

    def set_queue(self, queue):
        self.data_queue = queue

    def set_logger(self, logger_func):
        """Set the logger function"""
        self.logger = logger_func

    def on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self.logger("MQTT Connection status: 0")
            topic = f"duel_cipher32/{GROUP_NAME}/#"
            client.subscribe(topic)
            self.logger(f"Subscribed to: {topic}")
        else:
            self.logger(f"Failed to connect, return code {rc}", "ERROR")

    def process_message(self, topic, payload):
        """Process and decrypt the received message"""
        try:
            topic_parts = topic.split('/')
            if len(topic_parts) < 4:
                raise ValueError("Invalid topic format")
            
            device_mac = topic_parts[2]  # Extract device MAC from topic
            message_type = topic_parts[3]
            
            # Decrypt and process data...
            decrypted_data = self.default_decryptor.decrypt(payload)
            
            # Add device_mac to the returned data
            result = {'device_mac': device_mac}
            
            if message_type == 'temperature':
                result['temperature'] = float(decrypted_data)
            elif message_type == 'humidity':
                result['humidity'] = float(decrypted_data)
            
            return result
            
        except Exception as e:
            raise Exception(f"Error processing message: {str(e)}")

    def on_message(self, client, userdata, msg):
        try:
            if self.data_queue is None:
                return
            
            self.data_lock.acquire()
            try:
                payload = msg.payload.decode()
                self.logger(f"Received message:")
                self.logger(f"Topic: {msg.topic}")
                self.logger(f"Payload: {payload}")
                
                # Process and decrypt the message
                data = self.process_message(msg.topic, payload)
                self.logger(f"Processed data: {data}")
                
                # Extract device MAC from topic
                device_mac = msg.topic.split('/')[2]
                
                # Put the data in queue for UI update
                if data:  # Only send if we have valid data
                    self.data_queue.put({
                        'type': 'sensor',
                        'temperature': data.get('temperature'),
                        'humidity': data.get('humidity'),
                        'device_mac': device_mac,  # Add device MAC
                        'timestamp': datetime.now()
                    })
                
                # Update status with device info
                self.data_queue.put({
                    'type': 'status',
                    'connection': 'Connected',
                    'device': device_mac,
                    'last_message': str(data)
                })

            finally:
                self.data_lock.release()

        except Exception as e:
            self.logger(f"Error processing message: {str(e)}", "ERROR")
            if self.data_queue is not None:
                self.data_queue.put({
                    'type': 'log',
                    'message': f"Error processing message: {str(e)}"
                })

    def on_subscribe(self, client, userdata, mid, granted_qos, properties=None):
        """Callback for when a SUBSCRIBE response is received from the broker"""
        self.logger("Subscription confirmed")

    def on_disconnect(self, client, userdata, rc):
        self.logger("Disconnected from MQTT Broker", "WARNING")

    def start(self):
        self.running = True
        self.thread = threading.Thread(target=self._run)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join()
        self.client.disconnect()

    def _run(self):
        try:
            # 修改為 HiveMQ Cloud 的地址
            self.client.connect("81a9af6ef0a24070877e2fdb6ce5adb9.s1.eu.hivemq.cloud", 8883, 60)
            self.client.loop_start()
            while self.running:
                time.sleep(0.1)
        except Exception as e:
            self.logger(f"Connection error: {str(e)}", "ERROR")
        finally:
            self.client.loop_stop()
