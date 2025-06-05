import queue
import threading
import time
import tkinter as tk
from tkinter import messagebox # Import messagebox for confirmations

import ttkbootstrap as ttk
import zmq
from ttkbootstrap.constants import *
from ttkbootstrap.scrolled import ScrolledText  # ttkbootstrap's ScrolledText


class ZeroMQGUIClient(ttk.Window):
    def __init__(self):
        super().__init__(themename="superhero")  # You can choose any ttkbootstrap theme
        self.title("ZeroMQ Data Manager")
        self.geometry("1000x700") # Increased size for more options and frames

        self.cpp_server_address = "tcp://app-main:5558"
        self.zmq_context = zmq.Context()

        self.response_queue = queue.Queue()
        self.stop_event = threading.Event()

        # Mapping for data structure names to numeric IDs (1-6)
        self.data_structure_map = {
            "AVL": 1,
            "LINKED_LIST": 2,
            "HASHSET": 3,
            "CUCKOO_HASH": 4,
            "SEGMENT_TREE": 5,
            "RED_BLACK_TREE": 6 # Changed from SPLAY_TREE to RED_BLACK_TREE
        }
        # Reverse map for display purposes if needed, though not directly used here
        self.reverse_ds_map = {v: k for k, v in self.data_structure_map.items()}


        self._create_widgets()
        self.process_response_queue()
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self._log_message(
            "[SYS] GUI Client initialized. Select an operation and click 'Execute'."
        )

    def _create_widgets(self):
        main_frame = ttk.Frame(self, padding="10")
        main_frame.pack(fill=BOTH, expand=YES)

        # --- Top-Level Operation Selection Frame (4 options remaining) ---
        top_level_operation_frame = ttk.Labelframe(main_frame, text="Select Main Operation", padding="10")
        top_level_operation_frame.pack(fill=X, pady=(0, 10))

        self.selected_top_level_operation = tk.StringVar(value="GET_DATA") # Default operation
        
        # Row 0
        radio_get_data = ttk.Radiobutton(
            top_level_operation_frame,
            text="Query Last 3 Data Items",
            variable=self.selected_top_level_operation,
            value="GET_DATA",
            command=self._toggle_input_fields,
            bootstyle="info-round-toggle"
        )
        radio_get_data.grid(row=0, column=0, padx=5, pady=2, sticky="w")

        radio_query_id = ttk.Radiobutton(
            top_level_operation_frame,
            text="Query Data by ID",
            variable=self.selected_top_level_operation,
            value="QUERY_DATA_BY_ID",
            command=self._toggle_input_fields,
            bootstyle="info-round-toggle"
        )
        radio_query_id.grid(row=0, column=1, padx=5, pady=2, sticky="w")

        radio_remove = ttk.Radiobutton(
            top_level_operation_frame,
            text="Remove Data by ID",
            variable=self.selected_top_level_operation,
            value="REMOVE_DATA_BY_ID",
            command=self._toggle_input_fields,
            bootstyle="danger-round-toggle"
        )
        radio_remove.grid(row=0, column=2, padx=5, pady=2, sticky="w")

        # Row 1 (only one option here now)
        radio_stats = ttk.Radiobutton(
            top_level_operation_frame,
            text="Perform Statistics",
            variable=self.selected_top_level_operation,
            value="PERFORM_STATS",
            command=self._toggle_input_fields,
            bootstyle="success-round-toggle"
        )
        radio_stats.grid(row=1, column=0, padx=5, pady=2, sticky="w")
        
        # Removed radio_export_data
        # Removed radio_sys_info

        # Configure columns to expand
        for i in range(3):
            top_level_operation_frame.grid_columnconfigure(i, weight=1)


        # --- Secondary Data Structure Selection Frame (All 6 options initially defined) ---
        self.data_structure_selection_frame = ttk.Labelframe(main_frame, text="Select Data Structure", padding="10")
        # Initially not packed, will be packed/unpacked dynamically

        self.selected_data_structure = tk.StringVar(value="AVL") # Default data structure for general use

        self.ds_radio_buttons = {} # Store references to radio buttons for easy toggling
        all_ds_options = [
            ("AVL Tree", "AVL"),
            ("Linked List", "LINKED_LIST"),
            ("Hashset", "HASHSET"),
            ("Cuckoo Hash", "CUCKOO_HASH"),
            ("Segment Tree", "SEGMENT_TREE"),
            ("Red-Black Tree", "RED_BLACK_TREE") # Updated label and value
        ]

        for i, (text, value) in enumerate(all_ds_options):
            col = i % 3 # 3 columns per row
            row = i // 3 # 2 rows
            rb = ttk.Radiobutton(
                self.data_structure_selection_frame,
                text=text,
                variable=self.selected_data_structure,
                value=value,
                bootstyle="info-round-toggle"
            )
            rb.grid(row=row, column=col, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons[value] = rb # Store reference
        
        for i in range(3): # Configure columns to expand
            self.data_structure_selection_frame.grid_columnconfigure(i, weight=1)

        # --- Input Fields Frame (for ID) ---
        self.input_fields_frame = ttk.Frame(main_frame)
        self.input_fields_frame.pack(fill=X, pady=(0, 10)) # Packed by default, but content changes

        self.id_label = ttk.Label(self.input_fields_frame, text="Data ID:", bootstyle="secondary")
        self.id_entry = ttk.Entry(self.input_fields_frame, bootstyle="info", width=20)
        self.id_entry.grid(row=0, column=1, padx=(0, 10), pady=2, sticky="ew")
        self.input_fields_frame.grid_columnconfigure(1, weight=1) # Make entry expand

        # --- Statistics Sub-options Frame (Initially Hidden) ---
        self.stats_sub_options_frame = ttk.Labelframe(main_frame, text="Statistics Type", padding="10")
        # Don't pack it yet, will be packed/unpacked dynamically

        self.selected_stats_sub_type = tk.StringVar(value="STATS_SUMMARY_SLOAD") # Default stats sub-type

        self.stats_summary_sload_rb = ttk.Radiobutton(
            self.stats_sub_options_frame,
            text="Summary Statistics (SLoad & DLoad) by Interval",
            variable=self.selected_stats_sub_type,
            value="STATS_SUMMARY_SLOAD",
            command=self._toggle_input_fields, # Re-evaluate inputs on sub-type change
            bootstyle="success-round-toggle"
        )
        self.stats_summary_sload_rb.grid(row=0, column=0, padx=5, pady=2, sticky="w")

        self.stats_category_breakdown_rb = ttk.Radiobutton(
            self.stats_sub_options_frame,
            text="Attack Category Breakdown",
            variable=self.selected_stats_sub_type,
            value="STATS_CATEGORY_BREAKDOWN",
            command=self._toggle_input_fields, # Re-evaluate inputs on sub-type change
            bootstyle="success-round-toggle"
        )
        self.stats_category_breakdown_rb.grid(row=0, column=1, padx=5, pady=2, sticky="w")

        self.stats_sub_options_frame.grid_columnconfigure(0, weight=1)
        self.stats_sub_options_frame.grid_columnconfigure(1, weight=1)

        # Interval input for statistics
        self.interval_label = ttk.Label(self.stats_sub_options_frame, text="Interval (last N items):", bootstyle="secondary")
        self.interval_entry = ttk.Entry(self.stats_sub_options_frame, bootstyle="info", width=15)
        self.interval_entry.insert(0, "100") # Default interval
        self.stats_sub_options_frame.grid_columnconfigure(1, weight=1) # Make entry expand


        # --- Control Buttons Frame ---
        control_frame = ttk.Frame(main_frame)
        control_frame.pack(fill=X, pady=(0, 10))

        self.status_label = ttk.Label(
            control_frame,
            text=f"Target C++ Server: {self.cpp_server_address}",
            bootstyle="info",
        )
        self.status_label.pack(side=LEFT, padx=(0, 10))

        self.execute_button = ttk.Button(
            control_frame,
            text="Execute Operation",
            command=self.execute_selected_operation,
            bootstyle="primary",
        )
        self.execute_button.pack(side=LEFT, padx=5)

        self.clear_log_button = ttk.Button(
            control_frame,
            text="Clear Log",
            command=self.clear_log,
            bootstyle="secondary",
        )
        self.clear_log_button.pack(side=LEFT, padx=5)

        # --- Log Display Frame ---
        log_frame = ttk.Labelframe(
            main_frame, text="Responses from C++ Server / Logs", padding="10"
        )
        log_frame.pack(fill=BOTH, expand=YES)

        self.log_text = ScrolledText(
            log_frame, height=20, width=80, wrap=WORD, autohide=True
        )
        self.log_text.pack(fill=BOTH, expand=YES)
        self.log_text.text.configure(state="disabled")

        self._toggle_input_fields() # Set initial state

    def _toggle_input_fields(self):
        """Hides/shows input fields and sub-frames based on the selected operation."""
        selected_main_op = self.selected_top_level_operation.get()
        selected_stats_sub_type = self.selected_stats_sub_type.get()

        # Hide all specific input fields and sub-frames first
        self.id_label.grid_remove()
        self.id_entry.grid_remove()
        self.interval_label.grid_remove()
        self.interval_entry.grid_remove()
        self.stats_sub_options_frame.pack_forget() # Hide the entire stats sub-frame
        self.data_structure_selection_frame.pack_forget() # Hide the entire DS sub-frame

        # Hide all DS radio buttons first (important for selective display)
        for rb in self.ds_radio_buttons.values():
            rb.grid_remove()


        if selected_main_op == "QUERY_DATA_BY_ID" or selected_main_op == "REMOVE_DATA_BY_ID":
            self.id_label.grid(row=0, column=0, padx=(0, 5), pady=2, sticky="w")
            self.id_entry.grid(row=0, column=1, padx=(0, 10), pady=2, sticky="ew")
            self.input_fields_frame.pack(fill=X, pady=(0, 10)) # Ensure frame is packed
            self.data_structure_selection_frame.pack(fill=X, pady=(0, 10)) # Show DS sub-frame

            # Show all 6 data structure options for Query/Remove
            self.ds_radio_buttons["AVL"].grid(row=0, column=0, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons["LINKED_LIST"].grid(row=0, column=1, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons["HASHSET"].grid(row=0, column=2, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons["CUCKOO_HASH"].grid(row=1, column=0, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons["SEGMENT_TREE"].grid(row=1, column=1, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons["RED_BLACK_TREE"].grid(row=1, column=2, padx=5, pady=2, sticky="w") # Updated here
            
            # Ensure selected_data_structure has a default that will be visible
            if self.selected_data_structure.get() not in self.data_structure_map: # If a previously selected value is now hidden
                self.selected_data_structure.set("AVL") # Set default to AVL if current is not in all 6


        elif selected_main_op == "PERFORM_STATS":
            self.stats_sub_options_frame.pack(fill=X, pady=(0, 10)) # Show stats sub-frame
            self.data_structure_selection_frame.pack(fill=X, pady=(0, 10)) # Show DS sub-frame for stats

            # Show only Segment Tree and Linked List for Stats
            self.ds_radio_buttons["SEGMENT_TREE"].grid(row=0, column=0, padx=5, pady=2, sticky="w")
            self.ds_radio_buttons["LINKED_LIST"].grid(row=0, column=1, padx=5, pady=2, sticky="w")
            # Ensure other DS buttons are hidden (already done by initial grid_remove)
            
            # Ensure selected_data_structure has a default that will be visible
            if self.selected_data_structure.get() not in ["SEGMENT_TREE", "LINKED_LIST"]:
                self.selected_data_structure.set("SEGMENT_TREE") # Set default to Segment Tree if current is not one of these


            # Now check which stats sub-type is selected to show its specific inputs
            if selected_stats_sub_type == "STATS_SUMMARY_SLOAD":
                self.interval_label.grid(row=1, column=0, padx=(0, 5), pady=2, sticky="w")
                self.interval_entry.grid(row=1, column=1, padx=(0, 10), pady=2, sticky="ew")
            # If STATS_CATEGORY_BREAKDOWN is selected, no additional inputs beyond DS needed

        # For other operations (GET_DATA), no special inputs or sub-frames needed
        # so simply pack_forget() handles it if not explicitly packed by previous conditions.


    def _log_message(self, message, prefix="[LOG]", bootstyle="default"):
        """Appends a message to the ScrolledText widget's internal Text area."""
        self.log_text.text.configure(state="normal")
        self.log_text.text.insert(END, f"{prefix} {message}\n", bootstyle)
        self.log_text.text.see(END)
        self.log_text.text.configure(state="disabled")

    def clear_log(self):
        """Clears all text from the log display's internal Text area."""
        self.log_text.text.configure(state="normal")
        self.log_text.text.delete("1.0", END)
        self.log_text.text.configure(state="disabled")
        self._log_message("[SYS] Log cleared.")

    def execute_selected_operation(self):
        main_operation = self.selected_top_level_operation.get()
        selected_ds_str = self.selected_data_structure.get() # Get string name
        selected_ds_num = self.data_structure_map.get(selected_ds_str) # Convert to number
        
        request_payload = "" # Will be constructed

        if main_operation == "QUERY_DATA_BY_ID":
            data_id = self.id_entry.get().strip()
            if not data_id:
                messagebox.showwarning("Input Error", "Please enter an ID for Query.")
                self._log_message("[WARN] ID required for " + main_operation + " operation.", prefix="", bootstyle="warning")
                return
            try:
                int(data_id) # Validate if it's a valid integer ID
            except ValueError:
                messagebox.showwarning("Input Error", "ID must be a valid integer.")
                self._log_message("[WARN] Invalid ID format for " + main_operation + ". Must be integer.", prefix="", bootstyle="warning")
                return
            # Include selected data structure NUMBER in the request
            request_payload = f"{main_operation} {data_id} {selected_ds_num}"
            self._log_message(f"[REQ] Sending request '{request_payload}' to C++ server...", prefix="", bootstyle="info")
        
        elif main_operation == "REMOVE_DATA_BY_ID":
            data_id = self.id_entry.get().strip()
            if not data_id:
                messagebox.showwarning("Input Error", "Please enter an ID for Removal.")
                self._log_message("[WARN] ID required for " + main_operation + " operation.", prefix="", bootstyle="warning")
                return
            try:
                int(data_id) # Validate if it's a valid integer ID
            except ValueError:
                messagebox.showwarning("Input Error", "ID must be a valid integer.")
                self._log_message("[WARN] Invalid ID format for " + main_operation + ". Must be integer.", prefix="", bootstyle="warning")
                return
            # Include selected data structure NUMBER in the request
            request_payload = f"{main_operation} {data_id} {selected_ds_num}"
            self._log_message(f"[REQ] Sending request '{request_payload}' to C++ server...", prefix="", bootstyle="info")

        elif main_operation == "PERFORM_STATS":
            stats_sub_type = self.selected_stats_sub_type.get()
            
            if stats_sub_type == "STATS_SUMMARY_SLOAD":
                interval = self.interval_entry.get().strip()
                if not interval:
                    messagebox.showwarning("Input Error", "Please enter an interval for Summary Statistics.")
                    self._log_message("[WARN] Interval required for Summary Statistics.", prefix="", bootstyle="warning")
                    return
                try:
                    interval_int = int(interval)
                    if interval_int <= 0:
                        messagebox.showwarning("Input Error", "Interval must be a positive integer.")
                        self._log_message("[WARN] Interval must be a positive integer.", prefix="", bootstyle="warning")
                        return
                except ValueError:
                    messagebox.showwarning("Input Error", "Interval must be a valid integer.")
                    self._log_message("[WARN] Invalid interval format. Must be integer.", prefix="", bootstyle="warning")
                    return
                # Include selected data structure NUMBER in the request (Segment Tree or Linked List)
                request_payload = f"GET_STATS_SLOAD {interval} {selected_ds_num}"
                self._log_message(f"[REQ] Sending request '{request_payload}' to C++ server...", prefix="", bootstyle="info")
            
            elif stats_sub_type == "STATS_CATEGORY_BREAKDOWN":
                # Include selected data structure NUMBER in the request (Segment Tree or Linked List)
                request_payload = f"GET_STATS_CATEGORY_BREAKDOWN {selected_ds_num}"
                self._log_message(f"[REQ] Sending request '{request_payload}' to C++ server...", prefix="", bootstyle="info")
            else:
                messagebox.showwarning("Operation Error", "Please select a specific statistics type.")
                self._log_message("[WARN] No specific statistics type selected.", prefix="", bootstyle="warning")
                return
            
        elif main_operation == "GET_DATA": # Corrected value here from "GET_DATA_LAST_3"
            request_payload = "GET_DATA"
            self._log_message(f"[REQ] Sending request '{request_payload}' to C++ server...", prefix="", bootstyle="info")
        
        else: # Catch any other unexpected main operations, though our current radio buttons cover all cases.
            messagebox.showwarning("Operation Error", "An unexpected operation was selected.")
            self._log_message("[ERR] Unexpected main operation selected.", prefix="", bootstyle="danger")
            return


        self.execute_button.config(state=DISABLED)
        request_thread = threading.Thread(
            target=self._execute_zmq_request, args=(request_payload,), daemon=True
        )
        request_thread.start()

    def _execute_zmq_request(self, request_message_payload):
        socket = None
        try:
            socket = self.zmq_context.socket(zmq.REQ)
            socket.setsockopt(zmq.LINGER, 0)
            socket.setsockopt(zmq.RCVTIMEO, 10000) # Increased timeout for potentially longer C++ operations
            socket.setsockopt(zmq.SNDTIMEO, 5000)

            socket.connect(self.cpp_server_address)

            socket.send_string(request_message_payload)
            response = socket.recv_string()
            self.response_queue.put(response)

        except zmq.error.Again:
            error_msg = f"[ERR] Timeout: No response from C++ server at {self.cpp_server_address}."
            self.response_queue.put(error_msg)
        except zmq.ZMQError as e:
            error_msg = f"[ERR] ZeroMQ Error during request: {e}"
            self.response_queue.put(error_msg)
        except Exception as e:
            error_msg = f"[ERR] Unexpected error during request: {e}"
            self.response_queue.put(error_msg)
        finally:
            if socket:
                socket.close()
            self.after(0, lambda: self.execute_button.config(state=NORMAL))

    def process_response_queue(self):
        try:
            while not self.response_queue.empty():
                message = self.response_queue.get_nowait()
                if "[ERR]" in message:
                    self._log_message(message, prefix="", bootstyle="danger")
                # Specific messages that are informational/warning but not hard errors
                elif ("NOT IMPLEMENTED" in message or "No data collected" in message or 
                      "Unknown command" in message or "No data with ID" in message or
                      "Error parsing ID" in message or "Interval must be" in message or
                      "Please enter an ID" in message or "Please select a specific statistics type" in message):
                    self._log_message(message, prefix="[INFO]", bootstyle="warning")
                else: # General successful response
                    self._log_message(message, prefix="[RECV]", bootstyle="success")
                self.response_queue.task_done()
        except queue.Empty:
            pass

        if not self.stop_event.is_set():
            self.after(100, self.process_response_queue)

    def on_closing(self):
        self._log_message("[SYS] Closing application...")
        self.stop_event.set()

        if self.zmq_context and not self.zmq_context.closed:
            self._log_message("[SYS] Terminating ZMQ context...")
            self.zmq_context.term()

        self._log_message("[SYS] Destroying window.")
        self.destroy()


if __name__ == "__main__":
    app = ZeroMQGUIClient()
    app.mainloop()


