import queue
import threading
import time
import tkinter as tk

import ttkbootstrap as ttk
import zmq
from ttkbootstrap.constants import *
from ttkbootstrap.scrolled import ScrolledText  # ttkbootstrap's ScrolledText


class ZeroMQGUIClient(ttk.Window):
    def __init__(self):
        super().__init__(themename="superhero")  # You can choose any ttkbootstrap theme
        self.title("ZeroMQ Data Requester")
        self.geometry("700x500")

        self.cpp_server_address = "tcp://app-main:5558"
        self.zmq_context = zmq.Context()

        self.response_queue = queue.Queue()
        self.stop_event = threading.Event()

        main_frame = ttk.Frame(self, padding="10")
        main_frame.pack(fill=BOTH, expand=YES)

        control_frame = ttk.Frame(main_frame)
        control_frame.pack(fill=X, pady=(0, 10))

        self.status_label = ttk.Label(
            control_frame,
            text=f"Target C++ Server: {self.cpp_server_address}",
            bootstyle="info",
        )
        self.status_label.pack(side=LEFT, padx=(0, 10))

        self.request_button = ttk.Button(
            control_frame,
            text="Request Data from C++",
            command=self.send_request_to_cpp,
            bootstyle="primary",
        )
        self.request_button.pack(side=LEFT, padx=5)

        self.clear_log_button = ttk.Button(
            control_frame,
            text="Clear Log",
            command=self.clear_log,
            bootstyle="secondary",
        )
        self.clear_log_button.pack(side=LEFT, padx=5)

        log_frame = ttk.Labelframe(
            main_frame, text="Responses from C++ Server / Logs", padding="10"
        )
        log_frame.pack(fill=BOTH, expand=YES)

        # self.log_text is the ScrolledText *frame*. The actual Text widget is self.log_text.text
        self.log_text = ScrolledText(
            log_frame, height=20, width=80, wrap=WORD, autohide=True
        )
        self.log_text.pack(fill=BOTH, expand=YES)

        # IMPORTANT FIX: Configure the internal Text widget, not the ScrolledText frame
        self.log_text.text.configure(
            state="disabled"
        )  # Make the internal Text widget read-only

        self.process_response_queue()
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self._log_message(
            "[SYS] GUI Client initialized. Click 'Request Data' to query C++ server."
        )

    def _log_message(self, message, prefix="[LOG]"):
        """Appends a message to the ScrolledText widget's internal Text area."""
        # IMPORTANT FIX: Access .text attribute for Text widget operations
        self.log_text.text.configure(state="normal")
        self.log_text.text.insert(END, f"{prefix} {message}\n")
        self.log_text.text.see(END)
        self.log_text.text.configure(state="disabled")

    def clear_log(self):
        """Clears all text from the log display's internal Text area."""
        # IMPORTANT FIX: Access .text attribute
        self.log_text.text.configure(state="normal")
        self.log_text.text.delete("1.0", END)
        self.log_text.text.configure(state="disabled")
        self._log_message("[SYS] Log cleared.")

    def send_request_to_cpp(self):
        self.request_button.config(state=DISABLED)
        self._log_message("[REQ] Sending request 'GET_DATA' to C++ server...")

        request_payload = "GET_DATA"
        request_thread = threading.Thread(
            target=self._execute_zmq_request, args=(request_payload,), daemon=True
        )
        request_thread.start()

    def _execute_zmq_request(self, request_message_payload):
        socket = None
        try:
            socket = self.zmq_context.socket(zmq.REQ)
            socket.setsockopt(zmq.LINGER, 0)
            socket.setsockopt(zmq.RCVTIMEO, 5000)
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
            self.after(0, lambda: self.request_button.config(state=NORMAL))

    def process_response_queue(self):
        try:
            while not self.response_queue.empty():
                message = self.response_queue.get_nowait()
                if "[ERR]" in message:
                    self._log_message(message, prefix="")
                else:
                    self._log_message(message, prefix="[RECV]")
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
