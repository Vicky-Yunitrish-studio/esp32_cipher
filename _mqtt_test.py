import paho.mqtt.client as mqtt
from datetime import datetime
import tkinter as tk
from tkinter import ttk, scrolledtext
from threading import Thread, Lock
import queue
import traceback
import time
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.dates as mdates
from collections import deque
from Crypto.Cipher import AES
from Crypto.Hash import SHA256
import binascii
import ssl
import certifi

# Update MQTT Settings
MQTT_BROKER = "81a9af6ef0a24070877e2fdb6ce5adb9.s1.eu.hivemq.cloud"
MQTT_PORT = 8883
GROUP_NAME = "group1"
DEVICE_MACS = ["34:85:6D:B6:DF:94"]
# Add TLS settings
MQTT_USERNAME = "esp32-0002"
MQTT_PASSWORD = "Esp320002"

class Decryption:
    def __init__(self, group_name, device_mac):
        self.key_size = 32
        self.nonce_size = 16
        self.constant = "esp32_duel_cipher"
        self.group_name = group_name
        self.device_mac = device_mac
        self.key = self._derive_key()
        self.nonce = self._generate_nonce()
    
    def _derive_key(self):
        # 使用與 ESP32 相同的密鑰導出方法
        initial_key = self.constant.encode().ljust(32, b'\0')
        hash_obj = SHA256.new()
        hash_obj.update(initial_key)
        return hash_obj.digest()

    def _generate_nonce(self):
        # 使用相同的 nonce 生成方法
        seed = (self.group_name + self.device_mac).encode()
        hash_obj = SHA256.new()
        hash_obj.update(seed)
        return hash_obj.digest()[:16]

    def decrypt(self, encrypted_hex):
        try:
            # 將十六進制字串轉換為位元組
            encrypted_data = binascii.unhexlify(encrypted_hex)
            
            if len(encrypted_data) % 16 != 0:
                print("Invalid data length")
                return None

            # 創建 AES 物件
            cipher = AES.new(self.key, AES.MODE_ECB)
            
            # 解密
            decrypted_data = cipher.decrypt(encrypted_data)
            
            # 移除 PKCS7 填充
            padding_len = decrypted_data[-1]
            if padding_len > 16:
                print("Invalid padding")
                return None
                
            unpadded_data = decrypted_data[:-padding_len]
            
            # 轉換為字串
            return unpadded_data.decode('utf-8')
            
        except Exception as e:
            print(f"Decryption error: {e}")
            return None

