import zmq
import struct
import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
from tensorflow import keras
import time
import sys
import os
import json # For serializing statistics

# Ensure Python prints output immediately
sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

print("[DEBUG] prediction.py: Script started.")

try:
    from categorical import Protocolo, Servico, State, Attack_cat
    print("[DEBUG] prediction.py: Successfully imported enums from categorical.py.")
except ImportError:
    print("Error: Could not import enums from categorical.py.")
    print("Ensure categorical.py is in the same directory as this script.")
    sys.exit(1)

# --- Configuration ---
ZMQ_DATA_PUBLISHER_ADDRESS = "tcp://python_publisher:5556" # Address of the data generator
ZMQ_DATA_TOPIC = "data_batch"
MODEL_FILEPATH = "model20.keras"
TRAINING_DATA_CSV_PATH = "UNSW_NB15_training-set.csv"

# --- Statistics Publisher Configuration (NEW) ---
ZMQ_STATS_PUBLISHER_ADDRESS = "tcp://*:5559"  # Predictor will publish stats on this address
ZMQ_STATS_TOPIC = "prediction_stats"
print(f"[DEBUG] prediction.py: Statistics will be published on {ZMQ_STATS_PUBLISHER_ADDRESS} with topic '{ZMQ_STATS_TOPIC}'")


print(f"[DEBUG] prediction.py: ZMQ_DATA_PUBLISHER_ADDRESS set to {ZMQ_DATA_PUBLISHER_ADDRESS}")
print(f"[DEBUG] prediction.py: MODEL_FILEPATH set to {MODEL_FILEPATH}")
print(f"[DEBUG] prediction.py: TRAINING_DATA_CSV_PATH set to {TRAINING_DATA_CSV_PATH}")

DATA_STRUCT_FORMAT = (
    "<" + "I" + "f" * 11 + "H" * 2 + "I" * 2 + "B" * 2 + "H" * 2 + "H" + "I" * 2 + "H" +
    "H" * 2 + "H" + "I" + "H" * 5 + "H" * 4 + "?" * 3 + "B" * 4
)
DATA_STRUCT_UNPACKER = struct.Struct(DATA_STRUCT_FORMAT)
print(f"[DEBUG] prediction.py: DATA_STRUCT_UNPACKER initialized. Expected size: {DATA_STRUCT_UNPACKER.size}")

expected_df_columns_in_order = [
    'id', 'dur', 'rate', 'sload', 'dload', 'sinpkt', 'dinpkt', 'sjit', 'djit',
    'tcprtt', 'synack', 'ackdat', 'spkts', 'dpkts', 'sbytes', 'dbytes',
    'sttl', 'dttl', 'sloss', 'dloss', 'swin', 'stcpb', 'dtcpb', 'dwin',
    'smean', 'dmean', 'trans_depth', 'response_body_len', 'ct_srv_src',
    'ct_dst_ltm', 'ct_src_dport_ltm', 'ct_dst_sport_ltm',
    'ct_dst_src_ltm', 'ct_ftp_cmd', 'ct_flw_http_mthd', 'ct_src_ltm',
    'ct_srv_dst', 'is_ftp_login', 'is_sm_ips_ports', 'label',
    'proto', 'state', 'attack_cat', 'service'
]
CAT_COLS = ["proto", "service", "state"]
COLS_TO_DROP_FOR_MODEL_INPUT = ["id", "label", "attack_cat"]

def unpack_data_to_df(binary_payload):
    all_records = []
    offset = 0
    while offset < len(binary_payload):
        try:
            if offset + DATA_STRUCT_UNPACKER.size > len(binary_payload):
                break
            record_tuple = DATA_STRUCT_UNPACKER.unpack_from(binary_payload, offset)
            all_records.append(record_tuple)
            offset += DATA_STRUCT_UNPACKER.size
        except struct.error as e:
            print(f"[ERROR] unpack_data_to_df: Struct unpacking error at offset {offset}: {e}. Payload size: {len(binary_payload)}")
            break
    if not all_records:
        return pd.DataFrame()
    df = pd.DataFrame(all_records, columns=expected_df_columns_in_order)
    return df

def map_enums_to_strings(df):
    df_mapped = df.copy()
    try:
        df_mapped['proto'] = df_mapped['proto'].apply(lambda x: Protocolo(int(x)).name if pd.notnull(x) else None)
        df_mapped['service'] = df_mapped['service'].apply(lambda x: Servico(int(x)).name if pd.notnull(x) else None)
        df_mapped['state'] = df_mapped['state'].apply(lambda x: State(int(x)).name if pd.notnull(x) else None)
    except ValueError as e:
        print(f"[WARNING] map_enums_to_strings: Error mapping enum value: {e}.")
    return df_mapped

