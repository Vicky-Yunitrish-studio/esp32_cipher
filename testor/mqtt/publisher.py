import paho.mqtt.client as mqtt
import ssl
import certifi
import threading
import time
from testor.utils.config import *
from testor.utils.encryption import Encryption

class TestMessagePublisher:
    def __init__(self, parent_ui):
        self.parent_ui = parent_ui
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.tls_set(
            ca_certs=certifi.where(),
            tls_version=ssl.PROTOCOL_TLSv1_2
        )
        self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        
        self.is_connected = False
        self.message_sent = False
        self.sending = False
        self.auto_test_running = False
        self.test_thread = None
        self.encryption = Encryption()

    def publish_test_message(self, message, group=None, device=None, topic_type=None):
        try:
            if not self.is_connected:
                self.connect()
            
            # Use provided values or defaults
            group = group or GROUP_NAME
            device = device or DEVICE_MACS[0]
            topic_type = topic_type or "temperature"
            
            # Encrypt message before publishing
            encrypted_message = self.encryption.encrypt(message)
            
            # Build topic string
            topic = f"duel_cipher32/{group}/{device}/{topic_type}"
            
            self.parent_ui.log_message(f"Publishing to topic: {topic}")
            self.parent_ui.log_message(f"Group: {group}")
            self.parent_ui.log_message(f"Device: {device}")
            self.parent_ui.log_message(f"Topic Type: {topic_type}")
            self.parent_ui.log_message(f"Original message: {message}")
            self.parent_ui.log_message(f"Encrypted message: {encrypted_message}")
            
            self.client.publish(topic, encrypted_message)
            
        except Exception as e:
            self.parent_ui.log_message(f"Error publishing message: {str(e)}", "ERROR")

    def connect(self):
        try:
            # 修改為 HiveMQ Cloud 的地址
            self.client.connect("81a9af6ef0a24070877e2fdb6ce5adb9.s1.eu.hivemq.cloud", 8883, 60)
            self.client.loop_start()
            self.is_connected = True
            self.parent_ui.log_message("Test client connected")
        except Exception as e:
            self.parent_ui.log_message(f"Connection error: {str(e)}", "ERROR")
            self.is_connected = False

    def start_auto_test(self):
        if not self.auto_test_running:
            self.auto_test_running = True
            self.test_thread = threading.Thread(target=self._auto_test_loop)
            self.test_thread.daemon = True
            self.test_thread.start()

    def stop_auto_test(self):
        self.auto_test_running = False
        if self.test_thread:
            self.test_thread.join()

    def _auto_test_loop(self):
        while self.auto_test_running:
            try:
                message = "25.5"
                self.publish_test_message(
                    message=message,
                    group=self.parent_ui.test_panel.group_entry.get(),
                    device=self.parent_ui.test_panel.device_entry.get(),
                    topic_type=self.parent_ui.test_panel.topic_var.get()
                )
                time.sleep(5)
            except Exception as e:
                self.parent_ui.log_message(f"Auto test error: {str(e)}", "ERROR")
                time.sleep(1)
