import tkinter as tk
from tkinter import ttk
from testor.utils.config import *

class TestPanel:
    def __init__(self, parent, publisher):
        self.publisher = publisher
        self.frame = ttk.LabelFrame(parent, text="Test Controls")
        
        # Create configuration fields
        self.create_config_fields()
        
        # Create message input
        self.create_message_input()
        
        # Create buttons
        self.create_buttons()

    def create_config_fields(self):
        config_frame = ttk.LabelFrame(self.frame, text="Configuration")
        config_frame.pack(fill=tk.X, padx=5, pady=5)

        # Group input
        group_frame = ttk.Frame(config_frame)
        group_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Label(group_frame, text="Group:").pack(side=tk.LEFT)
        self.group_entry = ttk.Entry(group_frame)
        self.group_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        self.group_entry.insert(0, GROUP_NAME)  # Default value

        # Device MAC input
        device_frame = ttk.Frame(config_frame)
        device_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Label(device_frame, text="Device MAC:").pack(side=tk.LEFT)
        self.device_entry = ttk.Entry(device_frame)
        self.device_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        self.device_entry.insert(0, DEVICE_MACS[0])  # Default value

        # Topic type dropdown
        topic_frame = ttk.Frame(config_frame)
        topic_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Label(topic_frame, text="Topic Type:").pack(side=tk.LEFT)
        self.topic_var = tk.StringVar(value="temperature")
        topic_options = ["temperature", "humidity", "status"]
        self.topic_combo = ttk.Combobox(topic_frame, textvariable=self.topic_var, values=topic_options)
        self.topic_combo.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))

    def create_message_input(self):
        message_frame = ttk.LabelFrame(self.frame, text="Message")
        message_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Message input
        ttk.Label(message_frame, text="Value:").pack(side=tk.LEFT)
        self.message_entry = ttk.Entry(message_frame)
        self.message_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        self.message_entry.insert(0, "25.5")  # Default value

    def create_buttons(self):
        button_frame = ttk.Frame(self.frame)
        button_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Add test buttons
        ttk.Button(button_frame, text="Send Test", command=self._send_test).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Start Auto Test", command=self._start_auto_test).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Stop Auto Test", command=self._stop_auto_test).pack(side=tk.LEFT, padx=5)

    def _send_test(self):
        message = self.message_entry.get()
        if message:
            self.publisher.publish_test_message(
                message=message,
                group=self.group_entry.get(),
                device=self.device_entry.get(),
                topic_type=self.topic_var.get()
            )

    def _start_auto_test(self):
        self.publisher.start_auto_test()

    def _stop_auto_test(self):
        self.publisher.stop_auto_test()
