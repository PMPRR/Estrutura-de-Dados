#include <iostream>
#include <string>
#include <vector>
#include <iomanip> // For std::hex
#include <algorithm> // For std::min
#include <zmq.hpp> // C++ bindings
#include <cstring> // Required for strlen, strcmp
#include "data.h"  // Include your Data struct definition

// Helper function to print message content for debugging
void print_message_details(const zmq::message_t& msg, const std::string& context_msg) {
    std::cout << context_msg << " - Size: " << msg.size() << " bytes." << std::endl;
    if (msg.size() > 0) {
        // Print first few bytes as a string (if printable)
        size_t preview_len = std::min<size_t>(msg.size(), 50); // Preview up to 50 chars
        std::string preview_str;
        bool is_printable = true;
        // Correctly cast msg.data() to const char* for iterating printable characters
        const char* msg_char_ptr = static_cast<const char*>(msg.data());
        for (size_t i = 0; i < preview_len; ++i) {
            char c = msg_char_ptr[i];
            if (isprint(c) || c == '\n' || c == '\r' || c == '\t') {
                preview_str += c;
            } else {
                is_printable = false;
                break; // Stop if non-printable found early
            }
        }
        if(is_printable && !preview_str.empty()){
            std::cout << "  Preview (as string): \"" << preview_str << "\"" << std::endl;
        } else {
             std::cout << "  Preview (first " << preview_len << " bytes, as hex): ";
             // Correctly cast msg.data() to const unsigned char* for hex printing
             const unsigned char* msg_byte_ptr = static_cast<const unsigned char*>(msg.data());
            for (size_t i = 0; i < preview_len; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg_byte_ptr[i]) << " ";
            }
            std::cout << std::dec << std::endl; // Reset to decimal
        }
    }
}


// The main function
int main() {
    zmq::context_t context(1);
    zmq::socket_t subscriber_socket(context, ZMQ_SUB); // Use C-style socket type macro

    // In your docker-compose.yml, the publisher service is named 'python_publisher'.
    const std::string publisher_address = "tcp://python_publisher:5556";
    try {
        subscriber_socket.connect(publisher_address);
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to connect to publisher at " << publisher_address << ": " << e.what() << std::endl;
        return 1;
    }
    std::cout << "C++ Subscriber connected to " << publisher_address << std::endl;
    

    const char* specific_subscription_topic = "data_batch";
    // Use the C-style setsockopt for broader compatibility
    // The third argument is a pointer to the option value, and the fourth is its size.
    try {
        // MODIFICATION: Subscribe to ALL messages by using an empty topic string
        const char* catch_all_topic = "";
        subscriber_socket.setsockopt(ZMQ_SUBSCRIBE, catch_all_topic, 0); // Empty topic, length 0
        std::cout << "Subscribed to ALL topics (empty filter)." << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Failed to set socket option ZMQ_SUBSCRIBE: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Waiting for messages..." << std::endl;


    try {
        while (true) {
            zmq::message_t message;
            bool recv_ok = false;
            try {
                // Attempt to receive. This might throw if the context is terminated.
                recv_ok = subscriber_socket.recv(&message, 0); // 0 for blocking flags
            } catch (const zmq::error_t& e) {
                if (e.num() == ETERM) { // ETERM means context was terminated
                    std::cerr << "ZeroMQ context terminated, shutting down subscriber." << std::endl;
                    break; // Exit the loop
                }
                std::cerr << "Error during recv: " << e.what() << std::endl;
                 if (!recv_ok && e.num() != EAGAIN) { 
                    break;
                 }
            }


            if (recv_ok && message.size() > 0) {
                // DEBUG: Log that a message was received and its raw details
                print_message_details(message, "[DEBUG] Raw message received");

                // Successfully received a message
                size_t topic_len = strlen(specific_subscription_topic);
                const char* msg_data_ptr = static_cast<const char*>(message.data()); // Keep const

                // Ensure message is large enough to contain topic + space + potentially data
                if (message.size() > topic_len && msg_data_ptr[topic_len] == ' ') { // Check for space after potential topic
                    // Compare the beginning of the message with the specific subscription topic
                    if (strncmp(msg_data_ptr, specific_subscription_topic, topic_len) == 0) {
                        std::cout << "Message matches expected topic: '" << specific_subscription_topic << "'" << std::endl;
                        const char* packed_data_start_ptr = msg_data_ptr + topic_len + 1; // +1 for the space
                        size_t packed_data_total_size = message.size() - (topic_len + 1);

                        // Check if sizeof(Data) is zero, which would indicate an incomplete definition for Data
                        if (sizeof(Data) == 0) {
                            std::cerr << "Error: sizeof(Data) is 0. Please ensure 'data.h' is correctly included and 'Data' struct is fully defined." << std::endl;
                        } else if (packed_data_total_size > 0 && packed_data_total_size % sizeof(Data) == 0) {
                            size_t num_structs = packed_data_total_size / sizeof(Data);
                            std::cout << "Received batch of " << num_structs << " Data structs." << std::endl;
                            for (size_t i = 0; i < num_structs; ++i) {
                                // Cast to const Data* as we are only reading
                                const Data* received_data = reinterpret_cast<const Data*>(packed_data_start_ptr + i * sizeof(Data));
                                std::cout << "  Data ID: " << received_data->id
                                          << ", Dur: " << received_data->dur
                                          << ", Rate: " << received_data->rate
                                          // Add other fields as needed for debugging
                                          << std::endl;
                            }
                        } else if (packed_data_total_size == 0) {
                             std::cout << "Received topic '" << specific_subscription_topic << "' with empty data payload." << std::endl;
                        } else {
                            std::cerr << "Error: For topic '" << specific_subscription_topic << "', received total packed data size (" << packed_data_total_size
                                      << ") is not a multiple of expected Data struct size (" << sizeof(Data) << ")" << std::endl;
                        }
                    } else {
                        // Message was long enough for topic + space, but topic string didn't match
                        std::string received_prefix(msg_data_ptr, std::min<size_t>(message.size(), topic_len + 5));
                        std::cerr << "Warning: Received message does not start with expected topic '" << specific_subscription_topic
                                  << "'. Actual prefix: '" << received_prefix << "'" << std::endl;
                    }
                } else {
                    // Message is too short to be "topic + space + data" or doesn't have a space after potential topic length
                    std::cerr << "Warning: Received message does not match expected format (topic + space + data)." << std::endl;
                    // The raw message details were already printed by print_message_details.
                }
            } else if (recv_ok && message.size() == 0) {
                 std::cout << "[DEBUG] Received empty message part (size 0)." << std::endl;
            } else if (!recv_ok) {
                // This case should ideally be handled by the ETERM check or other specific zmq::error_t in the catch block
                // std::cerr << "Failed to receive message (recv returned false or threw non-ETERM error)." << std::endl;
            }
        }
    } catch (const zmq::error_t& e) {
        std::cerr << "Unhandled ZeroMQ error in main loop: " << e.what() << " (errno: " << e.num() << ")" << std::endl;
        if (e.num() == ETERM) {
            std::cerr << "Context was terminated (caught in outer try-catch)." << std::endl;
        }
        return 1; // Indicate error
    } catch (const std::exception& e) {
        std::cerr << "Standard exception: " << e.what() << std::endl;
        return 1; // Indicate error
    }

    std::cout << "Subscriber shutting down normally..." << std::endl;
    return 0; // Indicate success
}


