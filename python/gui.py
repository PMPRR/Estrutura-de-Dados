# Criado por IA gemini 2.5 pro

import queue
import threading
import time
import tkinter as tk
from tkinter import messagebox
import ttkbootstrap as ttk
import zmq
from ttkbootstrap.constants import *
from ttkbootstrap.scrolled import ScrolledText
import json

class ZeroMQGUIClient(ttk.Window):
    def __init__(self):
        super().__init__(themename="superhero")
        self.title("ZeroMQ Data Manager & Predictor Stats")
        self.geometry("1200x850")

        # For C++ Data Server
        self.cpp_server_address = "tcp://app-main:5558"
        self.cpp_response_queue = queue.Queue()
        
        # For Python Predictor Statistics
        self.predictor_stats_address = "tcp://live-predictor:5559"
        self.predictor_stats_topic = "prediction_stats"
        self.stats_queue = queue.Queue()

        self.zmq_context = zmq.Context()
        self.stop_event = threading.Event()

        # Updated: Added "SKIPLIST" with ID 7
        self.data_structure_map = {
            "AVL": 1, "LINKED_LIST": 2, "HASHSET": 3,
            "CUCKOO_HASH": 4, "SEGMENT_TREE": 5, "RED_BLACK_TREE": 6,
            "SKIPLIST": 7 
        }
        self.statistic_features_map = {
            "Duration (dur)": 0, "Rate": 1, "Source Load (sload)": 2,
            "Destination Load (dload)": 3, "Source Packets (spkts)": 4,
            "Destination Packets (dpkts)": 5, "Source Bytes (sbytes)": 6,
            "Destination Bytes (dbytes)": 7,
        }
        self.sort_features_map = {
            "ID": "id", "Duration": "dur", "Rate": "rate",
            "Source Bytes": "sbytes", "Dest Bytes": "dbytes"
        }
        self.protocol_filter_map = {"Any": "any", "TCP": 0, "UDP": 1, "ICMP": 4}

        self._create_widgets()
        self.process_cpp_response_queue()
        self.start_stats_listener()
        self.process_stats_queue()
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self._log_message("[SYS] GUI Client initialized.")

    def _create_widgets(self):
        main_container = ttk.Frame(self, padding="10")
        main_container.pack(fill=BOTH, expand=YES)

        left_frame = ttk.Frame(main_container)
        left_frame.pack(side=LEFT, fill=BOTH, expand=True, padx=(0, 10))

        right_frame = ttk.Labelframe(main_container, text="Live Prediction Statistics", padding="10")
        right_frame.pack(side=RIGHT, fill=BOTH, expand=True)

        top_level_operation_frame = ttk.Labelframe(left_frame, text="Select Main Operation", padding="10")
        top_level_operation_frame.pack(fill=X, pady=(0, 10))

        self.selected_top_level_operation = tk.StringVar(value="GET_DATA")
        operations = [
            ("Query Last 3 Data", "GET_DATA"), ("Query Data by ID", "QUERY_DATA_BY_ID"),
            ("Remove Data by ID", "REMOVE_DATA_BY_ID"), ("Perform Statistics", "PERFORM_STATS"),
            ("Filter & Sort Data", "QUERY_FILTERED_SORTED")
        ]
        for i, (text, value) in enumerate(operations):
            rb = ttk.Radiobutton(
                top_level_operation_frame, text=text, variable=self.selected_top_level_operation,
                value=value, command=self._toggle_input_fields, 
                bootstyle="info-round-toggle"
            )
            rb.grid(row=i // 3, column=i % 3, padx=5, pady=2, sticky="w")
        for i in range(3): top_level_operation_frame.grid_columnconfigure(i, weight=1)

        # --- Frames for different operations ---
        
        # Frame for Data Structure selection (General Purpose)
        self.data_structure_selection_frame = ttk.Labelframe(left_frame, text="Select Data Structure", padding="10")
        self.selected_data_structure = tk.StringVar(value="AVL")
        # Updated: Added ("SkipList", "SKIPLIST") to the list of general DS options
        all_ds_options = [
            ("AVL Tree", "AVL"), ("Linked List", "LINKED_LIST"), ("Hashset", "HASHSET"),
            ("Cuckoo Hash", "CUCKOO_HASH"), ("Segment Tree", "SEGMENT_TREE"), ("Red-Black Tree", "RED_BLACK_TREE"),
            ("SkipList", "SKIPLIST") 
        ]
        for i, (text, value) in enumerate(all_ds_options):
            rb = ttk.Radiobutton(
                self.data_structure_selection_frame, text=text, variable=self.selected_data_structure,
                value=value, bootstyle="info-round-toggle")
            rb.grid(row=i // 3, column=i % 3, padx=5, pady=2, sticky="w")
        for i in range(3): self.data_structure_selection_frame.grid_columnconfigure(i, weight=1)

        # Frame for Data Structure selection (Statistics ONLY)
        self.stats_ds_selection_frame = ttk.Labelframe(left_frame, text="Select Data Structure (for Stats)", padding="10")
        self.selected_stats_ds = tk.StringVar(value="LINKED_LIST")
        # Ensure that only data structures supporting statistics are listed here
        stats_ds_options = [("Linked List", "LINKED_LIST"), ("Segment Tree", "SEGMENT_TREE")]
        for i, (text, value) in enumerate(stats_ds_options):
            rb = ttk.Radiobutton(
                self.stats_ds_selection_frame, text=text, variable=self.selected_stats_ds,
                value=value, bootstyle="info-round-toggle"
            )
            rb.pack(side=LEFT, padx=5, pady=2)


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

        # --- Frame for Filtering and Sorting ---
        self.filter_sort_frame = ttk.Labelframe(left_frame, text="Filter & Sort Options", padding="10")
        ttk.Label(self.filter_sort_frame, text="Filter By:", font="-weight bold").grid(row=0, column=0, columnspan=2, sticky="w", pady=(0, 5))
        self.filter_label_var = tk.StringVar(value="any")
        ttk.Label(self.filter_sort_frame, text="Label:").grid(row=1, column=0, sticky="w")
        ttk.Combobox(self.filter_sort_frame, textvariable=self.filter_label_var, values=["any", "attack", "normal"], state="readonly").grid(row=1, column=1, sticky="ew", pady=2)
        
        self.filter_proto_var = tk.StringVar(value="Any")
        ttk.Label(self.filter_sort_frame, text="Protocol:").grid(row=2, column=0, sticky="w")
        ttk.Combobox(self.filter_sort_frame, textvariable=self.filter_proto_var, values=list(self.protocol_filter_map.keys()), state="readonly").grid(row=2, column=1, sticky="ew", pady=2)
        
        ttk.Label(self.filter_sort_frame, text="Sort By:", font="-weight bold").grid(row=3, column=0, columnspan=2, sticky="w", pady=(10, 5))
        self.sort_by_var = tk.StringVar(value="ID")
        ttk.Label(self.filter_sort_frame, text="Field:").grid(row=4, column=0, sticky="w")
        ttk.Combobox(self.filter_sort_frame, textvariable=self.sort_by_var, values=list(self.sort_features_map.keys()), state="readonly").grid(row=4, column=1, sticky="ew", pady=2)

        self.sort_order_var = tk.StringVar(value="asc")
        ttk.Label(self.filter_sort_frame, text="Order:").grid(row=5, column=0, sticky="w")
        ttk.Combobox(self.filter_sort_frame, textvariable=self.sort_order_var, values=["asc", "desc"], state="readonly").grid(row=5, column=1, sticky="ew", pady=2)
        
        self.limit_var = tk.StringVar(value="20")
        ttk.Label(self.filter_sort_frame, text="Limit:").grid(row=6, column=0, sticky="w", pady=(10,0))
        ttk.Entry(self.filter_sort_frame, textvariable=self.limit_var).grid(row=6, column=1, sticky="ew", pady=(10,0))
        self.filter_sort_frame.grid_columnconfigure(1, weight=1)

        # --- Control and Log frames ---
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
        self.log_text = ScrolledText(log_frame, height=15, width=70, wrap=WORD, autohide=True)
        self.log_text.pack(fill=BOTH, expand=YES)
        self.log_text.text.configure(state="disabled")

        # --- Statistics display widgets (in right_frame) ---
        self.stats_vars = {
            "total_processed": tk.StringVar(value="Total: N/A"), "tp": tk.StringVar(value="TP: N/A"),
            "fp": tk.StringVar(value="FP: N/A"), "fn": tk.StringVar(value="FN: N/A"),
            "tn": tk.StringVar(value="TN: N/A"), "accuracy": tk.StringVar(value="Accuracy: N/A"),
            "precision": tk.StringVar(value="Precision: N/A"), "recall": tk.StringVar(value="Recall: N/A"),
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
        # --- Start by hiding all optional frames ---
        self.input_fields_frame.pack_forget()
        self.stats_options_frame.pack_forget()
        self.data_structure_selection_frame.pack_forget()
        self.stats_ds_selection_frame.pack_forget() # Ensure stats DS frame is also handled
        self.filter_sort_frame.pack_forget() 
        
        # --- Selectively show frames based on the main operation ---
        if selected_main_op in ["QUERY_DATA_BY_ID", "REMOVE_DATA_BY_ID"]:
            self.input_fields_frame.pack(fill=X, pady=(0, 5))
            self.data_structure_selection_frame.pack(fill=X, pady=(0, 10))
        
        elif selected_main_op == "PERFORM_STATS":
            self.stats_options_frame.pack(fill=X, pady=(0, 5))
            self.stats_ds_selection_frame.pack(fill=X, pady=(0, 10)) # Show specific stats DS frame
        
        elif selected_main_op == "QUERY_FILTERED_SORTED":
            self.filter_sort_frame.pack(fill=X, pady=(0,10))

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
        request_payload = main_operation 
        
        if main_operation == "QUERY_DATA_BY_ID":
            data_id = self.id_entry.get().strip()
            if not data_id or not data_id.isdigit():
                messagebox.showwarning("Input Error", "ID must be a valid integer.")
                return
            ds_num = self.data_structure_map.get(self.selected_data_structure.get())
            request_payload += f" {data_id} {ds_num}"

        elif main_operation == "REMOVE_DATA_BY_ID":
            data_id = self.id_entry.get().strip()
            if not data_id or not data_id.isdigit():
                messagebox.showwarning("Input Error", "ID must be a valid integer.")
                return
            # Get the DS number to send to the backend
            ds_num = self.data_structure_map.get(self.selected_data_structure.get())
            request_payload += f" {data_id} {ds_num}"

        elif main_operation == "PERFORM_STATS":
            feature_enum = self.statistic_features_map.get(self.selected_feature.get())
            interval = self.interval_entry.get().strip()
            if not interval.isdigit() or int(interval) <= 0:
                messagebox.showwarning("Input Error", "Interval must be a positive integer.")
                return
            # Use selected_stats_ds for statistics (this is already correctly implemented)
            ds_num = self.data_structure_map.get(self.selected_stats_ds.get()) 
            request_payload += f" {feature_enum} {interval} {ds_num}"
        
        elif main_operation == "QUERY_FILTERED_SORTED":
            params = []
            label_filter = self.filter_label_var.get()
            if label_filter == "attack": params.append("label=true")
            elif label_filter == "normal": params.append("label=false")
            
            proto_filter_key = self.filter_proto_var.get()
            proto_filter_val = self.protocol_filter_map.get(proto_filter_key)
            if proto_filter_val != "any": params.append(f"proto={proto_filter_val}")
            
            sort_by_key = self.sort_by_var.get()
            sort_by_val = self.sort_features_map.get(sort_by_key)
            sort_order = self.sort_order_var.get()
            params.append(f"sort_by={sort_by_val}")
            params.append(f"sort_order={sort_order}")
            
            limit_val = self.limit_var.get()
            if limit_val.isdigit() and int(limit_val) > 0:
                params.append(f"limit={limit_val}")

            request_payload += " " + " ".join(params)
        
        elif main_operation == "GET_DATA":
            pass 

        else:
            messagebox.showwarning("Operation Error", f"Unexpected operation selected: {main_operation}")
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

    def start_stats_listener(self):
        self.stats_listener_thread = threading.Thread(target=self._listen_for_stats, daemon=True)
        self.stats_listener_thread.start()
        self._log_message(f"[SYS] Statistics listener started for {self.predictor_stats_address} on topic '{self.predictor_stats_topic}'.")

    def _listen_for_stats(self):
        stats_subscriber = self.zmq_context.socket(zmq.SUB)
        try:
            stats_subscriber.connect(self.predictor_stats_address)
            stats_subscriber.subscribe(self.predictor_stats_topic.encode('utf-8'))
            
            while not self.stop_event.is_set():
                try:
                    if stats_subscriber.poll(timeout=500):
                        topic, message_json = stats_subscriber.recv_multipart()
                        self.stats_queue.put(message_json.decode('utf-8'))
                except zmq.error.ContextTerminated:
                    self._log_message("[SYS] Statistics listener: ZMQ context terminated.", bootstyle="warning")
                    break
                except zmq.ZMQError as e:
                    if e.errno == zmq.ETERM: break
                    self._log_message(f"[ERR] Statistics listener ZMQ Error: {e}", bootstyle="danger")
                    time.sleep(1)
                except Exception as e:
                    self._log_message(f"[ERR] Statistics listener unexpected error: {e}", bootstyle="danger")
                    time.sleep(1)
        finally:
            if stats_subscriber and not stats_subscriber.closed:
                stats_subscriber.close()
            self._log_message("[SYS] Statistics listener thread stopped.")

    def process_stats_queue(self):
        try:
            while not self.stats_queue.empty():
                message_json_str = self.stats_queue.get_nowait()
                try:
                    stats_data = json.loads(message_json_str)
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
            self.after(200, self.process_stats_queue)

    def on_closing(self):
        self._log_message("[SYS] Closing application...")
        self.stop_event.set()
        if hasattr(self, 'stats_listener_thread') and self.stats_listener_thread.is_alive():
            self.stats_listener_thread.join(timeout=1.0) 

        if self.zmq_context and not self.zmq_context.closed:
            self._log_message("[SYS] Terminating ZMQ context...")
            self.zmq_context.term()
        
        self._log_message("[SYS] Destroying window.")
        self.destroy()

if __name__ == "__main__":
    app = ZeroMQGUIClient()
    app.mainloop()


