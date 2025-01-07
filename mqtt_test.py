import paho.mqtt.client as mqtt
from datetime import datetime
import struct
import binascii
import tkinter as tk
from tkinter import ttk, scrolledtext
from threading import Thread, Lock
import queue
import traceback  # 添加 traceback 導入
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.dates as mdates
from collections import deque
from datetime import datetime, timedelta

# MQTT Settings
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883

# 配置設定
GROUP_NAME = "group1"  # 確保與 ESP32 配置中的組名完全一致
DEVICE_MACS = [
    "34:85:6D:B6:DF:94",  # 請確保這是你的 ESP32 的 MAC 地址
]

# 添加實際使用的金鑰
ENCRYPTION_KEY = bytes([
    0xDC, 0x39, 0x35, 0xB9, 0x4F, 0x70, 0x69, 0x97,
    0x3B, 0x0A, 0xF6, 0x83, 0xF2, 0xF2, 0x19, 0xA1,
    0xEF, 0x24, 0x08, 0x39, 0x12, 0x54, 0xE6, 0x32,
    0x36, 0x79, 0x44, 0xE7, 0xBD, 0xAA, 0x7B, 0x0F
])

class ChaCha20Decryptor:
    def __init__(self):
        self.state = [0] * 16
        self.constants = [0] * 4
        self.key = ENCRYPTION_KEY
        self.nonce = bytearray(12)
        self.counter = 1

    def set_device_mac(self, mac_str):
        # 清除 nonce
        self.nonce = bytearray(12)
        
        # 直接使用完整的 MAC 地址作為 nonce
        clean_mac = mac_str.replace(':', '')
        mac_bytes = bytes.fromhex(clean_mac)
        # 複製所有可用的字節
        for i in range(min(12, len(mac_bytes))):
            self.nonce[i] = mac_bytes[i]
            
        print(f"Nonce set: {self.nonce.hex()}")

    def rotl32(self, x, n):
        return ((x << n) | (x >> (32 - 32))) & 0xffffffff

    def quarter_round(self, a, b, c, d):
        self.state[a] = (self.state[a] + self.state[b]) & 0xffffffff
        self.state[d] ^= self.state[a]
        self.state[d] = self.rotl32(self.state[d], 16)

        self.state[c] = (self.state[c] + self.state[d]) & 0xffffffff
        self.state[b] ^= self.state[c]
        self.state[b] = self.rotl32(self.state[b], 12)

        self.state[a] = (self.state[a] + self.state[b]) & 0xffffffff
        self.state[d] ^= self.state[a]
        self.state[d] = self.rotl32(self.state[d], 8)

        self.state[c] = (self.state[c] + self.state[d]) & 0xffffffff
        self.state[b] ^= self.state[c]
        self.state[b] = self.rotl32(self.state[b], 7)

    def chacha20_block(self):
        """生成 ChaCha20 加密塊"""
        # 初始化狀態
        self.state = list(self.constants)  # 前 4 個字為常數
        
        # 複製金鑰 (8個32位字)
        key_words = struct.unpack('<8I', self.key)
        self.state.extend(key_words)
        
        # 設置 counter 和 nonce
        self.state.append(self.counter)
        nonce_words = struct.unpack('<3I', self.nonce)
        self.state.extend(nonce_words)
        
        # 保存初始狀態
        initial = self.state.copy()
        
        # 執行 20 輪轉換
        for _ in range(10):
            # 列轉換
            self.quarter_round(0, 4, 8, 12)
            self.quarter_round(1, 5, 9, 13)
            self.quarter_round(2, 6, 10, 14)
            self.quarter_round(3, 7, 11, 15)
            
            # 對角線轉換
            self.quarter_round(0, 5, 10, 15)
            self.quarter_round(1, 6, 11, 12)
            self.quarter_round(2, 7, 8, 13)
            self.quarter_round(3, 4, 9, 14)
        
        # 將工作狀態加回初始狀態
        for i in range(16):
            self.state[i] = (self.state[i] + initial[i]) & 0xffffffff

    def decrypt_block(self, data):
        # 不需要重置 nonce 的前 6 個字節，因為本來就是 0
        self.counter = 1
        # 生成密鑰流
        self.chacha20_block()
        keystream = b''.join(struct.pack('<I', x) for x in self.state)
        
        # 解密數據
        result = bytearray()
        for i in range(len(data)):
            result.append(data[i] ^ keystream[i])
        
        return bytes(result)

    def decrypt_hex(self, hex_str):
        try:
            print("\nDecryption attempt:")
            print(f"Input hex: {hex_str}")
            print(f"Using constants: {[hex(x) for x in self.constants]}")
            print(f"Using nonce: {self.nonce.hex()}")
            
            data = bytes.fromhex(hex_str)
            self.counter = 1
            
            self.chacha20_block()
            keystream = b''.join(struct.pack('<I', x) for x in self.state)
            
            result = bytearray()
            for i, byte in enumerate(data):
                result.append(byte ^ keystream[i])
            
            # 改進浮點數解析
            try:
                decoded = result.decode()
                print(f"Decoded string: {decoded}")
                
                # 嘗試直接解析浮點數
                value = float(decoded)
                if self._is_valid_value(value):
                    # 保持2位小數的格式
                    return f"{value:.2f}"
                else:
                    print(f"Invalid value: {value}")
                    return "Error: Invalid value"
                    
            except Exception as e:
                print(f"Decoding/conversion error: {str(e)}")
                print(f"Raw bytes: {result.hex()}")
                return "Error: Conversion failed"
                
        except Exception as e:
            print(f"Decryption error: {str(e)}")
            traceback.print_exc()
            return "Error"

    def _is_valid_value(self, value):
        """改進值的有效性檢查"""
        if '/temp' in self.last_topic:
            return -40.0 <= value <= 85.0  # DHT11 溫度範圍
        elif '/hum' in self.last_topic:
            return 0.0 <= value <= 100.0   # DHT11 濕度範圍
        return False

    def update_constants(self, topic):
        """更新加密常數"""
        self.last_topic = topic
        # 使用與 ESP32 相同的標準常數
        self.constants = [
            0x61707865,  # "expa"
            0x3320646e,  # "nd 3"
            0x79622d32,  # "2-by"
            0x6b206574   # "te k"
        ]
        
        # 獲取最後一段 (temp 或 hum)
        last_part = topic.split('/')[-1]
        
        # 將字符串轉換為整數，使用與 ESP32 相同的方式
        topic_value = 0
        for i, c in enumerate(last_part[:4]):
            topic_value |= (ord(c) << (i * 8))
        
        # 修改第一個常數
        self.constants[0] ^= topic_value
        
        print(f"Topic: {topic}")
        print(f"Last part: {last_part}")
        print(f"Topic value: 0x{topic_value:08x}")
        print(f"Constants: {[hex(x) for x in self.constants]}")

