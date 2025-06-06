import zmq
import struct
import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
from tensorflow import keras # Or from keras.models import load_model if using standalone Keras
import time
import sys
import os

# Ensure Python prints output immediately
sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

print("[DEBUG] live_predictor.py: Script started.")

# Attempt to import enums from categorical.py
try:
    from categorical import Protocolo, Servico, State, Attack_cat
    print("[DEBUG] live_predictor.py: Successfully imported enums from categorical.py.")
except ImportError:
    print("Error: Could not import enums from categorical.py.")
    print("Ensure categorical.py is in the same directory as this script.")
    sys.exit(1)

# --- Configuration ---
ZMQ_PUBLISHER_ADDRESS = "tcp://python_publisher:5556" # Using service name for Docker network
ZMQ_TOPIC = "data_batch" 
MODEL_FILEPATH = "model20.keras" 
TRAINING_DATA_CSV_PATH = "UNSW_NB15_training-set.csv" 
print(f"[DEBUG] live_predictor.py: ZMQ_PUBLISHER_ADDRESS set to {ZMQ_PUBLISHER_ADDRESS}")
print(f"[DEBUG] live_predictor.py: MODEL_FILEPATH set to {MODEL_FILEPATH}")
print(f"[DEBUG] live_predictor.py: TRAINING_DATA_CSV_PATH set to {TRAINING_DATA_CSV_PATH}")


# --- Data Structure Definition (from generate.py) ---
DATA_STRUCT_FORMAT = (
    "<" +  # Little-endian
    "I" +  # id (uint32_t)
    "f" * 11 +  # 11 floats (dur, rate, sload, dload, sinpkt, dinpkt, sjit, djit, tcprtt, synack, ackdat)
    "H" * 2 +  # 2 uint16_t (spkts, dpkts)
    "I" * 2 +  # 2 uint32_t (sbytes, dbytes)
    "B" * 2 +  # 2 uint8_t (sttl, dttl)
    "H" * 2 +  # 2 uint16_t (sloss, dloss)
    "H" +  # 1 uint16_t (swin)
    "I" * 2 +  # 2 uint32_t (stcpb, dtcpb)
    "H" +  # 1 uint16_t (dwin)
    "H" * 2 +  # 2 uint16_t (smean, dmean)
    "H" +  # 1 uint16_t (trans_depth)
    "I" +  # 1 uint32_t (response_body_len)
    "H" * 5 +  # 5 uint16_t (ct_srv_src, ct_dst_ltm, ct_src_dport_ltm, ct_dst_sport_ltm, ct_dst_src_ltm)
    "H" * 4 +  # 4 uint16_t (ct_ftp_cmd, ct_flw_http_mthd, ct_src_ltm, ct_srv_dst)
    "?" * 3 +  # 3 bools (is_ftp_login, is_sm_ips_ports, label)
    "B" * 4   # 4 uint8_t (enums: proto, state, attack_cat, service)
)
DATA_STRUCT_UNPACKER = struct.Struct(DATA_STRUCT_FORMAT)
print(f"[DEBUG] live_predictor.py: DATA_STRUCT_UNPACKER initialized. Expected size: {DATA_STRUCT_UNPACKER.size}")


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
    # print(f"[DEBUG] unpack_data_to_df: Received payload of size {len(binary_payload)}")
    while offset < len(binary_payload):
        try:
            if offset + DATA_STRUCT_UNPACKER.size > len(binary_payload):
                # print(f"[DEBUG] unpack_data_to_df: Insufficient data for full struct. Offset: {offset}, Remaining: {len(binary_payload) - offset}")
                break
            record_tuple = DATA_STRUCT_UNPACKER.unpack_from(binary_payload, offset)
            all_records.append(record_tuple)
            offset += DATA_STRUCT_UNPACKER.size
        except struct.error as e:
            print(f"[ERROR] unpack_data_to_df: Struct unpacking error at offset {offset}: {e}. Payload size: {len(binary_payload)}")
            break 
    
    if not all_records:
        # print("[DEBUG] unpack_data_to_df: No records unpacked.")
        return pd.DataFrame()

    # print(f"[DEBUG] unpack_data_to_df: Successfully unpacked {len(all_records)} records.")
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

