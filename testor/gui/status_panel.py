import tkinter as tk
from tkinter import ttk

class StatusPanel:
    def __init__(self, parent):
        self.frame = ttk.LabelFrame(parent, text="System Status")
        
        # Create status indicators
        self.create_status_indicators()

    def create_status_indicators(self):
        # Connection status
        conn_frame = ttk.Frame(self.frame)
        conn_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Label(conn_frame, text="Connection:").pack(side=tk.LEFT)
        self.conn_status = ttk.Label(conn_frame, text="Disconnected")
        self.conn_status.pack(side=tk.RIGHT)

        # Device status
        device_frame = ttk.Frame(self.frame)
        device_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Label(device_frame, text="Device:").pack(side=tk.LEFT)
        self.device_status = ttk.Label(device_frame, text="Unknown")
        self.device_status.pack(side=tk.RIGHT)

        # Last message
        msg_frame = ttk.Frame(self.frame)
        msg_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Label(msg_frame, text="Last Message:").pack(side=tk.LEFT)
        self.last_message = ttk.Label(msg_frame, text="None")
        self.last_message.pack(side=tk.RIGHT)

    def update_status(self, status_data):
        if 'connection' in status_data:
            self.conn_status.config(text=status_data['connection'])
        if 'device' in status_data:
            self.device_status.config(text=status_data['device'])
        if 'last_message' in status_data:
            self.last_message.config(text=status_data['last_message'])
