import math
import os
import struct
import sys
import time
import zmq
from pandas import DataFrame, Series
from sdv.single_table import CTGANSynthesizer

# Ensure Python prints output immediately
sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

try:
    from categorical import Attack_cat, Protocolo, Servico, State
    print("[DEBUG] generate.py: Successfully imported 'categorical.py'")
except ImportError as e:
    print(f"[ERROR] generate.py: Error importing 'categorical.py': {e}")
    sys.exit(1)

# --- GAN Model Configuration ---
SAVED_MODEL_FILEPATH = "sdv_ctgan_unsw2_model.pkl"
NUM_SYNTHETIC_SAMPLES = 200

# --- Define the binary format string based on data.h constructor ---
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
    "H" * 5 +  # 5 uint16_t (ct_srv_src, ct_dst_ltm, ct_src_dport_ltm, ct_dst_sport_ltm, ct_dst_src_ltm) - ct_state_ttl removed
    "H" * 4 +  # 4 uint16_t (ct_ftp_cmd, ct_flw_http_mthd, ct_src_ltm, ct_srv_dst)
    "?" * 3 +  # 3 bools (is_ftp_login, is_sm_ips_ports, label)
    "B" * 4   # 4 uint8_t (enums: proto, state, attack_cat, service)
)
DATA_STRUCT = struct.Struct(DATA_STRUCT_FORMAT)
print(f"[INFO] generate.py: Binary stream format string: {DATA_STRUCT_FORMAT}")
print(f"[INFO] generate.py: Expected size of one packed data record: {DATA_STRUCT.size} bytes")

# --- Define expected DataFrame column order at module level ---
expected_df_columns_in_order = [
    'id', 'dur', 'rate', 'sload', 'dload', 'sinpkt', 'dinpkt', 'sjit', 'djit',
    'tcprtt', 'synack', 'ackdat', 'spkts', 'dpkts', 'sbytes', 'dbytes',
    'sttl', 'dttl', 'sloss', 'dloss', 'swin', 'stcpb', 'dtcpb', 'dwin',
    'smean', 'dmean', 'trans_depth', 'response_body_len', 'ct_srv_src',
    # 'ct_state_ttl', # Removed
    'ct_dst_ltm', 'ct_src_dport_ltm', 'ct_dst_sport_ltm',
    'ct_dst_src_ltm', 'ct_ftp_cmd', 'ct_flw_http_mthd', 'ct_src_ltm',
    'ct_srv_dst', 'is_ftp_login', 'is_sm_ips_ports', 'label',
    'proto', 'state', 'attack_cat', 'service'
]


try:
    print(f"[DEBUG] generate.py: Loading CTGAN model from {SAVED_MODEL_FILEPATH}...")
    model = CTGANSynthesizer.load(filepath=SAVED_MODEL_FILEPATH)
    print(f"[INFO] generate.py: Successfully loaded CTGAN model from {SAVED_MODEL_FILEPATH}")
except Exception as e:
    print(f"[ERROR] generate.py: Error loading CTGAN model: {e}")
    sys.exit(1)

# --- PROTOCOLO_MAP Initialization ---
PROTOCOLO_MAP = {member.name.upper(): member.value for member in Protocolo}
# print(f"[DEBUG] generate.py: Initial PROTOCOLO_MAP: {PROTOCOLO_MAP}")
for member in Protocolo:
    enum_member_name_upper = member.name.upper()
    if '_' in enum_member_name_upper:
        hyphenated_alias_key = enum_member_name_upper.replace('_', '-')
        if hyphenated_alias_key not in PROTOCOLO_MAP:
            PROTOCOLO_MAP[hyphenated_alias_key] = member.value
        elif PROTOCOLO_MAP[hyphenated_alias_key] != member.value:
            print(f"  [WARNING] generate.py: Alias conflict for proto. Key '{hyphenated_alias_key}' not overwritten.")
# print(f"[DEBUG] generate.py: Final PROTOCOLO_MAP: {PROTOCOLO_MAP}")


STATE_MAP = {member.name.upper(): member.value for member in State}
ATTACK_CAT_MAP = {member.name.upper(): member.value for member in Attack_cat}