def run_predictor():
    print("[DEBUG] run_predictor: Function started.")
    tp_count = 0
    fp_count = 0
    fn_count = 0
    tn_count = 0
    total_processed_count = 0

    # --- 1. Load Keras Model ---
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

    # --- 2. Prepare Reference Columns and Scaler (from training data) ---
    if not os.path.exists(TRAINING_DATA_CSV_PATH):
        print(f"[CRITICAL] run_predictor: Training data CSV '{TRAINING_DATA_CSV_PATH}' not found.")
        return
        
    print(f"[DEBUG] run_predictor: Loading training data ({TRAINING_DATA_CSV_PATH}) for reference...")
    try:
        X_train_ref_full = pd.read_csv(TRAINING_DATA_CSV_PATH)
        print(f"[DEBUG] run_predictor: Training data CSV loaded successfully. Shape: {X_train_ref_full.shape}")
    except Exception as e:
        print(f"[CRITICAL] run_predictor: Error loading training data CSV: {e}")
        return
    
    cols_to_drop_from_ref = ["id", "attack_cat", "label"]
    if "ct_state_ttl" in X_train_ref_full.columns: 
        cols_to_drop_from_ref.append("ct_state_ttl")
    
    X_train_ref = X_train_ref_full.drop(columns=cols_to_drop_from_ref, errors='ignore')
    print(f"[DEBUG] run_predictor: Reference features prepared. Shape: {X_train_ref.shape}")
    
    print("[DEBUG] run_predictor: Performing one-hot encoding on reference training data...")
    X_train_ref_dummies = pd.get_dummies(X_train_ref, columns=CAT_COLS, dummy_na=False)
    reference_model_columns = X_train_ref_dummies.columns.tolist()
    print(f"[DEBUG] run_predictor: Reference model columns created. Count: {len(reference_model_columns)}")

    print("[DEBUG] run_predictor: Initializing and fitting StandardScaler...")
    scaler = StandardScaler()
    try:
        scaler.fit(X_train_ref_dummies) 
        print("[DEBUG] run_predictor: StandardScaler fitted successfully.")
    except Exception as e:
        print(f"[CRITICAL] run_predictor: Error fitting StandardScaler: {e}")
        return

    # --- 3. Setup ZeroMQ Subscriber ---
    print(f"[DEBUG] run_predictor: Setting up ZeroMQ subscriber to {ZMQ_PUBLISHER_ADDRESS} on topic '{ZMQ_TOPIC}'...")
    context = zmq.Context()
    subscriber = context.socket(zmq.SUB)
    try:
        print(f"[DEBUG] run_predictor: Attempting to connect to {ZMQ_PUBLISHER_ADDRESS}...")
        subscriber.connect(ZMQ_PUBLISHER_ADDRESS)
        print(f"[DEBUG] run_predictor: Connected. Attempting to subscribe to topic '{ZMQ_TOPIC}'...")
        subscriber.subscribe(ZMQ_TOPIC.encode('utf-8')) 
        print("[DEBUG] run_predictor: Successfully connected and subscribed.")
    except zmq.ZMQError as e:
        print(f"[CRITICAL] run_predictor: ZeroMQ connection error: {e}")
        return
    
    print("\n[INFO] run_predictor: Listening for data from publisher...")
    try:
        while True:
            # print("[DEBUG] run_predictor: Waiting to receive message...") # Can be too verbose
            multipart_message = subscriber.recv_multipart()
            # print(f"[DEBUG] run_predictor: Received multipart message with {len(multipart_message)} parts.")
            
            if len(multipart_message) < 2:
                print("[WARNING] run_predictor: Received incomplete message. Skipping.")
                continue

            topic_received = multipart_message[0].decode('utf-8')
            binary_payload = multipart_message[1]

            if topic_received == ZMQ_TOPIC:
                if not binary_payload:
                    # print("[DEBUG] run_predictor: Received empty binary payload. Skipping.")
                    continue
                
                df_new_data_raw = unpack_data_to_df(binary_payload)

                if df_new_data_raw.empty:
                    # print("[DEBUG] run_predictor: No data records unpacked. Waiting for next message.")
                    continue
                
                # print(f"\n[INFO] run_predictor: Processing batch of {len(df_new_data_raw)} records ---")
                total_processed_count += len(df_new_data_raw)
                
                df_processed = df_new_data_raw.copy()
                df_processed = map_enums_to_strings(df_processed)
                
                cols_to_drop_actually_present = [col for col in COLS_TO_DROP_FOR_MODEL_INPUT if col in df_processed.columns]
                X_new = df_processed.drop(columns=cols_to_drop_actually_present)

                X_new_dummies = pd.get_dummies(X_new, columns=CAT_COLS, dummy_na=False)
                X_new_aligned = X_new_dummies.reindex(columns=reference_model_columns, fill_value=0)
                X_new_scaled = scaler.transform(X_new_aligned)
                
                predictions_probs = model.predict(X_new_scaled)
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
                
                print("\n[INFO] --- Cumulative Confusion Matrix ---")
                print(f"  Total Records Processed: {total_processed_count}")
                print(f"  True Positives (TP):  {tp_count}")
                print(f"  False Positives (FP): {fp_count}")
                print(f"  False Negatives (FN): {fn_count}")
                print(f"  True Negatives (TN):  {tn_count}")
                accuracy = (tp_count + tn_count) / total_processed_count if total_processed_count > 0 else 0
                precision = tp_count / (tp_count + fp_count) if (tp_count + fp_count) > 0 else 0
                recall = tp_count / (tp_count + fn_count) if (tp_count + fn_count) > 0 else 0
                f1_score_val = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0 # Renamed f1_score
                print(f"  Accuracy:  {accuracy:.4f}")
                print(f"  Precision: {precision:.4f}")
                print(f"  Recall:    {recall:.4f}")
                print(f"  F1-Score:  {f1_score_val:.4f}") # Used renamed variable
            else:
                print(f"[WARNING] run_predictor: Received message on unexpected topic: '{topic_received}'")
            
            # time.sleep(0.01) # Reduced sleep or make it optional

    except KeyboardInterrupt:
        print("\n[INFO] run_predictor: Predictor stopped by user (Ctrl+C).")
    except zmq.error.ContextTerminated:
        print("\n[INFO] run_predictor: ZeroMQ context terminated. Exiting loop.")
    except Exception as e:
        print(f"[CRITICAL] run_predictor: An unexpected error occurred in the predictor loop: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("\n[INFO] --- Final Cumulative Confusion Matrix ---")
        print(f"  Total Records Processed: {total_processed_count}")
        print(f"  True Positives (TP):  {tp_count}")
        print(f"  False Positives (FP): {fp_count}")
        print(f"  False Negatives (FN): {fn_count}")
        print(f"  True Negatives (TN):  {tn_count}")
        accuracy = (tp_count + tn_count) / total_processed_count if total_processed_count > 0 else 0
        precision = tp_count / (tp_count + fp_count) if (tp_count + fp_count) > 0 else 0
        recall = tp_count / (tp_count + fn_count) if (tp_count + fn_count) > 0 else 0
        f1_score_val = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
        print(f"  Accuracy:  {accuracy:.4f}")
        print(f"  Precision: {precision:.4f}")
        print(f"  Recall:    {recall:.4f}")
        print(f"  F1-Score:  {f1_score_val:.4f}")
        
        print("\n[DEBUG] run_predictor: Closing ZeroMQ subscriber and context...")
        if 'subscriber' in locals() and subscriber and not subscriber.closed:
            subscriber.close()
        if 'context' in locals() and context and not context.closed:
            context.term()
        print("[DEBUG] run_predictor: Shutdown complete.")

if __name__ == "__main__":
    print("[DEBUG] __main__: Starting script execution.")
    if not os.path.exists(MODEL_FILEPATH):
        print(f"[CRITICAL] __main__: Keras model '{MODEL_FILEPATH}' not found.")
        sys.exit(1)
    if not os.path.exists(TRAINING_DATA_CSV_PATH):
        print(f"[CRITICAL] __main__: Training data CSV '{TRAINING_DATA_CSV_PATH}' not found.")
        sys.exit(1)
    
    if DATA_STRUCT_UNPACKER.size != 113: 
         print(f"[WARNING] __main__: DATA_STRUCT_UNPACKER.size is {DATA_STRUCT_UNPACKER.size}, expected 113.")

    # Change ZMQ_PUBLISHER_ADDRESS if running in Docker and publisher is another container
    if os.environ.get('DOCKER_ENV') == 'true': # Example: Set DOCKER_ENV=true in Dockerfile
        ZMQ_PUBLISHER_ADDRESS = "tcp://python_publisher:5556" 
        print(f"[DEBUG] __main__: Running in Docker, ZMQ_PUBLISHER_ADDRESS set to {ZMQ_PUBLISHER_ADDRESS}")
    
    run_predictor()
    print("[DEBUG] __main__: Script execution finished.")