def publish_statistics(stats_publisher_socket, stats_data):
    """Publishes statistics as a JSON string."""
    try:
        topic = ZMQ_STATS_TOPIC.encode('utf-8')
        message = json.dumps(stats_data).encode('utf-8')
        stats_publisher_socket.send_multipart([topic, message])
        # print(f"[DEBUG] Sent statistics: {stats_data}") # Can be too verbose
    except Exception as e:
        print(f"[ERROR] Failed to publish statistics: {e}")


def run_predictor():
    print("[DEBUG] run_predictor: Function started.")
    tp_count = 0
    fp_count = 0
    fn_count = 0
    tn_count = 0
    total_processed_count = 0

    # --- Keras Model Loading ---
    if not os.path.exists(MODEL_FILEPATH):
        print(f"[CRITICAL] run_predictor: Model file '{MODEL_FILEPATH}' not found.")
        return
    try:
        print(f"[DEBUG] run_predictor: Loading Keras model from {MODEL_FILEPATH}...")
        model = keras.models.load_model(MODEL_FILEPATH)
        print("[DEBUG] run_predictor: Model loaded successfully.")
    except Exception as e:
        print(f"[CRITICAL] run_predictor: Error loading Keras model: {e}")
        return

    # --- Reference Data & Scaler Prep ---
    if not os.path.exists(TRAINING_DATA_CSV_PATH):
        print(f"[CRITICAL] run_predictor: Training data CSV '{TRAINING_DATA_CSV_PATH}' not found.")
        return
    print(f"[DEBUG] run_predictor: Loading training data ({TRAINING_DATA_CSV_PATH}) for reference...")
    try:
        X_train_ref_full = pd.read_csv(TRAINING_DATA_CSV_PATH)
        print(f"[DEBUG] run_predictor: Training data CSV loaded. Shape: {X_train_ref_full.shape}")
    except Exception as e:
        print(f"[CRITICAL] run_predictor: Error loading training data CSV: {e}")
        return

    cols_to_drop_from_ref = ["id", "attack_cat", "label"]
    if "ct_state_ttl" in X_train_ref_full.columns:
        cols_to_drop_from_ref.append("ct_state_ttl")
    X_train_ref = X_train_ref_full.drop(columns=cols_to_drop_from_ref, errors='ignore')
    X_train_ref_dummies = pd.get_dummies(X_train_ref, columns=CAT_COLS, dummy_na=False)
    reference_model_columns = X_train_ref_dummies.columns.tolist()
    scaler = StandardScaler()
    try:
        scaler.fit(X_train_ref_dummies)
        print("[DEBUG] run_predictor: StandardScaler fitted successfully.")
    except Exception as e:
        print(f"[CRITICAL] run_predictor: Error fitting StandardScaler: {e}")
        return

    # --- ZeroMQ Setup ---
    context = zmq.Context()
    # Data Subscriber
    data_subscriber = context.socket(zmq.SUB)
    try:
        print(f"[DEBUG] run_predictor: Data subscriber connecting to {ZMQ_DATA_PUBLISHER_ADDRESS}...")
        data_subscriber.connect(ZMQ_DATA_PUBLISHER_ADDRESS)
        data_subscriber.subscribe(ZMQ_DATA_TOPIC.encode('utf-8'))
        print("[DEBUG] run_predictor: Data subscriber connected and subscribed.")
    except zmq.ZMQError as e:
        print(f"[CRITICAL] run_predictor: Data subscriber ZMQ connection error: {e}")
        return

    # Statistics Publisher (NEW)
    stats_publisher = context.socket(zmq.PUB)
    try:
        print(f"[DEBUG] run_predictor: Statistics publisher binding to {ZMQ_STATS_PUBLISHER_ADDRESS}...")
        stats_publisher.bind(ZMQ_STATS_PUBLISHER_ADDRESS)
        print("[DEBUG] run_predictor: Statistics publisher bound successfully.")
        # Brief pause to allow subscribers to connect to the stats publisher
        time.sleep(1)
    except zmq.ZMQError as e:
        print(f"[CRITICAL] run_predictor: Statistics publisher ZMQ bind error: {e}")
        # Clean up data_subscriber if stats_publisher fails to bind
        data_subscriber.close()
        context.term()
        return

    print("\n[INFO] run_predictor: Listening for data from publisher...")
    try:
        while True:
            multipart_message = data_subscriber.recv_multipart()
            if len(multipart_message) < 2:
                print("[WARNING] run_predictor: Received incomplete data message. Skipping.")
                continue

            topic_received = multipart_message[0].decode('utf-8')
            binary_payload = multipart_message[1]

            if topic_received == ZMQ_DATA_TOPIC:
                if not binary_payload:
                    continue
                df_new_data_raw = unpack_data_to_df(binary_payload)
                if df_new_data_raw.empty:
                    continue
                
                total_processed_count += len(df_new_data_raw)
                df_processed = df_new_data_raw.copy()
                df_processed = map_enums_to_strings(df_processed)
                cols_to_drop_actually_present = [col for col in COLS_TO_DROP_FOR_MODEL_INPUT if col in df_processed.columns]
                X_new = df_processed.drop(columns=cols_to_drop_actually_present)
                X_new_dummies = pd.get_dummies(X_new, columns=CAT_COLS, dummy_na=False)
                X_new_aligned = X_new_dummies.reindex(columns=reference_model_columns, fill_value=0)
                X_new_scaled = scaler.transform(X_new_aligned)
                
                predictions_probs = model.predict(X_new_scaled, verbose=0) # verbose=0 to reduce Keras logs
                predictions = (predictions_probs > 0.5).astype(int)
                
                results_df = pd.DataFrame({
                    'id': df_new_data_raw['id'],
                    'original_label': df_new_data_raw['label'].astype(int),
                    'prediction_prob': predictions_probs.flatten(),
                    'prediction': predictions.flatten()
                })
                results_df['prediction_class'] = results_df['prediction'].apply(lambda x: "Attack" if x == 1 else "Normal")

                print("\n[INFO] Predictions for the current batch:")
                for _, row in results_df.iterrows():
                    print(f"  ID: {row['id']:<5} | Original: {'Attack' if row['original_label'] else 'Normal':<7} | Predicted: {row['prediction_class']:<7} (Prob: {row['prediction_prob']:.4f})")
                    true_label = row['original_label']
                    pred_label = row['prediction']
                    if true_label == 1 and pred_label == 1: tp_count += 1
                    elif true_label == 0 and pred_label == 1: fp_count += 1
                    elif true_label == 1 and pred_label == 0: fn_count += 1
                    elif true_label == 0 and pred_label == 0: tn_count += 1
                
                accuracy = (tp_count + tn_count) / total_processed_count if total_processed_count > 0 else 0
                precision = tp_count / (tp_count + fp_count) if (tp_count + fp_count) > 0 else 0
                recall = tp_count / (tp_count + fn_count) if (tp_count + fn_count) > 0 else 0
                f1_score_val = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
                
                stats_summary = {
                    "total_processed": total_processed_count,
                    "tp": tp_count, "fp": fp_count, "fn": fn_count, "tn": tn_count,
                    "accuracy": round(accuracy, 4), "precision": round(precision, 4),
                    "recall": round(recall, 4), "f1_score": round(f1_score_val, 4)
                }
                print(f"\n[INFO] --- Publishing Cumulative Stats --- \n{json.dumps(stats_summary, indent=2)}")
                publish_statistics(stats_publisher, stats_summary)
            else:
                print(f"[WARNING] run_predictor: Received message on unexpected data topic: '{topic_received}'")
            time.sleep(0.05)

    except KeyboardInterrupt:
        print("\n[INFO] run_predictor: Predictor stopped by user (Ctrl+C).")
    except zmq.error.ContextTerminated:
        print("\n[INFO] run_predictor: ZeroMQ context terminated. Exiting loop.")
    except Exception as e:
        print(f"[CRITICAL] run_predictor: An unexpected error occurred: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("\n[INFO] --- Final Cumulative Statistics ---")
        accuracy = (tp_count + tn_count) / total_processed_count if total_processed_count > 0 else 0
        precision = tp_count / (tp_count + fp_count) if (tp_count + fp_count) > 0 else 0
        recall = tp_count / (tp_count + fn_count) if (tp_count + fn_count) > 0 else 0
        f1_score_val = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
        final_stats = {
            "total_processed": total_processed_count,
            "tp": tp_count, "fp": fp_count, "fn": fn_count, "tn": tn_count,
            "accuracy": round(accuracy, 4), "precision": round(precision, 4),
            "recall": round(recall, 4), "f1_score": round(f1_score_val, 4)
        }
        print(json.dumps(final_stats, indent=2))
        publish_statistics(stats_publisher, final_stats) # Publish final stats
        
        print("\n[DEBUG] run_predictor: Closing ZeroMQ sockets and context...")
        if 'data_subscriber' in locals() and data_subscriber and not data_subscriber.closed:
            data_subscriber.close()
        if 'stats_publisher' in locals() and stats_publisher and not stats_publisher.closed:
            stats_publisher.close()
        if 'context' in locals() and context and not context.closed:
            context.term()
        print("[DEBUG] run_predictor: Shutdown complete.")

if __name__ == "__main__":
    print("[DEBUG] __main__: Starting script execution.")
    # Basic file checks
    for f_path in [MODEL_FILEPATH, TRAINING_DATA_CSV_PATH, "categorical.py"]:
        if not os.path.exists(f_path):
            print(f"[CRITICAL] __main__: Required file '{f_path}' not found.")
            sys.exit(1)
    
    if DATA_STRUCT_UNPACKER.size != 113:
         print(f"[WARNING] __main__: DATA_STRUCT_UNPACKER.size is {DATA_STRUCT_UNPACKER.size}, expected 113.")
    
    run_predictor()
    print("[DEBUG] __main__: Script execution finished.")

