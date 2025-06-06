import queue
import threading
import time
import tkinter as tk
from tkinter import messagebox
import ttkbootstrap as ttk
import zmq
from ttkbootstrap.constants import *
from ttkbootstrap.scrolled import ScrolledText
import json # For parsing statistics

class ZeroMQGUIClient(ttk.Window):
    def __init__(self):
        super().__init__(themename="superhero")
        self.title("ZeroMQ Data Manager & Predictor Stats") # Updated title
        self.geometry("1200x850") # Increased size for stats display

        # For C++ Data Server
        self.cpp_server_address = "tcp://app-main:5558"
        self.cpp_response_queue = queue.Queue()
        
        # For Python Predictor Statistics (NEW)
        self.predictor_stats_address = "tcp://live-predictor:5559" # Assuming service name in Docker
        self.predictor_stats_topic = "prediction_stats"
        self.stats_queue = queue.Queue() # Queue for stats messages

        self.zmq_context = zmq.Context()
        self.stop_event = threading.Event()

        self.data_structure_map = {
            "AVL": 1, "LINKED_LIST": 2, "HASHSET": 3,
            "CUCKOO_HASH": 4, "SEGMENT_TREE": 5, "RED_BLACK_TREE": 6
        }
        self.statistic_features_map = {
            "Duration (dur)": 0, "Rate": 1, "Source Load (sload)": 2,
            "Destination Load (dload)": 3, "Source Packets (spkts)": 4,
            "Destination Packets (dpkts)": 5, "Source Bytes (sbytes)": 6,
            "Destination Bytes (dbytes)": 7,
        }

        self._create_widgets()
        self.process_cpp_response_queue()
        self.start_stats_listener() # Start the new listener
        self.process_stats_queue()  # Start processing stats from the queue
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self._log_message("[SYS] GUI Client initialized.")

    def _create_widgets(self):
        # Main container frame
        main_container = ttk.Frame(self, padding="10")
        main_container.pack(fill=BOTH, expand=YES)

        # Left frame for C++ server interaction
        left_frame = ttk.Frame(main_container)
        left_frame.pack(side=LEFT, fill=BOTH, expand=True, padx=(0, 10))

        # Right frame for predictor statistics (NEW)
        right_frame = ttk.Labelframe(main_container, text="Live Prediction Statistics", padding="10")
        right_frame.pack(side=RIGHT, fill=BOTH, expand=True)

        # --- Widgets for C++ Server Interaction (in left_frame) ---
        top_level_operation_frame = ttk.Labelframe(left_frame, text="Select Main Operation", padding="10")
        top_level_operation_frame.pack(fill=X, pady=(0, 10))

        self.selected_top_level_operation = tk.StringVar(value="GET_DATA")
        operations = [
            ("Query Last 3 Data", "GET_DATA"), ("Query Data by ID", "QUERY_DATA_BY_ID"),
            ("Remove Data by ID", "REMOVE_DATA_BY_ID"), ("Perform Statistics", "PERFORM_STATS")
        ]
        for i, (text, value) in enumerate(operations):
            rb = ttk.Radiobutton(
                top_level_operation_frame, text=text, variable=self.selected_top_level_operation,
                value=value, command=self._toggle_input_fields, 
                bootstyle="info-round-toggle" if "Query" in text or "Perform" in text else "danger-round-toggle"
            )
            rb.grid(row=i // 2, column=i % 2, padx=5, pady=2, sticky="w")
        for i in range(2): top_level_operation_frame.grid_columnconfigure(i, weight=1)

        self.data_structure_selection_frame = ttk.Labelframe(left_frame, text="Select Data Structure", padding="10")
        self.selected_data_structure = tk.StringVar(value="AVL")
        all_ds_options = [
            ("AVL Tree", "AVL"), ("Linked List", "LINKED_LIST"), ("Hashset", "HASHSET"),
            ("Cuckoo Hash", "CUCKOO_HASH"), ("Segment Tree", "SEGMENT_TREE"), ("Red-Black Tree", "RED_BLACK_TREE")
        ]
        self.ds_radio_buttons = {}
        for i, (text, value) in enumerate(all_ds_options):
            rb = ttk.Radiobutton(
                self.data_structure_selection_frame, text=text, variable=self.selected_data_structure,
                value=value, bootstyle="info-round-toggle")
            rb.grid(row=i // 3, column=i % 3, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons[value] = rb
        for i in range(3): self.data_structure_selection_frame.grid_columnconfigure(i, weight=1)

        self.input_fields_frame = ttk.Frame(left_frame)
        self.id_label = ttk.Label(self.input_fields_frame, text="Data ID:", bootstyle="secondary")
        self.id_entry = ttk.Entry(self.input_fields_frame, bootstyle="info", width=20)
        self.id_label.grid(row=0, column=0, padx=(0, 5), pady=2, sticky="w")
        self.id_entry.grid(row=0, column=1, padx=(0, 10), pady=2, sticky="ew")
        self.input_fields_frame.grid_columnconfigure(1, weight=1)

        self.stats_options_frame = ttk.Labelframe(left_frame, text="Statistics Parameters", padding="10")
        self.feature_label = ttk.Label(self.stats_options_frame, text="Select Feature:", bootstyle="secondary")
        self.feature_label.grid(row=0, column=0, padx=(0,5), pady=2, sticky="w")
        self.selected_feature = tk.StringVar(value=list(self.statistic_features_map.keys())[0])
        self.feature_combobox = ttk.Combobox(
            self.stats_options_frame, textvariable=self.selected_feature,
            values=list(self.statistic_features_map.keys()), state="readonly", bootstyle="info")
        self.feature_combobox.grid(row=0, column=1, padx=(0,10), pady=2, sticky="ew")
        self.stats_options_frame.grid_columnconfigure(1, weight=1)
        self.interval_label = ttk.Label(self.stats_options_frame, text="Interval (last N items):", bootstyle="secondary")
        self.interval_label.grid(row=1, column=0, padx=(0, 5), pady=2, sticky="w")
        self.interval_entry = ttk.Entry(self.stats_options_frame, bootstyle="info", width=15)
        self.interval_entry.insert(0, "100")
        self.interval_entry.grid(row=1, column=1, padx=(0, 10), pady=2, sticky="ew")

        control_frame = ttk.Frame(left_frame)
        control_frame.pack(fill=X, pady=(5, 10))
        self.status_label = ttk.Label(control_frame, text=f"C++ Server: {self.cpp_server_address}", bootstyle="info")
        self.status_label.pack(side=LEFT, padx=(0, 10))
        self.execute_button = ttk.Button(control_frame, text="Execute Operation", command=self.execute_selected_operation, bootstyle="primary")
        self.execute_button.pack(side=LEFT, padx=5)
        self.clear_log_button = ttk.Button(control_frame, text="Clear Log", command=self.clear_log, bootstyle="secondary")
        self.clear_log_button.pack(side=LEFT, padx=5)

        log_frame = ttk.Labelframe(left_frame, text="Responses from C++ Server / Logs", padding="10")
        log_frame.pack(fill=BOTH, expand=YES)
        self.log_text = ScrolledText(log_frame, height=15, width=70, wrap=WORD, autohide=True) # Adjusted height
        self.log_text.pack(fill=BOTH, expand=YES)
        self.log_text.text.configure(state="disabled")

        # --- Widgets for Predictor Statistics (in right_frame) (NEW) ---
        self.stats_vars = {
            "total_processed": tk.StringVar(value="Total: N/A"),
            "tp": tk.StringVar(value="TP: N/A"),
            "fp": tk.StringVar(value="FP: N/A"),
            "fn": tk.StringVar(value="FN: N/A"),
            "tn": tk.StringVar(value="TN: N/A"),
            "accuracy": tk.StringVar(value="Accuracy: N/A"),
            "precision": tk.StringVar(value="Precision: N/A"),
            "recall": tk.StringVar(value="Recall: N/A"),
            "f1_score": tk.StringVar(value="F1-Score: N/A")
        }

        ttk.Label(right_frame, textvariable=self.stats_vars["total_processed"], font=("Helvetica", 12, "bold")).pack(pady=5, anchor="w")
        
        cm_frame = ttk.Frame(right_frame)
        cm_frame.pack(fill=X, pady=5)
        ttk.Label(cm_frame, textvariable=self.stats_vars["tp"], bootstyle="success", font=("Helvetica", 11)).pack(side=LEFT, padx=5)
        ttk.Label(cm_frame, textvariable=self.stats_vars["fp"], bootstyle="danger", font=("Helvetica", 11)).pack(side=LEFT, padx=5)
        
        cm_frame2 = ttk.Frame(right_frame)
        cm_frame2.pack(fill=X, pady=5)
        ttk.Label(cm_frame2, textvariable=self.stats_vars["fn"], bootstyle="warning", font=("Helvetica", 11)).pack(side=LEFT, padx=5)
        ttk.Label(cm_frame2, textvariable=self.stats_vars["tn"], bootstyle="success", font=("Helvetica", 11)).pack(side=LEFT, padx=5)

        ttk.Separator(right_frame, orient=HORIZONTAL).pack(fill=X, pady=10, padx=5)

        ttk.Label(right_frame, textvariable=self.stats_vars["accuracy"], font=("Helvetica", 11)).pack(pady=3, anchor="w")
        ttk.Label(right_frame, textvariable=self.stats_vars["precision"], font=("Helvetica", 11)).pack(pady=3, anchor="w")
        ttk.Label(right_frame, textvariable=self.stats_vars["recall"], font=("Helvetica", 11)).pack(pady=3, anchor="w")
        ttk.Label(right_frame, textvariable=self.stats_vars["f1_score"], font=("Helvetica", 11)).pack(pady=3, anchor="w")
        
        self.predictor_status_label = ttk.Label(right_frame, text=f"Stats from: {self.predictor_stats_address}", bootstyle="info")
        self.predictor_status_label.pack(side=BOTTOM, fill=X, pady=(10,0))


        self._toggle_input_fields()

    def _toggle_input_fields(self):
        selected_main_op = self.selected_top_level_operation.get()
        self.id_label.grid_remove()
        self.id_entry.grid_remove()
        self.stats_options_frame.pack_forget()
        self.data_structure_selection_frame.pack_forget()
        for rb in self.ds_radio_buttons.values(): rb.grid_remove()

        if selected_main_op == "QUERY_DATA_BY_ID" or selected_main_op == "REMOVE_DATA_BY_ID":
            self.id_label.grid(row=0, column=0, padx=(0, 5), pady=2, sticky="w")
            self.id_entry.grid(row=0, column=1, padx=(0, 10), pady=2, sticky="ew")
            self.input_fields_frame.pack(fill=X, pady=(0, 10))
            self.data_structure_selection_frame.pack(fill=X, pady=(0, 10))
            for i, value in enumerate(self.data_structure_map.keys()):
                self.ds_radio_buttons[value].grid(row=i // 3, column=i % 3, padx=5, pady=2, sticky="w")
            if self.selected_data_structure.get() not in self.data_structure_map:
                self.selected_data_structure.set("AVL")

        elif selected_main_op == "PERFORM_STATS":
            self.stats_options_frame.pack(fill=X, pady=(0, 10))
            self.data_structure_selection_frame.pack(fill=X, pady=(0, 10))
            stats_ds_options = ["SEGMENT_TREE", "LINKED_LIST"] # Only these for stats
            for i, value in enumerate(stats_ds_options):
                if value in self.ds_radio_buttons: # Check if radio button exists
                     self.ds_radio_buttons[value].grid(row=0, column=i, padx=5, pady=2, sticky="w")
            if self.selected_data_structure.get() not in stats_ds_options:
                self.selected_data_structure.set("LINKED_LIST")

    def _log_message(self, message, prefix="[LOG]", bootstyle="default"):
        self.log_text.text.configure(state="normal")
        self.log_text.text.insert(END, f"{prefix} {message}\n", bootstyle)
        self.log_text.text.see(END)
        self.log_text.text.configure(state="disabled")

    def clear_log(self):
        self.log_text.text.configure(state="normal")
        self.log_text.text.delete("1.0", END)
        self.log_text.text.configure(state="disabled")
        self._log_message("[SYS] Log cleared.")

    def execute_selected_operation(self):
        main_operation = self.selected_top_level_operation.get()
        selected_ds_str = self.selected_data_structure.get()
        selected_ds_num = self.data_structure_map.get(selected_ds_str)
        request_payload = ""

        if main_operation in ["QUERY_DATA_BY_ID", "REMOVE_DATA_BY_ID"]:
            data_id = self.id_entry.get().strip()
            if not data_id:
                messagebox.showwarning("Input Error", f"Please enter an ID for {main_operation}.")
                return
            try: int(data_id)
            except ValueError:
                messagebox.showwarning("Input Error", "ID must be a valid integer.")
                return
            request_payload = f"{main_operation} {data_id} {selected_ds_num}"
        elif main_operation == "PERFORM_STATS":
            selected_feature_name = self.selected_feature.get()
            feature_enum_value = self.statistic_features_map.get(selected_feature_name)
            interval = self.interval_entry.get().strip()
            if not interval:
                messagebox.showwarning("Input Error", "Please enter an interval for Statistics.")
                return
            try:
                interval_int = int(interval)
                if interval_int <= 0: raise ValueError("Interval must be positive.")
            except ValueError as e:
                messagebox.showwarning("Input Error", f"Interval must be a positive integer. Error: {e}")
                return
            if feature_enum_value is None:
                messagebox.showwarning("Input Error", "Please select a valid statistical feature.")
                return
            request_payload = f"GET_STATS_SUMMARY {feature_enum_value} {interval} {selected_ds_num}"
        elif main_operation == "GET_DATA":
            request_payload = "GET_DATA"
        else:
            messagebox.showwarning("Operation Error", "Unexpected operation selected.")
            return

        self._log_message(f"[REQ] Sending to C++: '{request_payload}'", prefix="", bootstyle="info")
        self.execute_button.config(state=DISABLED)
        threading.Thread(target=self._execute_zmq_request_cpp, args=(request_payload,), daemon=True).start()

    def _execute_zmq_request_cpp(self, request_message_payload):
        socket = None
        try:
            socket = self.zmq_context.socket(zmq.REQ)
            socket.setsockopt(zmq.LINGER, 0)
            socket.setsockopt(zmq.RCVTIMEO, 10000)
            socket.setsockopt(zmq.SNDTIMEO, 5000)
            socket.connect(self.cpp_server_address)
            socket.send_string(request_message_payload)
            response = socket.recv_string()
            self.cpp_response_queue.put(response)
        except zmq.error.Again:
            self.cpp_response_queue.put(f"[ERR] Timeout: No response from C++ server at {self.cpp_server_address}.")
        except zmq.ZMQError as e:
            self.cpp_response_queue.put(f"[ERR] C++ Server ZMQ Error: {e}")
        except Exception as e:
            self.cpp_response_queue.put(f"[ERR] C++ Server Unexpected error: {e}")
        finally:
            if socket: socket.close()
            self.after(0, lambda: self.execute_button.config(state=NORMAL))

    def process_cpp_response_queue(self):
        try:
            while not self.cpp_response_queue.empty():
                message = self.cpp_response_queue.get_nowait()
                if "[ERR]" in message: self._log_message(message, prefix="", bootstyle="danger")
                elif any(warn_str in message for warn_str in ["NOT IMPLEMENTED", "No data collected", "Unknown command", "No data with ID", "Error parsing"]):
                    self._log_message(message, prefix="[INFO]", bootstyle="warning")
                else: self._log_message(message, prefix="[RECV C++]", bootstyle="success")
                self.cpp_response_queue.task_done()
        except queue.Empty: pass
        if not self.stop_event.is_set():
            self.after(100, self.process_cpp_response_queue)

    # --- New methods for Predictor Statistics ---
    def start_stats_listener(self):
        """Starts a new thread to listen for predictor statistics."""
        self.stats_listener_thread = threading.Thread(target=self._listen_for_stats, daemon=True)
        self.stats_listener_thread.start()
        self._log_message(f"[SYS] Statistics listener started for {self.predictor_stats_address} on topic '{self.predictor_stats_topic}'.")

    def _listen_for_stats(self):
        """ZMQ SUB socket loop to receive statistics messages."""
        stats_subscriber = self.zmq_context.socket(zmq.SUB)
        try:
            stats_subscriber.connect(self.predictor_stats_address)
            stats_subscriber.subscribe(self.predictor_stats_topic.encode('utf-8'))
            
            while not self.stop_event.is_set():
                try:
                    # Set a timeout to allow the loop to check stop_event periodically
                    if stats_subscriber.poll(timeout=500): # Check for message for 500ms
                        topic, message_json = stats_subscriber.recv_multipart()
                        self.stats_queue.put(message_json.decode('utf-8'))
                except zmq.error.ContextTerminated:
                    self._log_message("[SYS] Statistics listener: ZMQ context terminated.", bootstyle="warning")
                    break
                except zmq.ZMQError as e:
                    if e.errno == zmq.ETERM:
                        self._log_message("[SYS] Statistics listener: ZMQ context terminated (ETERM).", bootstyle="warning")
                        break
                    self._log_message(f"[ERR] Statistics listener ZMQ Error: {e}", bootstyle="danger")
                    time.sleep(1) # Avoid tight loop on other ZMQ errors
                except Exception as e:
                    self._log_message(f"[ERR] Statistics listener unexpected error: {e}", bootstyle="danger")
                    time.sleep(1)
        finally:
            if stats_subscriber and not stats_subscriber.closed:
                stats_subscriber.close()
            self._log_message("[SYS] Statistics listener thread stopped.")


    def process_stats_queue(self):
        """Processes statistics messages from the queue and updates the GUI."""
        try:
            while not self.stats_queue.empty():
                message_json_str = self.stats_queue.get_nowait()
                try:
                    stats_data = json.loads(message_json_str)
                    # Update GUI labels
                    self.stats_vars["total_processed"].set(f"Total Processed: {stats_data.get('total_processed', 'N/A')}")
                    self.stats_vars["tp"].set(f"TP: {stats_data.get('tp', 'N/A')}")
                    self.stats_vars["fp"].set(f"FP: {stats_data.get('fp', 'N/A')}")
                    self.stats_vars["fn"].set(f"FN: {stats_data.get('fn', 'N/A')}")
                    self.stats_vars["tn"].set(f"TN: {stats_data.get('tn', 'N/A')}")
                    self.stats_vars["accuracy"].set(f"Accuracy: {stats_data.get('accuracy', 'N/A')}")
                    self.stats_vars["precision"].set(f"Precision: {stats_data.get('precision', 'N/A')}")
                    self.stats_vars["recall"].set(f"Recall: {stats_data.get('recall', 'N/A')}")
                    self.stats_vars["f1_score"].set(f"F1-Score: {stats_data.get('f1_score', 'N/A')}")
                except json.JSONDecodeError:
                    self._log_message(f"[ERR] Failed to parse statistics JSON: {message_json_str}", bootstyle="danger")
                except Exception as e:
                    self._log_message(f"[ERR] Error updating stats UI: {e}", bootstyle="danger")

                self.stats_queue.task_done()
        except queue.Empty:
            pass

        if not self.stop_event.is_set():
            self.after(200, self.process_stats_queue) # Check queue every 200ms

    def on_closing(self):
        self._log_message("[SYS] Closing application...")
        self.stop_event.set()
        # Wait for listener thread to join if it was started
        if hasattr(self, 'stats_listener_thread') and self.stats_listener_thread.is_alive():
            self.stats_listener_thread.join(timeout=1.0) 
            if self.stats_listener_thread.is_alive():
                 self._log_message("[WARN] Statistics listener thread did not join in time.", bootstyle="warning")


        if self.zmq_context and not self.zmq_context.closed:
            self._log_message("[SYS] Terminating ZMQ context...")
            self.zmq_context.term() # Terminate context to unblock any hanging sockets
        
        self._log_message("[SYS] Destroying window.")
        self.destroy()

if __name__ == "__main__":
    app = ZeroMQGUIClient()
    app.mainloop()

