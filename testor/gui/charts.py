import tkinter as tk
from tkinter import ttk
import matplotlib.dates as mdates
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from datetime import datetime
import matplotlib.colors as mcolors

class SensorCharts:
    def __init__(self, parent):
        self.frame = ttk.LabelFrame(parent, text="Sensor Data")
        
        # Create figure with subplots
        self.fig = Figure(figsize=(8, 6))
        self.temp_ax = self.fig.add_subplot(211)
        self.humid_ax = self.fig.add_subplot(212)
        
        # Configure axes
        self.temp_ax.set_title('Temperature')
        self.humid_ax.set_title('Humidity')
        self.temp_ax.set_ylabel('Temperature (°C)')
        self.humid_ax.set_ylabel('Humidity (%)')
        
        # Initialize data storage for multiple devices
        self.device_data = {}  # 格式: {device_mac: {'temps': [], 'humids': [], 'timestamps': []}}
        self.colors = list(mcolors.TABLEAU_COLORS)  # 為不同設備使用不同顏色
        
        # Create canvas
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def _format_device_name(self, mac):
        """Format device MAC for display"""
        if (mac.lower() == 'unknown'):
            return 'unknown'
        # 取前5個字符，去除冒號
        return mac.replace(':', '')[:5]

    def update_data(self, data):
        try:
            timestamp = data.get('timestamp', datetime.now())
            device_mac = data.get('device_mac', 'unknown')
            
            # Initialize device data if not exists
            if device_mac not in self.device_data:
                self.device_data[device_mac] = {
                    'temps': [],
                    'humids': [],
                    'timestamps': [],
                    'humid_timestamps': [],  # 為濕度添加單獨的時間戳列表
                    'color': self.colors[len(self.device_data) % len(self.colors)],
                    'display_name': self._format_device_name(device_mac)
                }
            
            device = self.device_data[device_mac]
            
            # Update temperature data
            if 'temperature' in data and data['temperature'] is not None:
                try:
                    temp_value = float(data['temperature'])
                    device['timestamps'].append(timestamp)
                    device['temps'].append(temp_value)
                    
                    # Keep last 50 points
                    if len(device['temps']) > 50:
                        device['temps'].pop(0)
                        device['timestamps'].pop(0)
                except ValueError:
                    pass
            
            # Update humidity data with its own timestamp
            if 'humidity' in data and data['humidity'] is not None:
                try:
                    humid_value = float(data['humidity'])
                    device['humids'].append(humid_value)
                    device['humid_timestamps'].append(timestamp)
                    
                    # Keep last 50 points for humidity
                    if len(device['humids']) > 50:
                        device['humids'].pop(0)
                        device['humid_timestamps'].pop(0)
                except ValueError:
                    pass

            # Redraw plots
            self._update_temperature_plot()
            self._update_humidity_plot()
            
            # Update layout
            self.fig.tight_layout()
            self.canvas.draw()
            
        except Exception as e:
            print(f"Error updating charts: {str(e)}")
            import traceback
            traceback.print_exc()  # 添加詳細的錯誤追踪

    def _update_temperature_plot(self):
        self.temp_ax.clear()
        self.temp_ax.set_title('Temperature')
        self.temp_ax.set_ylabel('Temperature (°C)')
        self.temp_ax.grid(True, linestyle='--', alpha=0.7)
        
        # Plot each device's temperature data
        all_temps = []
        for mac, device in self.device_data.items():
            if device['temps'] and device['timestamps']:
                self.temp_ax.plot(device['timestamps'], device['temps'], 
                                color=device['color'], 
                                label=f"Device: {device['display_name']}",
                                marker='o', linestyle='-', linewidth=2, markersize=4)
                all_temps.extend(device['temps'])
        
        # Set y-axis limits
        if all_temps:
            margin = (max(all_temps) - min(all_temps)) * 0.1 + 1
            self.temp_ax.set_ylim([min(all_temps) - margin, max(all_temps) + margin])
        
        self.temp_ax.legend(loc='upper right', fontsize='small')
        self.temp_ax.tick_params(axis='x', rotation=45)

    def _update_humidity_plot(self):
        self.humid_ax.clear()
        self.humid_ax.set_title('Humidity')
        self.humid_ax.set_ylabel('Humidity (%)')
        self.humid_ax.grid(True, linestyle='--', alpha=0.7)
        
        # Plot each device's humidity data
        all_humids = []
        for mac, device in self.device_data.items():
            if device['humids'] and device['humid_timestamps']:
                self.humid_ax.plot(device['humid_timestamps'], device['humids'], 
                                 color=device['color'],
                                 label=f"Device: {device['display_name']}", 
                                 marker='o', linestyle='-', linewidth=2, markersize=4)
                all_humids.extend(device['humids'])
        
        # Set y-axis limits
        if all_humids:
            margin = (max(all_humids) - min(all_humids)) * 0.1 + 1
            self.humid_ax.set_ylim([min(all_humids) - margin, max(all_humids) + margin])
        
        if self.device_data:  # 只在有數據時添加圖例
            self.humid_ax.legend(loc='upper right', fontsize='small')
        
        self.humid_ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        self.humid_ax.tick_params(axis='x', rotation=45)