# --- SERVICO_MAP Initialization ---
SERVICO_MAP = {member.name.upper(): member.value for member in Servico}
# print(f"[DEBUG] generate.py: Initial SERVICO_MAP: {SERVICO_MAP}")
for member in Servico:
    enum_member_name_upper = member.name.upper()
    if '_' in enum_member_name_upper:
        hyphenated_alias_key = enum_member_name_upper.replace('_', '-')
        if hyphenated_alias_key not in SERVICO_MAP:
            SERVICO_MAP[hyphenated_alias_key] = member.value
        elif SERVICO_MAP[hyphenated_alias_key] != member.value:
             print(f"  [WARNING] generate.py: Alias conflict for service. Key '{hyphenated_alias_key}' not overwritten.")

if hasattr(Servico, 'NOTHING'):
    if '-' not in SERVICO_MAP or SERVICO_MAP['-'] == Servico.NOTHING.value:
        SERVICO_MAP['-'] = Servico.NOTHING.value
        # print(f"[DEBUG] generate.py: Ensured service '-' maps to Servico.NOTHING (value: {Servico.NOTHING.value})")
    else:
        print(f"[WARNING] generate.py: Service '-' already mapped in SERVICO_MAP to a different value. Not overriding for Servico.NOTHING.")
elif '-' not in SERVICO_MAP :
    print("[WARNING] generate.py: Servico.NOTHING not found in categorical.py. Cannot map GAN output '-' for service.")
# print(f"[DEBUG] generate.py: Final SERVICO_MAP: {SERVICO_MAP}")


def safe_categorical_to_int(value, enum_map, field_name="<unknown_field>", default_value=0):
    if value is None:
        return default_value
    if isinstance(value, (int, float)):
        return int(value)
    elif isinstance(value, str):
        lookup_val = value if field_name == 'service' and value == '-' else value.upper()
        if lookup_val not in enum_map:
            print(f"[WARNING] safe_categorical_to_int [{field_name}]: Value '{value}' (lookup '{lookup_val}') not in enum_map. Using default {default_value}.")
        return enum_map.get(lookup_val, default_value)
    elif hasattr(value, 'value') and isinstance(value.value, int):
        return value.value
    print(f"[WARNING] safe_categorical_to_int [{field_name}]: Unexpected type for value '{value}' (type: {type(value)}). Using default {default_value}.")
    return default_value

def safe_numeric_conversion(value, target_type, field_name="<unknown_field>", default_value=0, min_val=None, max_val=None):
    if value is None:
        return default_value
    try:
        if isinstance(value, float) and (math.isnan(value) or math.isinf(value)):
            return default_value
        original_value_for_debug = value
        if target_type == int:
            if isinstance(value, str):
                try: value = float(value)
                except ValueError: return default_value
            converted_value = int(round(float(value)))
            if min_val is not None and converted_value < min_val: converted_value = min_val
            if max_val is not None and converted_value > max_val: converted_value = max_val
            return converted_value
        elif target_type == float:
            return float(value)
        elif target_type == bool:
            if isinstance(value, str): return value.lower() in ['true', '1', 't', 'y', 'yes', '1.0', 'on']
            if isinstance(value, (int, float)): return bool(int(value))
            return bool(value)
        else:
            print(f"[WARNING] safe_numeric_conversion [{field_name}]: Unknown target_type {target_type} for value {original_value_for_debug}. Using default {default_value}")
            return default_value
    except (ValueError, TypeError) as e:
        print(f"[WARNING] safe_numeric_conversion [{field_name}]: Conversion error for value '{original_value_for_debug}' to {target_type.__name__}. Error: {e}. Using default {default_value}.")
        return default_value

