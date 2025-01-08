import tkinter as tk
from tkinter import ttk, scrolledtext
from queue import Queue
import matplotlib.dates as mdates
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from datetime import datetime
import json
from testor.mqtt.publisher import TestMessagePublisher
from testor.utils.config import *
from testor.gui.test_panel import TestPanel
from testor.gui.charts import SensorCharts
from testor.gui.status_panel import StatusPanel

class MQTTUI:
    def __init__(self, subscriber):
        self.subscriber = subscriber
        self.root = tk.Tk()
        self.root.title("ESP32 MQTT Monitor")
        self.data_queue = Queue()
        
        # Create publisher before setting up window
        self.test_publisher = TestMessagePublisher(self)
        
        # Initialize UI components
        self.setup_window()
        
        # Setup logging and updates after window creation
        self.log_message(f"ESP32 MQTT Monitor Started")
        self.log_message("----------------------------")
        self.setup_periodic_update()
        self.subscriber.set_logger(self.log_message)

    def setup_window(self):
        self.root.geometry("1200x800")
        
        # Create main container
        main_container = ttk.Frame(self.root)
        main_container.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Left panel for controls
        left_panel = ttk.Frame(main_container)
        left_panel.pack(side=tk.LEFT, fill=tk.BOTH, padx=(0, 10))

        # Right panel for charts
        right_panel = ttk.Frame(main_container)
        right_panel.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        # Create status panel
        self.status_panel = StatusPanel(left_panel)
        self.status_panel.frame.pack(fill=tk.X, pady=(0, 10))

        # Create test panel
        self.test_panel = TestPanel(left_panel, self.test_publisher)
        self.test_panel.frame.pack(fill=tk.X, pady=(0, 10))

        # Create charts
        self.charts = SensorCharts(right_panel)
        self.charts.frame.pack(fill=tk.BOTH, expand=True)

        # Create log area
        self.create_log_area(left_panel)

    def create_log_area(self, parent):
        log_frame = ttk.LabelFrame(parent, text="Log Messages")
        log_frame.pack(fill=tk.BOTH, expand=True)

        self.log_text = scrolledtext.ScrolledText(log_frame, height=10)
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

    def log_message(self, message, level="INFO"):
        """Centralized logging function"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        prefix = f"[{level}]" if level != "INFO" else ""
        
        # Format the message more clearly
        if isinstance(message, dict):
            message = json.dumps(message, indent=2)
        
        log_text = f"[{timestamp}]{prefix} {message}"
        
        # Add to queue instead of direct update
        self.data_queue.put({
            'type': 'log',
            'message': log_text
        })

        # If it's a status message, also update status panel
        if "Connection status" in message or "Subscribed to" in message:
            self.data_queue.put({
                'type': 'status',
                'connection': message
            })

    def update_log(self, message):
        """Update log in GUI thread"""
        self.log_text.insert(tk.END, f"{message}\n")
        self.log_text.see(tk.END)

    def update_charts(self, data):
        self.charts.update_data(data)

    def update_status(self, status_data):
        self.status_panel.update_status(status_data)

    def setup_periodic_update(self):
        self.check_queue()

    def check_queue(self):
        try:
            while not self.data_queue.empty():
                # 改用 get_nowait() 來避免阻塞
                try:
                    data = self.data_queue.get_nowait()
                    if 'type' in data:
                        if data['type'] == 'status':
                            self.update_status(data)
                        elif data['type'] == 'sensor':
                            self.update_charts(data)
                        elif data['type'] == 'log':
                            self.update_log(data['message'])
                except queue.Empty:
                    break
        except Exception as e:
            self.update_log(f"Error processing data: {str(e)}")
        finally:
            # 使用更短的間隔來檢查佇列
            self.root.after(50, self.check_queue)

    def start(self):
        self.subscriber.set_queue(self.data_queue)
        self.subscriber.start()
        self.root.mainloop()

    def stop(self):
        self.subscriber.stop()
        self.root.quit()
