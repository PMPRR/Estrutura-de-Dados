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
        self.geometry("800x600")

        self.cpp_server_address = "tcp://app-main:5558"
        self.zmq_context = zmq.Context()

        self.response_queue = queue.Queue()
        self.stop_event = threading.Event()

        self._create_widgets()
        self.process_response_queue()
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self._log_message(
            "[SYS] GUI Client initialized. Select an operation and click 'Execute'."
        )

    def _create_widgets(self):
        main_frame = ttk.Frame(self, padding="10")
        main_frame.pack(fill=BOTH, expand=YES)

        # --- Operation Selection Frame ---
        operation_frame = ttk.Labelframe(main_frame, text="Select Operation", padding="10")
        operation_frame.pack(fill=X, pady=(0, 10))

        self.selected_operation = tk.StringVar(value="GET_DATA") # Default operation
        
        radio_get_data = ttk.Radiobutton(
            operation_frame,
            text="Query Last 3 Data Items",
            variable=self.selected_operation,
            value="GET_DATA",
            command=self._toggle_input_fields,
            bootstyle="info-round-toggle"
        )
        radio_get_data.grid(row=0, column=0, padx=5, pady=2, sticky="w")

        radio_query_id = ttk.Radiobutton(
            operation_frame,
            text="Query Data by ID",
            variable=self.selected_operation,
            value="QUERY_ID",
            command=self._toggle_input_fields,
            bootstyle="info-round-toggle"
        )
        radio_query_id.grid(row=0, column=1, padx=5, pady=2, sticky="w")


        radio_remove = ttk.Radiobutton(
            operation_frame,
            text="Remove Data by ID",
            variable=self.selected_operation,
            value="REMOVE_DATA",
            command=self._toggle_input_fields,
            bootstyle="danger-round-toggle"
        )
        radio_remove.grid(row=0, column=2, padx=5, pady=2, sticky="w")

        radio_stats = ttk.Radiobutton(
            operation_frame,
            text="Get Statistics (with Interval)",
            variable=self.selected_operation,
            value="GET_STATS",
            command=self._toggle_input_fields,
            bootstyle="success-round-toggle"
        )
        radio_stats.grid(row=0, column=3, padx=5, pady=2, sticky="w")


        # --- Input Fields Frame (for ID and Interval) ---
        input_frame = ttk.Frame(main_frame)
        input_frame.pack(fill=X, pady=(0, 10))

        self.id_label = ttk.Label(input_frame, text="Data ID:", bootstyle="secondary")
        self.id_label.grid(row=0, column=0, padx=(0, 5), pady=2, sticky="w")

        self.id_entry = ttk.Entry(input_frame, bootstyle="info", width=20)
        self.id_entry.grid(row=0, column=1, padx=(0, 10), pady=2, sticky="ew")

        self.interval_label = ttk.Label(input_frame, text="Interval (last N items):", bootstyle="secondary")
        self.interval_label.grid(row=1, column=0, padx=(0, 5), pady=2, sticky="w")

        self.interval_entry = ttk.Entry(input_frame, bootstyle="info", width=20)
        self.interval_entry.grid(row=1, column=1, padx=(0, 10), pady=2, sticky="ew")
        self.interval_entry.insert(0, "100") # Default interval

        # Configure column weights for input_frame
        input_frame.grid_columnconfigure(1, weight=1)

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
        """Hides/shows input fields based on the selected operation."""
        selected = self.selected_operation.get()
        if selected == "REMOVE_DATA" or selected == "QUERY_ID":
            self.id_label.grid(row=0, column=0)
            self.id_entry.grid(row=0, column=1)
            self.interval_label.grid_remove()
            self.interval_entry.grid_remove()
        elif selected == "GET_STATS":
            self.id_label.grid_remove()
            self.id_entry.grid_remove()
            self.interval_label.grid(row=1, column=0)
            self.interval_entry.grid(row=1, column=1)
        else: # GET_DATA
            self.id_label.grid_remove()
            self.id_entry.grid_remove()
            self.interval_label.grid_remove()
            self.interval_entry.grid_remove()

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
        operation = self.selected_operation.get()
        request_payload = operation

        if operation == "REMOVE_DATA" or operation == "QUERY_ID":
            data_id = self.id_entry.get().strip()
            if not data_id:
                messagebox.showwarning("Input Error", "Please enter an ID.")
                self._log_message("[WARN] ID required for " + operation + " operation.", prefix="", bootstyle="warning")
                return
            try:
                int(data_id) # Validate if it's a valid integer ID
            except ValueError:
                messagebox.showwarning("Input Error", "ID must be a valid integer.")
                self._log_message("[WARN] Invalid ID format for " + operation + ". Must be integer.", prefix="", bootstyle="warning")
                return
            request_payload = f"{operation} {data_id}"
            self._log_message(f"[REQ] Sending request '{request_payload}' to C++ server...", prefix="", bootstyle="info")
        
        elif operation == "GET_STATS":
            interval = self.interval_entry.get().strip()
            if not interval:
                messagebox.showwarning("Input Error", "Please enter an interval for statistics.")
                self._log_message("[WARN] Interval required for GET_STATS operation.", prefix="", bootstyle="warning")
                return
            try:
                interval_int = int(interval)
                if interval_int <= 0:
                    messagebox.showwarning("Input Error", "Interval must be a positive integer.")
                    self._log_message("[WARN] Interval must be a positive integer.", prefix="", bootstyle="warning")
                    return
            except ValueError:
                messagebox.showwarning("Input Error", "Interval must be a valid integer.")
                self._log_message("[WARN] Invalid interval format for GET_STATS. Must be integer.", prefix="", bootstyle="warning")
                return
            request_payload = f"GET_STATS {interval}"
            self._log_message(f"[REQ] Sending request '{request_payload}' to C++ server...", prefix="", bootstyle="info")
            
        elif operation == "GET_DATA":
            self._log_message(f"[REQ] Sending request 'GET_DATA' to C++ server...", prefix="", bootstyle="info")
        
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
                elif "Successfully removed" in message or "No data collected" in message or "Unknown command" in message or "No data with ID" in message or "NOT IMPLEMENTED" in message:
                    self._log_message(message, prefix="[INFO]", bootstyle="warning")
                else:
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