def generate_synthetic_data(current_id_start: int) -> DataFrame:
    # print(f"[DEBUG] generate.py: Generating {NUM_SYNTHETIC_SAMPLES} synthetic samples starting ID {current_id_start}...")
    synthetic_data_df = model.sample(num_rows=NUM_SYNTHETIC_SAMPLES)
    # if not synthetic_data_df.empty:
    #     print(f"[DEBUG] generate.py: Columns in GAN-generated DataFrame: {synthetic_data_df.columns.tolist()}")
    # else:
    #     print("[WARNING] generate.py: Generated DataFrame is empty!")
    #     return synthetic_data_df
    id_range = range(current_id_start, current_id_start + NUM_SYNTHETIC_SAMPLES)
    synthetic_data_df["id"] = Series(id_range, index=synthetic_data_df.index, dtype="int32")
    if "id" in synthetic_data_df.columns:
        cols = ["id"] + [col for col in synthetic_data_df.columns if col != "id"]
        synthetic_data_df = synthetic_data_df[cols]
    # print("[DEBUG] generate.py: Synthetic data generation complete.")
    return synthetic_data_df

def python_pub_publisher():
    context = zmq.Context()
    publisher_socket = context.socket(zmq.PUB)
    port = "5556"
    publisher_socket.bind(f"tcp://*:{port}")
    print(f"[INFO] generate.py: Python PUB publisher bound to tcp://*:{port}")

    initial_sleep_duration = 5 # seconds
    print(f"[INFO] generate.py: Initial sleep for {initial_sleep_duration} seconds to allow subscribers to connect...")
    time.sleep(initial_sleep_duration)
    print("[INFO] generate.py: Initial sleep complete. Starting data generation and sending.")

    current_id_counter = 0
    U8_MAX = 255
    U16_MAX = 65535
    first_batch_check_done = False

    try:
        while True:
            print("\n[INFO] generate.py: --- Starting new batch generation ---")
            synthetic_df = generate_synthetic_data(current_id_counter)
            if synthetic_df.empty:
                print("[WARNING] generate.py: Generated an empty DataFrame. Skipping sending this batch.")
                current_id_counter += NUM_SYNTHETIC_SAMPLES
                time.sleep(1)
                continue
            current_id_counter += NUM_SYNTHETIC_SAMPLES

            if not first_batch_check_done:
                # print(f"[DEBUG] generate.py: Verifying DataFrame columns from GAN against expected order for packing:")
                actual_df_cols = synthetic_df.columns.tolist()
                for expected_col in expected_df_columns_in_order:
                    if expected_col == 'id': continue
                    if expected_col not in actual_df_cols:
                        print(f"  [WARNING] generate.py: Expected packing column '{expected_col}' NOT FOUND in GAN's output DataFrame columns: {actual_df_cols}")
                # print("  [DEBUG] generate.py: Expected packing order (from expected_df_columns_in_order):")
                # for i, col_name in enumerate(expected_df_columns_in_order):
                #     print(f"    Pack item {i}: '{col_name}' (for struct field type '{DATA_STRUCT_FORMAT[i+1]}')")
                first_batch_check_done = True

            all_packed_data = b""
            sent_ids_in_batch = []
            rows_successfully_packed = 0

            for index, row in synthetic_df.iterrows():
                current_row_id = row.get('id', 'N/A_IN_ROW_DATA')
                try:
                    _data_values = []
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[0]), int, field_name=expected_df_columns_in_order[0]))
                    for i in range(1, 12): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), float, field_name=expected_df_columns_in_order[i]))
                    for i in range(12, 14): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    for i in range(14, 16): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0))
                    for i in range(16, 18): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U8_MAX))
                    for i in range(18, 20): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[20]), int, field_name=expected_df_columns_in_order[20], min_val=0, max_val=U16_MAX))
                    for i in range(21, 23): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0))
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[23]), int, field_name=expected_df_columns_in_order[23], min_val=0, max_val=U16_MAX))
                    for i in range(24, 26): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[26]), int, field_name=expected_df_columns_in_order[26], min_val=0, max_val=U16_MAX))
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[27]), int, field_name=expected_df_columns_in_order[27], min_val=0))
                    for i in range(28, 33): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    for i in range(33, 37): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    for i in range(37, 40): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), bool, field_name=expected_df_columns_in_order[i]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[40]), PROTOCOLO_MAP, field_name=expected_df_columns_in_order[40]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[41]), STATE_MAP, field_name=expected_df_columns_in_order[41]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[42]), ATTACK_CAT_MAP, field_name=expected_df_columns_in_order[42]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[43]), SERVICO_MAP, field_name=expected_df_columns_in_order[43]))
                    data_to_pack_tuple = tuple(_data_values)
                    if len(data_to_pack_tuple) != len(expected_df_columns_in_order):
                        print(f"[ERROR] generate.py: CRITICAL PACKING ERROR for ID {current_row_id}: "
                              f"Prepared data tuple has {len(data_to_pack_tuple)} items, "
                              f"but expected {len(expected_df_columns_in_order)}. Skipping row.")
                        continue
                    packed_data_for_row = DATA_STRUCT.pack(*data_to_pack_tuple)
                    all_packed_data += packed_data_for_row
                    sent_ids_in_batch.append(current_row_id)
                    rows_successfully_packed += 1
                except struct.error as se:
                    print(f"[ERROR] generate.py: Struct packing error for ID {current_row_id}: {se}. Skipping row.")
                    continue
                except Exception as ex:
                    print(f"[ERROR] generate.py: General error processing ID {current_row_id}: {ex}. Skipping row.")
                    import traceback
                    traceback.print_exc()
                    continue

            # print(f"[DEBUG] generate.py: Finished processing batch. {rows_successfully_packed} of {NUM_SYNTHETIC_SAMPLES} rows packed.")

            if all_packed_data:
                topic = ZMQ_TOPIC.encode('utf-8') # Use the ZMQ_TOPIC variable
                # Send as a multipart message: [topic, payload]
                publisher_socket.send_multipart([topic, all_packed_data])
                print(f"[INFO] generate.py: Sent ZMQ multipart message. Topic: '{ZMQ_TOPIC}', Payload size: {len(all_packed_data)}")
                if sent_ids_in_batch:
                    print(f"  [INFO] generate.py: Sent batch of {len(sent_ids_in_batch)} Data structs (IDs: {sent_ids_in_batch[0]} to {sent_ids_in_batch[-1]})")
                else:
                    print(f"  [WARNING] generate.py: all_packed_data was not empty, but sent_ids_in_batch is. This is unexpected.")
            else:
                print("[INFO] generate.py: No data was packed in this batch. Nothing to send.")

            time.sleep(2)
    except KeyboardInterrupt:
        print("\n[INFO] generate.py: Publisher stopped by user (Ctrl+C).")
    except Exception as e:
        print(f"[ERROR] generate.py: An unexpected error occurred in the main publishing loop: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("[INFO] generate.py: Closing publisher socket and terminating context...")
        publisher_socket.close()
        context.term()
        print("[INFO] generate.py: Python PUB publisher closed.")

if __name__ == "__main__":
    print("[DEBUG] generate.py: __main__ block started.")
    EXPECTED_FIELD_COUNT = 44 # Reduced by 1
    EXPECTED_STRUCT_SIZE_BYTES = 113 # Recalculated

    ZMQ_TOPIC = "data_batch" # Define it here as well if not defined globally earlier for some reason

    if len(expected_df_columns_in_order) != EXPECTED_FIELD_COUNT:
         print(f"[CRITICAL] generate.py: 'expected_df_columns_in_order' has {len(expected_df_columns_in_order)}, but EXPECTED_FIELD_COUNT is {EXPECTED_FIELD_COUNT}.")
         sys.exit(1)
    if DATA_STRUCT.size != EXPECTED_STRUCT_SIZE_BYTES:
         print(f"[CRITICAL] generate.py: DATA_STRUCT.size is {DATA_STRUCT.size}, but EXPECTED_STRUCT_SIZE_BYTES is {EXPECTED_STRUCT_SIZE_BYTES}.")
         sys.exit(1)
    if (len(DATA_STRUCT_FORMAT) -1) != EXPECTED_FIELD_COUNT: # -1 for the '<'
        print(f"[CRITICAL] generate.py: DATA_STRUCT_FORMAT implies {len(DATA_STRUCT_FORMAT)-1} fields, but EXPECTED_FIELD_COUNT is {EXPECTED_FIELD_COUNT}.")
        sys.exit(1)
    print("[DEBUG] generate.py: Initial checks passed.")
    python_pub_publisher()
    print("[DEBUG] generate.py: python_pub_publisher function finished.")