class MQTTSubscriber:
    def __init__(self):
        # Update MQTT client to use latest API version
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        
        # Add TLS configuration
        self.client.tls_set(
            ca_certs=certifi.where(),
            tls_version=ssl.PROTOCOL_TLSv1_2
        )  # Enable TLS
        self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)  # Set credentials
        
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_subscribe = self.on_subscribe
        self.client.on_disconnect = self.on_disconnect
        self.message_queue = queue.Queue()
        self.data_lock = Lock()
        self.device_data = {}
        self.data_history = {}
        self.history_length = 50
        self.decryptors = {}  # Remove predefined decryptors
        print(f"Monitoring topics: duel_cipher32/{GROUP_NAME}/#")

    def get_decryptor(self, mac):
        # Create decryptor on demand based on MAC from topic
        if mac not in self.decryptors:
            self.decryptors[mac] = Decryption(GROUP_NAME, mac)
        return self.decryptors[mac]

    def on_connect(self, client, userdata, flags, reason_code, properties=None):
        # Convert ReasonCode to integer for compatibility
        rc = reason_code.value if hasattr(reason_code, 'value') else reason_code
        print(f"\nMQTT Connection status: {rc}")
        connection_status = {
            0: "Connected",
            1: "Incorrect protocol version",
            2: "Invalid client ID",
            3: "Server unavailable",
            4: "Bad username or password",
            5: "Not authorized"
        }
        status = connection_status.get(rc, f"Unknown error {rc}")
        self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                              "CONNECTION", status))
        
        if rc == 0:
            topic = f"duel_cipher32/{GROUP_NAME}/#"
            self.client.subscribe(topic)
            print(f"Subscribed to: {topic}")
            self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                                  "SUBSCRIBE", f"Subscribed to {topic}"))

    def on_subscribe(self, client, userdata, mid, granted_qos, properties=None):
        # Updated callback signature for MQTT V2
        self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                              "SUBSCRIBE", f"Subscription confirmed (QoS: {granted_qos})"))

    def on_disconnect(self, client, userdata, rc, properties=None, reason_code=None):
        # Updated callback signature for MQTT V2
        disconnect_reason = reason_code.value if hasattr(reason_code, 'value') else rc
        self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                              "DISCONNECT", "Disconnected from broker"))
        if disconnect_reason != 0:
            self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                                  "ERROR", f"Unexpected disconnection (code: {disconnect_reason})"))

    def on_message(self, client, userdata, msg):
        try:
            now = datetime.now()
            topic = msg.topic
            encrypted_payload = msg.payload.decode()
            
            # 修改主題解析，去除前綴
            parts = topic.split('/')
            if len(parts) != 4:  # 新格式: duel_cipher32/group1/MAC/servicename
                print(f"Invalid topic format: {topic}")
                return
                
            _, group, mac, service = parts  # 加入前綴後要解析4個部分
            
            # 將service名稱映射到內部使用的measurement名稱
            measurement_map = {
                'temperature': 'temp',
                'humidity': 'hum'
            }
            
            measurement = measurement_map.get(service)
            if not measurement:
                print(f"Unsupported service: {service}")
                return
            
            # Get or create decryptor for this MAC
            decryptor = self.get_decryptor(mac)
            decrypted_payload = decryptor.decrypt(encrypted_payload)
            
            if decrypted_payload is None:
                print(f"Decryption failed for message from {mac}")
                return
                
            payload = decrypted_payload
            
            print(f"\nReceived message:")
            print(f"Topic: {topic}")
            print(f"Payload: {payload}")
            
            # Update device data
            with self.data_lock:
                if mac not in self.device_data:
                    self.device_data[mac] = {
                        'temp': 'N/A',
                        'hum': 'N/A',
                        'last_update': 'Never'
                    }
                
                # 解析主題
                parts = topic.split('/')
                if len(parts) == 4:  # 格式: esp32/group1/MAC/type
                    _, group, mac, measurement = parts
                    
                    with self.data_lock:
                        if mac not in self.device_data:
                            self.device_data[mac] = {
                                'temp': 'N/A',
                                'hum': 'N/A',
                                'last_update': 'Never'
                            }
                        
                        # 更新數據
                        self.device_data[mac][measurement] = payload
                        self.device_data[mac]['last_update'] = now
                    
                        # 更新歷史數據
                        try:
                            value = float(payload)
                            if mac not in self.data_history:
                                self.data_history[mac] = {
                                    'temp': {'times': deque(maxlen=self.history_length),
                                            'values': deque(maxlen=self.history_length)},
                                    'hum': {'times': deque(maxlen=self.history_length),
                                        'values': deque(maxlen=self.history_length)}
                                }
                            
                            self.data_history[mac][measurement]['times'].append(now)
                            self.data_history[mac][measurement]['values'].append(value)
                            
                            # 發送到消息隊列以更新 UI
                            device_name = DEVICE_NAMES.get(mac, mac)
                            self.message_queue.put((now.strftime("%Y-%m-%d %H:%M:%S"), 
                                "DATA", f"[{device_name}] {measurement}: {payload}"))
                            
                        except ValueError as e:
                            print(f"Value conversion error: {e}")
                            
                    print(f"Updated device data: {self.device_data[mac]}")
            
        except Exception as e:
            print(f"Message processing error: {e}")
            traceback.print_exc()
            self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"), 
                                "ERROR", str(e)))

    def start(self):
        # MQTT連接在單獨的線程中運行
        Thread(target=self._mqtt_loop, daemon=True).start()

    def _mqtt_loop(self):
        try:
            print(f"Connecting to {MQTT_BROKER}...")
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_forever()
        except Exception as e:
            error_msg = f"MQTT Error: {str(e)}"
            print(error_msg)  # Console debug
            self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                                  "ERROR", error_msg))

