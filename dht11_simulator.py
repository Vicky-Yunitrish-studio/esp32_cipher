import tkinter as tk
from tkinter import ttk, scrolledtext
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from datetime import datetime, timedelta
import time
from collections import deque
from chacha20 import ChaCha20
import binascii
import random

# 生成更多隨機數據
MOCK_MESSAGES = []

# 生成模擬數據
base_time = datetime.now()
base_temp = 23.0  # 起始溫度
base_hum = 65.0   # 起始濕度

# 模擬緩慢的溫度變化
temp_trend = 0.0  # 溫度變化趨勢
hum_trend = 0.0   # 濕度變化趨勢

# 增加錯誤模擬的變數
ERROR_RATE = 0.1  # 10% 的錯誤率

for i in range(100):
    # 更新變化趨勢（緩慢變化）
    if i % 10 == 0:  # 每10個數據點才改變趨勢
        temp_trend = random.uniform(-0.1, 0.1)
        hum_trend = random.uniform(-0.2, 0.2)
    
    # 生成溫濕度數據，加入微小的隨機波動
    temp = base_temp + (temp_trend * (i/10)) + random.uniform(-0.1, 0.1)
    hum = base_hum + (hum_trend * (i/10)) + random.uniform(-0.2, 0.2)
    
    # 限制溫濕度範圍
    temp = max(min(temp, 28.0), 18.0)  # 溫度限制在18-28度
    hum = max(min(hum, 75.0), 55.0)    # 濕度限制在55-75%
    
    timestamp = (base_time + timedelta(seconds=i*5)).strftime("%H:%M:%S")
    
    # 加入隨機錯誤訊息
    if random.random() < ERROR_RATE:
        # 生成錯誤的加密訊息
        encrypted = ''.join(random.choices('0123456789abcdef', k=96))  # 隨機的 hex 字串
        error_type = random.choice([
            "Invalid encrypted format",
            "Decryption failed: corrupted data",
            "Message authentication failed",
            "Invalid padding"
        ])
    else:
        # 正常的加密訊息
        encrypted = f"b88c46138efd9cc4173ba397c8ac32b97b29e5bed2855cc23422a841ddd1132361deb70904c33b{i:02x}55"
        error_type = None
    
    MOCK_MESSAGES.append({
        "topic": "duel_cipher32/test_group/34865DB6DF94/dht11",
        "encrypted": encrypted,
        "temp": round(temp, 1),
        "hum": round(hum, 1),
        "timestamp": timestamp,
        "error": error_type
    })