class MQTTSubscriber:
    def __init__(self):
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_subscribe = self.on_subscribe
        self.client.on_disconnect = self.on_disconnect
        self.message_queue = queue.Queue()
        self.data_lock = Lock()
        self.device_data = {}  # 存儲設備數據
        self.data_history = {}  # 存儲歷史數據
        self.history_length = 50  # 保存最近50個數據點
        print(f"Monitoring topics: esp32/{GROUP_NAME}/#")

    def on_connect(self, client, userdata, flags, rc):
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
            # 訂閱整個主題樹
            topic = f"esp32/{GROUP_NAME}/#"
            self.client.subscribe(topic)
            print(f"Subscribed to: {topic}")
            self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                                  "SUBSCRIBE", f"Subscribed to {topic}"))

    def on_subscribe(self, client, userdata, mid, granted_qos):
        self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                              "SUBSCRIBE", f"Subscription confirmed (QoS: {granted_qos})"))

    def on_disconnect(self, client, userdata, rc):
        self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                              "DISCONNECT", "Disconnected from broker"))
        if rc != 0:
            self.message_queue.put((datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                                  "ERROR", f"Unexpected disconnection (code: {rc})"))

    def on_message(self, client, userdata, msg):
        try:
            now = datetime.now()
            topic = msg.topic
            payload = msg.payload.decode()
            
            # 調試輸出
            print(f"\nReceived message:")
            print(f"Topic: {topic}")
            print(f"Payload: {payload}")
            
            # 解析主題
            parts = topic.split('/')
            if len(parts) == 4:  # 格式: esp32/group1/MAC/type
                _, group, mac, measurement = parts
                
                # 更新設備數據
                with self.data_lock:
                    if mac not in self.device_data:
                        self.device_data[mac] = {
                            'temp': 'N/A',
                            'hum': 'N/A',
                            'last_update': 'Never'
                        }
                    
                    # 更新對應的數值
                    self.device_data[mac][measurement] = payload
                    self.device_data[mac]['last_update'] = now
                
                # 初始化歷史數據結構
                if mac not in self.data_history:
                    self.data_history[mac] = {
                        'temp': {'times': deque(maxlen=self.history_length),
                                'values': deque(maxlen=self.history_length)},
                        'hum': {'times': deque(maxlen=self.history_length),
                               'values': deque(maxlen=self.history_length)}
                    }
                
                # 更新歷史數據
                try:
                    value = float(payload)
                    self.data_history[mac][measurement]['times'].append(now)
                    self.data_history[mac][measurement]['values'].append(value)
                except ValueError:
                    print(f"Could not convert payload to float: {payload}")

                # 發送到消息隊列以更新 UI
                device_name = DEVICE_NAMES.get(mac, mac)
                self.message_queue.put((now, "DATA", 
                    f"[{device_name}] {measurement}: {payload}"))
                
                print(f"Updated device data: {self.device_data[mac]}")
            
        except Exception as e:
            print(f"Message processing error: {e}")
            traceback.print_exc()
            self.message_queue.put((now, "ERROR", str(e)))

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

