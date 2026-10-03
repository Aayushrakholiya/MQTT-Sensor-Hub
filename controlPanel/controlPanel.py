#
# FILE          : main.cpp
# PROJECT       : SENG3030_Project
# PROGRAMMER    : Ayushkumar Rakholiya, Darsh Patel, Viraj Solanki, Jal Shah
# FIRST VERSION : 2025-11-25
# DESCRIPTION   :
#   This file implements a Tkinter-based GUI control panel for managing MQTT subscriptions
#

import ssl
import time
from collections import defaultdict
from typing import Dict, Set, List, Tuple

import tkinter as tk
from tkinter import ttk, messagebox

import paho.mqtt.client as mqtt

# MQTT CONFIGURATION
MQTT_BROKER = "p07da41d.ala.us-east-1.emqxsl.com"
MQTT_PORT = 8883
MQTT_USERNAME = "Vsolanki1864"
MQTT_PASSWORD = "1864"
COURSE_SUBSCRIPTION = "SENG3030/#"


topics_by_source: Dict[str, Dict[str, str]] = defaultdict(dict)
message_log: List[Tuple[str, str, str]] = []
active_topic_filters: Set[str] = set()
show_only_subscribed: bool = False
client: mqtt.Client | None = None

#---------------------------------------------------------
#  FUNCTION      : make_day_agnostic_filter
#  DESCRIPTION   :
#    Given a topic like "SENG3030/Friday/Arakholiya1894/sht40/temperature",
#  PARAMETERS    : 
#    - topic : str : original topic
#  RETURNS       : 
#    - str : topic
#---------------------------------------------------------
def make_day_agnostic_filter(topic: str) -> str:
    parts = topic.split("/")
    if len(parts) >= 3 and parts[0] == "SENG3030":
        parts[1] = "+" 
        return "/".join(parts)
    return topic


# -------------------------------------------------------------------
#  FUNCTION      : mqtt_topic_matches
#  DESCRIPTION   :
#    Check if a topic matches a given MQTT-style pattern with '+' and '#' wildcards.
#  PARAMETERS    : 
#    - pattern : str : topic pattern (may include '+' and '#')
#    - topic   : str : actual topic to check
#  RETURNS       : 
#    - bool : True if topic matches pattern, else False
#---------------------------------------------------------
def mqtt_topic_matches(pattern: str, topic: str) -> bool:
    p_parts = pattern.split("/")
    t_parts = topic.split("/")

    pi = ti = 0

    while pi < len(p_parts) and ti < len(t_parts):
        p = p_parts[pi]

        if p == "#":
            return True

        if p != "+" and p != t_parts[ti]:
            return False

        pi += 1
        ti += 1

    if pi == len(p_parts) and ti == len(t_parts):
        return True

    if pi == len(p_parts) - 1 and p_parts[pi] == "#":
        return True

    return False

# -------------------------------------------------------------------
#  FUNCTION      : extract_source_from_topic
#  DESCRIPTION   :
#    Extract the source user from a topic like "SENG3030/Friday/Arakholiya1894/sht40/temperature".
#  PARAMETERS    :
#    - topic : str : full MQTT topic string
#  RETURNS       :
#    - str : source user or "unknown" if not found
#-------------------------------------------------------------------
def extract_source_from_topic(topic: str) -> str:
    parts = topic.split("/")
    if len(parts) >= 3 and parts[0] == "SENG3030":
        return parts[2]
    return "unknown"