class DHT11Simulator:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("DHT11 Sensor Simulator")
        self.paused = False
        
        # 初始化數據儲存
        self.temps = deque(maxlen=20)
        self.hums = deque(maxlen=20)
        self.times = deque(maxlen=20)
        
        # 初始化解密器
        self.cryptor = ChaCha20()
        self.key = binascii.unhexlify("DC3935B94F7069973B0AF683F2F219A1EF2408391254E632367944E7BDAA7B0F")
        
        self.setup_gui()
        self.message_index = 0
        self.update_data()

    def setup_gui(self):
        # 主框架
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # 當前值顯示
        values_frame = ttk.LabelFrame(main_frame, text="Current Values", padding="5")
        values_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E))
        
        self.temp_label = ttk.Label(values_frame, text="Temperature: -- °C")
        self.temp_label.grid(row=0, column=0, padx=10)
        
        self.hum_label = ttk.Label(values_frame, text="Humidity: -- %")
        self.hum_label.grid(row=0, column=1, padx=10)

        # 圖表
        figure = plt.Figure(figsize=(10, 4))
        
        # 溫度圖表
        self.temp_ax = figure.add_subplot(121)
        self.temp_ax.set_title('Temperature History')
        self.temp_ax.set_ylabel('Temperature (°C)')
        
        # 濕度圖表
        self.hum_ax = figure.add_subplot(122)
        self.hum_ax.set_title('Humidity History')
        self.hum_ax.set_ylabel('Humidity (%)')
        
        self.canvas = FigureCanvasTkAgg(figure, master=main_frame)
        self.canvas.get_tk_widget().grid(row=1, column=0, columnspan=2)

        # 日誌視窗
        log_frame = ttk.LabelFrame(main_frame, text="Decryption Log", padding="5")
        log_frame.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E))
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=10)
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E))

        # 添加控制按鈕框架
        control_frame = ttk.Frame(main_frame)
        control_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)
        
        self.continue_btn = ttk.Button(control_frame, text="Continue", command=self.continue_update)
        self.continue_btn.pack(side=tk.RIGHT, padx=5)
        self.continue_btn.state(['disabled'])  # 初始時禁用
        
        self.pause_btn = ttk.Button(control_frame, text="Pause", command=self.pause_update)
        self.pause_btn.pack(side=tk.RIGHT, padx=5)
        
        # 狀態標籤
        self.status_label = ttk.Label(control_frame, text="Status: Running")
        self.status_label.pack(side=tk.LEFT, padx=5)

    def log_message(self, message):
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.see(tk.END)

    def update_charts(self):
        self.temp_ax.clear()
        self.hum_ax.clear()
        
        # 設定圖表範圍
        y_temp_min = min(self.temps) - 0.5
        y_temp_max = max(self.temps) + 0.5
        y_hum_min = min(self.hums) - 0.5
        y_hum_max = max(self.hums) + 0.5
        
        # 繪製數據
        self.temp_ax.plot(self.times, self.temps, '-o', color='red')
        self.temp_ax.set_title('Temperature History')
        self.temp_ax.set_ylabel('Temperature (°C)')
        self.temp_ax.set_ylim(y_temp_min, y_temp_max)
        self.temp_ax.tick_params(axis='x', rotation=45)
        
        self.hum_ax.plot(self.times, self.hums, '-o', color='blue')
        self.hum_ax.set_title('Humidity History')
        self.hum_ax.set_ylabel('Humidity (%)')
        self.hum_ax.set_ylim(y_hum_min, y_hum_max)
        self.hum_ax.tick_params(axis='x', rotation=45)
        
        # 調整圖表布局
        plt.tight_layout()
        self.canvas.draw()

    def pause_update(self):
        self.paused = True
        self.continue_btn.state(['!disabled'])  # 啟用繼續按鈕
        self.pause_btn.state(['disabled'])      # 禁用暫停按鈕
        self.status_label.config(text="Status: Paused")
        self.log_message("=== Simulation Paused ===")

    def continue_update(self):
        self.paused = False
        self.continue_btn.state(['disabled'])   # 禁用繼續按鈕
        self.pause_btn.state(['!disabled'])     # 啟用暫停按鈕
        self.status_label.config(text="Status: Running")
        self.log_message("=== Simulation Resumed ===")
        self.update_data()  # 繼續更新

    def update_data(self):
        if self.paused:
            return

        if self.message_index >= len(MOCK_MESSAGES):
            # 當達到數據末尾時停止更新
            self.log_message("\n=== Simulation Completed ===")
            return

        data = MOCK_MESSAGES[self.message_index]
        
        # 模擬解密過程
        self.log_message(f"\n=== Received Message at {data['timestamp']} ===")
        self.log_message(f"Topic: {data['topic']}")
        self.log_message(f"Encrypted: {data['encrypted']}")
        
        # 如果有錯誤，顯示錯誤訊息並暫停
        if data['error']:
            self.log_message(f"ERROR: {data['error']}")
            self.log_message("Decryption failed!")
            self.pause_update()  # 自動暫停
            return
        else:
            # 更新數據
            self.temps.append(data['temp'])
            self.hums.append(data['hum'])
            self.times.append(data['timestamp'])
            
            # 更新顯示
            self.temp_label.config(text=f"Temperature: {data['temp']}°C")
            self.hum_label.config(text=f"Humidity: {data['hum']}%")
            
            # 更新圖表
            self.update_charts()
        
        self.message_index += 1
        
        # 每秒更新一次
        if self.message_index < len(MOCK_MESSAGES):
            self.root.after(1000, self.update_data)

    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    app = DHT11Simulator()
    app.run()