class TestMessagePublisher:
    def __init__(self, parent_ui):
        self.parent_ui = parent_ui
        self.client = None
        self.is_connected = False
        self.message_sent = False
        self.sending = False  # 添加發送狀態標誌
        
    def cleanup(self):
        if self.client:
            try:
                self.client.disconnect()
            except:
                pass
            self.client = None
        if hasattr(self, 'pending_message'):
            delattr(self, 'pending_message')
        self.is_connected = False
        self.message_sent = False
        self.sending = False  # 重置發送狀態
        
    def send(self, topic, payload):
        if self.sending:  # 如果正在發送，直接返回
            print("Already sending a message, please wait...")
            return
            
        self.sending = True  # 設置發送狀態
        try:
            self.cleanup()  # 清理之前的客戶端
            
            self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
            self.client.on_connect = self.on_connect
            self.client.on_publish = self.on_publish
            self.client.on_disconnect = self.on_disconnect
            
            self.client.tls_set(
                ca_certs=certifi.where(),
                tls_version=ssl.PROTOCOL_TLSv1_2
            )
            self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
            
            self.pending_message = (topic, payload)
            self.client.connect_async(MQTT_BROKER, MQTT_PORT, 60)
            self.client.loop_start()
            
            # 設定超時清理
            self.parent_ui.root.after(5000, self.timeout_cleanup)
            
        except Exception as e:
            print(f"Setup error: {e}")
            self.cleanup()
            
    def timeout_cleanup(self):
        """超時清理"""
        if self.sending:
            print("Message send timeout, cleaning up...")
            self.cleanup()

    def on_publish(self, client, userdata, mid, reason_codes=None, properties=None):
        print("Message published successfully")
        self.message_sent = True
        self.parent_ui.root.after(100, self.cleanup)  # 延遲清理

    def on_connect(self, client, userdata, flags, reason_code, properties=None):
        self.is_connected = True
        print("Test client connected")
        if hasattr(self, 'pending_message'):
            self.publish_message(*self.pending_message)
            
    def on_publish(self, client, userdata, mid, reason_codes=None, properties=None):
        # Updated callback signature for MQTT V2
        self.message_sent = True
        self.client.disconnect()
        
    def on_disconnect(self, client, userdata, rc, properties=None, reason_code=None):
        self.client = None
        self.parent_ui.root.after(100, self.cleanup)
        
    def cleanup(self):
        if hasattr(self, 'pending_message'):
            delattr(self, 'pending_message')
        self.is_connected = False
        self.message_sent = False
        
    def publish_message(self, topic, payload):
        print(f"Publishing to topic: {topic}")
        # 將 QoS 從 1 改為 0
        self.client.publish(topic, payload, qos=0)
        
    def send(self, topic, payload):
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_connect = self.on_connect
        self.client.on_publish = self.on_publish
        self.client.on_disconnect = self.on_disconnect
        
        self.client.tls_set(
            ca_certs=certifi.where(),
            tls_version=ssl.PROTOCOL_TLSv1_2
        )
        self.client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
        
        self.pending_message = (topic, payload)
        self.client.connect_async(MQTT_BROKER, MQTT_PORT, 60)
        self.client.loop_start()