# -------------------------------------------------------------------
# FUNCTION      : on_connect
# DESCRIPTION   :
#   This function is called when the MQTT client connects to the broker.
# PARAMETERS    :
#   - mqtt_client : mqtt.Client : the MQTT client instance
#   - userdata    : any         : user-defined data of any type
#   - flags       : dict        : response flags sent by the broker
#   - rc          : int         : connection result
#   - properties  : mqtt.Properties or None : MQTT v5.0 properties
# RETURNS       : None
# -------------------------------------------------------------------
def on_connect(mqtt_client: mqtt.Client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("[INFO] Connected to broker.")
        mqtt_client.subscribe(COURSE_SUBSCRIPTION, qos=1)
        print(f"[INFO] Subscribed to '{COURSE_SUBSCRIPTION}'")
    else:
        print(f"[ERROR] Connection failed with code {rc}")


# -------------------------------------------------------------------
# FUNCTION      : on_message
# DESCRIPTION   :
#   This function is called when a message is received on a subscribed topic.
# PARAMETERS    :
#   - mqtt_client : mqtt.Client : the MQTT client instance
#   - userdata    : any         : user-defined data of any type
#   - msg         : mqtt.MQTTMessage : the received MQTT message
# RETURNS       : None
# -------------------------------------------------------------------
def on_message(mqtt_client: mqtt.Client, userdata, msg: mqtt.MQTTMessage):
    global show_only_subscribed

    topic = msg.topic
    payload = msg.payload.decode(errors="replace")

    source = extract_source_from_topic(topic)
    topics_by_source[source][topic] = payload

    if show_only_subscribed:
        if not active_topic_filters:
            return
        if not any(mqtt_topic_matches(filt, topic) for filt in active_topic_filters):
            return

    ts = time.strftime("%H:%M:%S")
    message_log.append((ts, topic, payload))

    if len(message_log) > 200:
        del message_log[:50] 

def on_disconnect(mqtt_client: mqtt.Client, userdata, rc, properties=None):
    print(f"[INFO] Disconnected from broker (rc={rc}).")

# -------------------------------------------------------------------
# FUNCTION      : send_subscribe
# DESCRIPTION   :
#   Send a subscribe command to a device via MQTT.
# PARAMETERS    :
#   - device_user : str : the user/device to send the subscribe command to
#   - topic_filter : str : the topic filter to subscribe to
# RETURNS       : None
# -------------------------------------------------------------------
def send_subscribe(device_user: str, topic_filter: str) -> None:
    """
    Ask a remote device to subscribe to topic_filter, and remember that filter
    so only those messages are shown in the GUI.
    """
    global show_only_subscribed

    if client is None:
        print("[ERROR] MQTT client is not initialized.")
        return

    control_topic = f"{MQTT_USERNAME}/{device_user}/subscribe"
    client.publish(control_topic, topic_filter, qos=1, retain=False)

    active_topic_filters.add(topic_filter)
    show_only_subscribed = True

    print(f"[CONTROL] SUBSCRIBE -> {control_topic}  payload='{topic_filter}'")
    print("[INFO] GUI will show only subscribed messages.")


# -------------------------------------------------------------------
# FUNCTION      : send_unsubscribe
# DESCRIPTION   :
#   Send an unsubscribe command to a device via MQTT.
# PARAMETERS    :
#   - device_user : str : the user/device to send the unsubscribe command to
#   - topic_filter : str : the topic filter to unsubscribe from
# RETURNS       : None
# -------------------------------------------------------------------
def send_unsubscribe(device_user: str, topic_filter: str) -> None:
    """
    Ask the device to unsubscribe from topic_filter, and remove that filter.
    If no filters remain, show all messages again.
    """
    global show_only_subscribed

    if client is None:
        print("[ERROR] MQTT client is not initialized.")
        return

    control_topic = f"{MQTT_USERNAME}/{device_user}/unsubscribe"
    client.publish(control_topic, topic_filter, qos=1, retain=False)

    if topic_filter in active_topic_filters:
        active_topic_filters.remove(topic_filter)

    if not active_topic_filters:
        show_only_subscribed = False
        print("[INFO] No active filters. GUI will show ALL messages.")

    print(f"[CONTROL] UNSUBSCRIBE -> {control_topic}  payload='{topic_filter}'")


#-------------------------------------------------------------
# CLASS : ControlPanelApp
# PURPOSE :
#   Implements the Tkinter GUI for the MQTT control panel.
#-------------------------------------------------------------
class ControlPanelApp:

    #---------------------------------------------------------
    # FUNCTION      : __init__
    # DESCRIPTION   :
    #   Initialize the GUI components.
    # PARAMETERS    :
    #     - root : tk.Tk : the root Tkinter window
    # RETURNS       : None
    #---------------------------------------------------------
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("SENG3030 MQTT Control Panel")
        self.root.geometry("900x500")

        self.left_frame = ttk.Frame(self.root, padding=5)
        self.right_frame = ttk.Frame(self.root, padding=5)

        self.left_frame.pack(side=tk.LEFT, fill=tk.Y)
        self.right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        ttk.Label(self.left_frame,text="Devices (sources)",font=("Segoe UI",10,"bold")).pack(anchor="w");self.device_listbox=tk.Listbox(self.left_frame,height=20,selectmode=tk.SINGLE,exportselection=False);self.device_listbox.pack(fill=tk.Y,expand=True);self.device_listbox.bind("<ButtonRelease-1>",self.on_device_click)

        top_frame = ttk.Frame(self.right_frame)
        top_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        ttk.Label(top_frame, text="Topics for selected device", font=("Segoe UI", 10, "bold")).pack(anchor="w")

        self.topic_listbox = tk.Listbox(top_frame, height=10)
        self.topic_listbox.pack(fill=tk.BOTH, expand=True)

        controls_frame = ttk.Frame(self.right_frame)
        controls_frame.pack(side=tk.TOP, fill=tk.X, pady=5)

        ttk.Label(controls_frame, text="Selected topic filter:").grid(row=0, column=0, sticky="w")

        self.selected_topic_var = tk.StringVar()
        self.selected_topic_entry = ttk.Entry(controls_frame, textvariable=self.selected_topic_var, width=60)
        self.selected_topic_entry.grid(row=0, column=1, padx=5, pady=2, sticky="we")

        self.subscribe_button = ttk.Button(controls_frame, text="Subscribe", command=self.on_subscribe_clicked)
        self.subscribe_button.grid(row=0, column=2, padx=5, pady=2)

        self.unsubscribe_button = ttk.Button(controls_frame, text="Unsubscribe", command=self.on_unsubscribe_clicked)
        self.unsubscribe_button.grid(row=0, column=3, padx=5, pady=2)

        controls_frame.columnconfigure(1, weight=1)

        self.filter_status_var = tk.StringVar(value="Filters: none (showing all messages)")
        self.filter_status_label = ttk.Label(self.right_frame, textvariable=self.filter_status_var)
        self.filter_status_label.pack(anchor="w", pady=(0, 3))

        ttk.Label(self.right_frame, text="Message Log", font=("Segoe UI", 10, "bold")).pack(anchor="w")

        self.log_text = tk.Text(self.right_frame, height=12, state=tk.DISABLED)
        self.log_text.pack(fill=tk.BOTH, expand=True)

        self._last_devices: List[str] = []

        self.root.after(500, self.refresh_ui)

    #---------------------------------------------------------
    # FUNCTION      : on_device_selected
    # DESCRIPTION   :
    #   Handle device selection changes.
    # PARAMETERS    :
    #     - event : tk.Event : the selection event
    # RETURNS       : None
    #---------------------------------------------------------
    def on_device_selected(self, event):
        self.update_topics_for_selected_device()
    
    #---------------------------------------------------------
    # FUNCTION      : on_device_click
    # DESCRIPTION   :
    #   Handle device click events to update selection.
    # PARAMETERS    :
    #     - event : tk.Event : the click event
    # RETURNS       : None
    #---------------------------------------------------------
    def on_device_click(self,event):
        idx=self.device_listbox.nearest(event.y);self.device_listbox.selection_clear(0,tk.END);self.device_listbox.selection_set(idx);self.device_listbox.activate(idx);self.update_topics_for_selected_device()
    
    #---------------------------------------------------------
    # FUNCTION      : on_subscribe_clicked
    # DESCRIPTION   :
    #   Handle subscribe button clicks.
    # PARAMETERS    : None
    # RETURNS       : None
    #---------------------------------------------------------
    def on_subscribe_clicked(self):
        device_user = self.get_selected_device()
        raw_filter = self.selected_topic_var.get().strip()

        if not device_user:
            messagebox.showwarning("No device selected", "Please select a device first.")
            return
        if not raw_filter:
            messagebox.showwarning("No topic", "Please choose a topic or type a topic filter.")
            return

        topic_filter = make_day_agnostic_filter(raw_filter)

        send_subscribe(device_user, topic_filter)
        self.update_filter_status()

    #---------------------------------------------------------
    # FUNCTION      : on_unsubscribe_clicked
    # DESCRIPTION   :
    #   Handle unsubscribe button clicks.
    # PARAMETERS    : None
    # RETURNS       : None
    #---------------------------------------------------------
    def on_unsubscribe_clicked(self):
        device_user = self.get_selected_device()
        topic_filter = self.selected_topic_var.get().strip()

        if not device_user:
            messagebox.showwarning("No device selected", "Please select a device first.")
            return
        if not topic_filter:
            messagebox.showwarning("No topic", "Please choose a topic or type a topic filter.")
            return

        send_unsubscribe(device_user, topic_filter)
        self.update_filter_status()

    #---------------------------------------------------------
    # FUNCTION      : get_selected_device
    # DESCRIPTION   :
    #   Get the currently selected device from the listbox.
    # PARAMETERS    : None
    # RETURNS       : str | None
    #---------------------------------------------------------
    def get_selected_device(self) -> str | None:
        sel = self.device_listbox.curselection()
        if not sel:
            return None
        idx = sel[0]
        return self.device_listbox.get(idx)
    
    #---------------------------------------------------------
    # FUNCTION      : update_devices_list
    # DESCRIPTION   :
    #   Refresh the device list but keep the current selection if possible.
    # PARAMETERS    : None
    # RETURNS       : None
    #---------------------------------------------------------
    def update_devices_list(self):
        """Refresh the device list but keep the current selection if possible."""
        previously_selected = self.get_selected_device()

        current_devices = sorted(topics_by_source.keys())

        if current_devices == self._last_devices and previously_selected in current_devices:
            return

        self.device_listbox.delete(0, tk.END)
        selected_index = None

        for idx, d in enumerate(current_devices):
            self.device_listbox.insert(tk.END, d)
            if d == previously_selected:
                selected_index = idx

        if selected_index is not None:
            self.device_listbox.selection_set(selected_index)
        elif current_devices:
            self.device_listbox.selection_set(0)

        self._last_devices = current_devices

    #---------------------------------------------------------
    # FUNCTION      : update_topics_for_selected_device
    # DESCRIPTION   :
    #   Update the list of topics for the currently selected device.
    # PARAMETERS    : None
    # RETURNS       : None
    #---------------------------------------------------------
    def update_topics_for_selected_device(self):
        device_user = self.get_selected_device()
        if not device_user:
            self.topic_listbox.delete(0, tk.END)
            self.selected_topic_var.set("")
            return

        topic_map = topics_by_source.get(device_user, {})

        old_sel = None
        sel = self.topic_listbox.curselection()
        if sel:
            old_sel = self.topic_listbox.get(sel[0])

        self.topic_listbox.delete(0, tk.END)
        topics = sorted(topic_map.keys())
        for t in topics:
            self.topic_listbox.insert(tk.END, t)

        if old_sel in topics:
            idx = topics.index(old_sel)
            self.topic_listbox.selection_set(idx)
            self.topic_listbox.activate(idx)
            self.selected_topic_var.set(old_sel)

        elif topics:
            if not sel: 
                self.topic_listbox.selection_set(0)
                self.topic_listbox.activate(0)
                self.selected_topic_var.set(topics[0])

    #---------------------------------------------------------
    # FUNCTION      : update_log_text
    # DESCRIPTION   :
    #   Refresh the message log display.
    # PARAMETERS    : None
    # RETURNS       : None
    #---------------------------------------------------------
    def update_log_text(self):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete("1.0", tk.END)
        for ts, topic, payload in message_log:
            self.log_text.insert(tk.END, f"[{ts}] {topic} -> {payload}\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)

    #---------------------------------------------------------
    # FUNCTION      : update_filter_status
    # DESCRIPTION   :
    #   Update the filter status label.
    # PARAMETERS    : None
    # RETURNS       : None
    #---------------------------------------------------------
    def update_filter_status(self):
        if active_topic_filters:
            filters_str = ", ".join(sorted(active_topic_filters))
            if show_only_subscribed:
                self.filter_status_var.set(f"Filters: {filters_str} (showing ONLY subscribed messages)")
            else:
                self.filter_status_var.set(f"Filters: {filters_str} (filters set, but show_all mode)")
        else:
            self.filter_status_var.set("Filters: none (showing ALL messages)")

    #---------------------------------------------------------
    # FUNCTION      : refresh_ui
    # DESCRIPTION   :
    #   Periodically refresh the UI components.
    # PARAMETERS    : None
    # RETURNS       : None
    #---------------------------------------------------------
    def refresh_ui(self):
        self.update_devices_list()
        self.update_topics_for_selected_device()
        self.update_log_text()
        self.update_filter_status()
        self.root.after(500, self.refresh_ui)


# ---------------------------------------------------------
# FUNCTION      : start_mqtt
# DESCRIPTION   :
#   Initialize and start the MQTT client.
# PARAMETERS    : None
# RETURNS       : None
#---------------------------------------------------------
def start_mqtt():
    global client

    c = mqtt.Client(client_id="SENG3030_ControlPanel_GUI", protocol=mqtt.MQTTv5)
    c.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

    c.tls_set(
        certfile=None,
        keyfile=None,
        cert_reqs=ssl.CERT_REQUIRED,
        tls_version=ssl.PROTOCOL_TLS_CLIENT,
        ciphers=None,
    )

    c.on_connect = on_connect
    c.on_message = on_message
    c.on_disconnect = on_disconnect

    print(f"[INFO] Connecting to {MQTT_BROKER}:{MQTT_PORT} as {MQTT_USERNAME} ...")
    c.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)

    c.loop_start()

    client = c


def main():
    start_mqtt()

    root = tk.Tk()
    app = ControlPanelApp(root)

    try:
        root.mainloop()
    finally:
        if client is not None:
            print("[INFO] Shutting down MQTT...")
            client.loop_stop()
            client.disconnect()
            time.sleep(1)


if __name__ == "__main__":
    main()