# 添加裝置名稱映射
DEVICE_NAMES = {
    "34:85:6D:B6:DF:94": "Living Room Sensor",  # 自定義裝置名稱
    "3485:6D:B6:DF:94": "Living Room Sensor",
    "34856DB6DF94": "Living Room Sensor",  # 添加無分隔符版本
    "34:85:6D:B6:DF:94": "Living Room Sensor",  # 添加原始 MAC 地址
}

class MQTTUI:
    def __init__(self, subscriber):
        self.subscriber = subscriber
        self.root = tk.Tk()
        self.root.title("ESP32 MQTT Monitor")
        
        # 設置最小視窗大小
        self.root.minsize(600, 400)
        
        # 綁定視窗大小改變事件
        self.root.bind('<Configure>', self.on_window_resize)
        
        self.plot_update_interval = 1000  # 圖表更新間隔（毫秒）
        
        self.setup_ui()
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.update_ui()

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
        
        self.status_text = scrolledtext.ScrolledText(status_frame, height=5, font=('TkDefaultFont', 10))
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
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=10, font=('TkDefaultFont', 10))
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
                    temp = data['temp'] + "°C" if data['temp'] != 'N/A' else 'N/A'
                    hum = data['hum'] + "%" if data['hum'] != 'N/A' else 'N/A'
                    
                    self.devices_tree.insert('', 'end', values=(
                        device_name,
                        temp,
                        hum,
                        data['last_update']
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
            # 清除圖表
            self.temp_ax.clear()
            self.hum_ax.clear()
            
            # 為每個設備繪製溫度曲線
            for mac, data in self.subscriber.data_history.items():
                device_name = DEVICE_NAMES.get(mac, mac)
                
                # 繪製溫度數據
                temp_times = data['temp']['times']
                temp_values = data['temp']['values']
                if temp_times and temp_values:
                    self.temp_ax.plot(temp_times, temp_values, '-o', 
                                    label=device_name, markersize=4)
                
                # 繪製濕度數據
                hum_times = data['hum']['times']
                hum_values = data['hum']['values']
                if hum_times and hum_values:
                    self.hum_ax.plot(hum_times, hum_values, '-o', 
                                   label=device_name, markersize=4)
            
            # 設置溫度圖表格式
            self.temp_ax.set_title('Temperature History')
            self.temp_ax.set_xlabel('Time')
            self.temp_ax.set_ylabel('Temperature (°C)')
            self.temp_ax.grid(True)
            self.temp_ax.legend()
            
            # 設置濕度圖表格式
            self.hum_ax.set_title('Humidity History')
            self.hum_ax.set_xlabel('Time')
            self.hum_ax.set_ylabel('Humidity (%)')
            self.hum_ax.grid(True)
            self.hum_ax.legend()
            
            # 格式化時間軸
            for ax in [self.temp_ax, self.hum_ax]:
                ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
                
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