class MQTTUI:
    def __init__(self, subscriber):
        self.subscriber = subscriber
        self.root = tk.Tk()
        self.root.title("ESP32 MQTT Monitor")
        
        # 初始化變量
        self.plot_update_interval = 2000  # 2秒
        self.last_plot_update = time.time() * 1000
        
        # 設置最小視窗大小
        self.root.minsize(600, 400)
        
        # 先創建UI元件
        self.setup_ui()
        
        # 再綁定事件
        self.root.bind('<Configure>', self.on_window_resize)
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        # 啟動更新
        self.update_ui()
        self.setup_test_panel()  # 添加這一行到__init__方法中
        self.test_publisher = TestMessagePublisher(self)

    def setup_ui(self):
        # 主框架設置
        self.main_frame = ttk.Frame(self.root, padding="10")
        self.main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 設置主框架的權重
        self.root.grid_rowconfigure(0, weight=1)
        self.root.grid_columnconfigure(0, weight=1)

        # 狀態面板
        status_frame = ttk.LabelFrame(self.main_frame, text="Encryption Status", padding="5")
        status_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 確保狀態文本框正確創建
        self.status_text = scrolledtext.ScrolledText(status_frame, height=5)
        self.status_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 設備數據顯示
        devices_frame = ttk.LabelFrame(self.main_frame, text="Devices", padding="5")
        devices_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 使用自定義樣式
        style = ttk.Style()
        style.configure("Treeview", font=('TkDefaultFont', 10))
        style.configure("Treeview.Heading", font=('TkDefaultFont', 10, 'bold'))
        
        self.devices_tree = ttk.Treeview(devices_frame, 
                                       columns=('Name', 'Temperature', 'Humidity', 'Last Update'),
                                       show='headings',
                                       style="Treeview")
        
        # 設置列標題
        for col in ('Name', 'Temperature', 'Humidity', 'Last Update'):
            self.devices_tree.heading(col, text=col)
            self.devices_tree.column(col, width=1)  # 讓列寬自動調整
        
        self.devices_tree.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 添加滾動條
        scrollbar = ttk.Scrollbar(devices_frame, orient="vertical", command=self.devices_tree.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.devices_tree.configure(yscrollcommand=scrollbar.set)

        # 日誌區域
        log_frame = ttk.LabelFrame(self.main_frame, text="Message Log", padding="5")
        log_frame.grid(row=2, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 確保日誌文本框正確創建
        self.log_text = scrolledtext.ScrolledText(log_frame, height=10)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 修改圖表區域
        charts_frame = ttk.LabelFrame(self.main_frame, text="Sensor Data Charts", padding="5")
        charts_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 創建左右分隔的框架
        left_chart_frame = ttk.Frame(charts_frame)
        left_chart_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5)
        
        right_chart_frame = ttk.Frame(charts_frame)
        right_chart_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5)
        
        # 設置列/行權重
        charts_frame.grid_columnconfigure(0, weight=1)
        charts_frame.grid_columnconfigure(1, weight=1)
        
        # 創建溫度圖表
        self.temp_fig = Figure(figsize=(4, 3), dpi=100)
        self.temp_ax = self.temp_fig.add_subplot(111)
        self.temp_canvas = FigureCanvasTkAgg(self.temp_fig, master=left_chart_frame)
        self.temp_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # 創建濕度圖表
        self.hum_fig = Figure(figsize=(4, 3), dpi=100)
        self.hum_ax = self.hum_fig.add_subplot(111)
        self.hum_canvas = FigureCanvasTkAgg(self.hum_fig, master=right_chart_frame)
        self.hum_canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # 設置圖表的初始格式
        self.temp_ax.set_title('Temperature History')
        self.temp_ax.set_xlabel('Time')
        self.temp_ax.set_ylabel('Temperature (°C)')
        self.temp_ax.grid(True)
        
        self.hum_ax.set_title('Humidity History')
        self.hum_ax.set_xlabel('Time')
        self.hum_ax.set_ylabel('Humidity (%)')
        self.hum_ax.grid(True)

        # 設置框架的權重
        self.main_frame.grid_rowconfigure(1, weight=3)  # 設備列表佔較大空間
        self.main_frame.grid_rowconfigure(2, weight=2)  # 日誌區域次之
        self.main_frame.grid_rowconfigure(3, weight=3)  # 給圖表區域分配空間
        self.main_frame.grid_columnconfigure(0, weight=1)
        
        devices_frame.grid_columnconfigure(0, weight=1)
        log_frame.grid_columnconfigure(0, weight=1)

    def setup_test_panel(self):
        # 創建測試面板
        test_frame = ttk.LabelFrame(self.main_frame, text="Test Message Panel", padding="5")
        test_frame.grid(row=4, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Group ID 輸入
        ttk.Label(test_frame, text="Group ID:").grid(row=0, column=0, padx=5, pady=2)
        self.group_entry = ttk.Entry(test_frame)
        self.group_entry.insert(0, GROUP_NAME)
        self.group_entry.grid(row=0, column=1, padx=5, pady=2)

        # MAC 輸入
        ttk.Label(test_frame, text="MAC:").grid(row=0, column=2, padx=5, pady=2)
        self.mac_entry = ttk.Entry(test_frame)
        self.mac_entry.insert(0, "34856DB6DF94")  # 預設值
        self.mac_entry.grid(row=0, column=3, padx=5, pady=2)

        # 數值輸入
        ttk.Label(test_frame, text="Value:").grid(row=0, column=4, padx=5, pady=2)
        self.value_entry = ttk.Entry(test_frame)
        self.value_entry.insert(0, "25.5")  # 預設值
        self.value_entry.grid(row=0, column=5, padx=5, pady=2)

        # 主題選擇
        self.topic_var = tk.StringVar(value="temperature")
        ttk.Radiobutton(test_frame, text="Temperature", 
                       variable=self.topic_var, 
                       value="temperature").grid(row=1, column=1, padx=5, pady=2)
        ttk.Radiobutton(test_frame, text="Humidity", 
                       variable=self.topic_var, 
                       value="humidity").grid(row=1, column=2, padx=5, pady=2)

        # 發送按鈕
        send_button = ttk.Button(test_frame, text="Send Test Message", 
                               command=self.send_test_message)
        send_button.grid(row=1, column=3, columnspan=2, pady=5)

        # 調整框架權重
        self.main_frame.grid_rowconfigure(4, weight=1)

    def send_test_message(self):
        if self.test_publisher.sending:  # 檢查是否正在發送
            print("Please wait for the previous message to complete")
            return
            
        try:
            # 獲取輸入值
            group_id = self.group_entry.get()
            mac = self.mac_entry.get().upper()
            value = self.value_entry.get()
            topic_type = self.topic_var.get()
            
            # 創建加密器和加密數據
            encryptor = Decryption(group_id, mac)
            topic = f"duel_cipher32/{group_id}/{mac}/{topic_type}"
            
            try:
                # 使用PKCS7填充
                data = value.encode('utf-8')
                padding_length = 16 - (len(data) % 16)
                padded_data = data + bytes([padding_length] * padding_length)
                cipher = AES.new(encryptor.key, AES.MODE_ECB)
                encrypted = cipher.encrypt(padded_data)
                encrypted_data = binascii.hexlify(encrypted).decode()
            except Exception as e:
                self._log_error(f"Encryption error: {e}")
                return

            # 訂閱新主題（如果需要）
            if group_id not in GROUP_NAME:
                self.subscriber.client.subscribe(f"duel_cipher32/{group_id}/#")
                print(f"Subscribed to additional group: duel_cipher32/{group_id}/#")

            # 使用非阻塞方式發送消息
            self.test_publisher.send(topic, encrypted_data)
            
            # 記錄發送嘗試
            self._log_success(value, topic, encrypted_data)

        except Exception as e:
            self._log_error(str(e))

    def _log_success(self, value, topic, encrypted_data):
        """安全地在主線程中更新UI"""
        self.log_text.insert(tk.END, 
            f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] TEST: "
            f"Sent {value} to {topic} (encrypted: {encrypted_data})\n")
        self.log_text.see(tk.END)

    def _log_error(self, error_message):
        """安全地在主線程中顯示錯誤"""
        self.log_text.insert(tk.END, 
            f"[{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] "
            f"TEST ERROR: {error_message}\n")
        self.log_text.see(tk.END)
        print(f"Test message error: {error_message}")

    def on_window_resize(self, event):
        # 根據視窗大小調整字體大小
        width = self.root.winfo_width()
        height = self.root.winfo_height()
        
        # 計算基礎字體大小（根據視窗大小調整）
        base_size = min(max(int(width/80), 8), 16)
        
        # 更新各個元件的字體
        style = ttk.Style()
        style.configure("Treeview", font=('TkDefaultFont', base_size))
        style.configure("Treeview.Heading", font=('TkDefaultFont', base_size, 'bold'))
        
        self.status_text.configure(font=('TkDefaultFont', base_size))
        self.log_text.configure(font=('TkDefaultFont', base_size))
        
        # 根據視窗大小調整列寬
        tree_width = self.devices_tree.winfo_width()
        self.devices_tree.column('Name', width=int(tree_width * 0.3))
        self.devices_tree.column('Temperature', width=int(tree_width * 0.2))
        self.devices_tree.column('Humidity', width=int(tree_width * 0.2))
        self.devices_tree.column('Last Update', width=int(tree_width * 0.3))

    def update_ui(self):
        try:
            # 更新設備數據顯示
            with self.subscriber.data_lock:
                # 清除現有項目
                for item in self.devices_tree.get_children():
                    self.devices_tree.delete(item)
                
                # 添加/更新設備數據
                for mac, data in self.subscriber.device_data.items():
                    device_name = DEVICE_NAMES.get(mac, mac)
                    temp = f"{data['temp']}°C" if data['temp'] != 'N/A' else 'N/A'
                    hum = f"{data['hum']}%" if data['hum'] != 'N/A' else 'N/A'
                    last_update = data['last_update']
                    if isinstance(last_update, datetime):
                        last_update = last_update.strftime("%Y-%m-%d %H:%M:%S")
                    
                    self.devices_tree.insert('', 'end', values=(
                        device_name,
                        temp,
                        hum,
                        last_update
                    ))

            # 處理消息隊列
            while not self.subscriber.message_queue.empty():
                timestamp, msg_type, message = self.subscriber.message_queue.get_nowait()
                
                # 添加到日誌
                self.log_text.insert(tk.END, f"[{timestamp}] {msg_type}: {message}\n")
                self.log_text.see(tk.END)

        except Exception as e:
            print(f"UI Update Error: {e}")
            traceback.print_exc()

        # 定期更新
        self.root.after(100, self.update_ui)

    def update_chart(self):
        try:
            current_time = time.time() * 1000
            
            if current_time - self.last_plot_update < self.plot_update_interval:
                self.root.after(100, self.update_chart)
                return
                
            self.last_plot_update = current_time
            
            # 清除圖表
            self.temp_ax.clear()
            self.hum_ax.clear()
            
            has_data = False  # 添加標誌來追踪是否有數據
            
            with self.subscriber.data_lock:
                for mac, data in self.subscriber.data_history.items():
                    device_name = DEVICE_NAMES.get(mac, mac)
                    
                    # 檢查並繪製溫度數據
                    temp_times = list(data['temp']['times'])
                    temp_values = list(data['temp']['values'])
                    if temp_times and temp_values:
                        self.temp_ax.plot(temp_times, temp_values, '-o', 
                                        label=device_name,
                                        markersize=4,
                                        linewidth=1)
                        has_data = True
                    
                    # 檢查並繪製濕度數據
                    hum_times = list(data['hum']['times'])
                    hum_values = list(data['hum']['values'])
                    if hum_times and hum_values:
                        self.hum_ax.plot(hum_times, hum_values, '-o', 
                                       label=device_name,
                                       markersize=4,
                                       linewidth=1)
                        has_data = True
            
            # 只有在有數據時才設置圖例
            if has_data:
                self.temp_ax.set_title('Temperature History')
                self.temp_ax.set_xlabel('Time')
                self.temp_ax.set_ylabel('Temperature (°C)')
                self.temp_ax.grid(True)
                if len(self.temp_ax.lines) > 0:  # 確認有線條才添加圖例
                    self.temp_ax.legend(loc='upper right')
                
                self.hum_ax.set_title('Humidity History')
                self.hum_ax.set_xlabel('Time')
                self.hum_ax.set_ylabel('Humidity (%)')
                self.hum_ax.grid(True)
                if len(self.hum_ax.lines) > 0:  # 確認有線條才添加圖例
                    self.hum_ax.legend(loc='upper right')
                
                # 格式化時間軸
                for ax in [self.temp_ax, self.hum_ax]:
                    ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
                    # 設置合適的時間範圍
                    if temp_times:  # 如果有數據
                        ax.set_xlim(min(temp_times), max(temp_times))
                
                # 自動調整日期標籤角度
                self.temp_fig.autofmt_xdate()
                self.hum_fig.autofmt_xdate()
            
            # 更新畫布
            self.temp_canvas.draw()
            self.hum_canvas.draw()
            
        except Exception as e:
            print(f"Chart update error: {e}")
            traceback.print_exc()
        
        # 安排下一次更新
        self.root.after(self.plot_update_interval, self.update_chart)

    def on_closing(self):
        print("Closing application...")
        try:
            self.subscriber.client.disconnect()
        finally:
            self.root.destroy()

    def run(self):
        self.subscriber.start()
        # 開始定期更新圖表
        self.update_chart()
        self.root.mainloop()

if __name__ == "__main__":
    print(f"Starting MQTT subscriber for group: {GROUP_NAME}")
    print(f"Monitoring devices: {DEVICE_MACS}")
    subscriber = MQTTSubscriber()
    ui = MQTTUI(subscriber)
    ui.run()
