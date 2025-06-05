import math
import os
import struct
import sys
import time
import zmq
from pandas import DataFrame, Series
from sdv.single_table import CTGANSynthesizer

try:
    from categorical import Attack_cat, Protocolo, Servico, State
    print("Successfully imported 'categorical.py'")
except ImportError as e:
    print(f"Error importing 'categorical.py': {e}")
    print("Please ensure 'categorical.py' is in the same directory as this script (/app in the container) and that the Dockerfile copies it.")
    sys.exit(1)

# --- GAN Model Configuration ---
SAVED_MODEL_FILEPATH = "sdv_ctgan_unsw2_model.pkl"
NUM_SYNTHETIC_SAMPLES = 10 

# --- Define the binary format string based on data.h constructor ---
# This format string defines the structure of the binary data to be sent.
# It must exactly match the C++ struct definition on the subscriber side.
# < : Little-endian
# I : uint32_t
# f : float
# H : uint16_t
# B : uint8_t
# ? : bool
DATA_STRUCT_FORMAT = (
    "<" +  # Little-endian
    "I" +  # id (uint32_t)
    "f" * 11 +  # 11 floats (dur, rate, sload, dload, sinpkt, dinpkt, sjit, djit, tcprtt, synack, ackdat)
    "H" * 2 +  # 2 uint16_t (spkts, dpkts)
    "I" * 2 +  # 2 uint32_t (sbytes, dbytes)
    "B" * 2 +  # 2 uint8_t (sttl, dttl)
    "H" * 2 +  # 2 uint16_t (sloss, dloss)
    "H" +  # 1 uint16_t (swin)
    "I" +  # 1 uint32_t (stcpb)
    "I" +  # 1 uint32_t (dtcpb)
    "H" +  # 1 uint16_t (dwin)
    "H" * 2 +  # 2 uint16_t (smean, dmean)
    "H" +  # 1 uint16_t (trans_depth)
    "I" +  # 1 uint32_t (response_body_len)
    "H" * 6 +  # 6 uint16_t (ct_srv_src, ct_state_ttl, ct_dst_ltm, ct_src_dport_ltm, ct_dst_sport_ltm, ct_dst_src_ltm)
    "H" * 4 +  # 4 uint16_t (ct_ftp_cmd, ct_flw_http_mthd, ct_src_ltm, ct_srv_dst)
    "?" * 3 +  # 3 bools (is_ftp_login, is_sm_ips_ports, label)
    "B" * 4   # 4 uint8_t (enums: proto, state, attack_cat, service)
)
DATA_STRUCT = struct.Struct(DATA_STRUCT_FORMAT)
print(f"Python: Binary stream format string: {DATA_STRUCT_FORMAT}")
print(f"Python: Expected size of one packed data record: {DATA_STRUCT.size} bytes") 

# --- Define expected DataFrame column order at module level ---
# This list defines the order of columns expected from the DataFrame to be packed.
# The order MUST match the sequence of fields in DATA_STRUCT_FORMAT and the C++ struct.
expected_df_columns_in_order = [
    'id', 'dur', 'rate', 'sload', 'dload', 'sinpkt', 'dinpkt', 'sjit', 'djit',
    'tcprtt', 'synack', 'ackdat', 'spkts', 'dpkts', 'sbytes', 'dbytes',
    'sttl', 'dttl', 'sloss', 'dloss', 'swin', 'stcpb', 'dtcpb', 'dwin',
    'smean', 'dmean', 'trans_depth', 'response_body_len', 'ct_srv_src',
    'ct_state_ttl', 'ct_dst_ltm', 'ct_src_dport_ltm', 'ct_dst_sport_ltm',
    'ct_dst_src_ltm', 'ct_ftp_cmd', 'ct_flw_http_mthd', 'ct_src_ltm',
    'ct_srv_dst', 'is_ftp_login', 'is_sm_ips_ports', 'label',
    'proto', 'state', 'attack_cat', 'service' 
]


try:
    model = CTGANSynthesizer.load(filepath=SAVED_MODEL_FILEPATH)
    print(f"Python: Successfully loaded CTGAN model from {SAVED_MODEL_FILEPATH}")
except Exception as e:
    print(f"Python: Error loading CTGAN model: {e}")
    sys.exit(1)

# --- PROTOCOLO_MAP Initialization ---
PROTOCOLO_MAP = {member.name.upper(): member.value for member in Protocolo}
print(f"Python: Initial PROTOCOLO_MAP populated with {len(PROTOCOLO_MAP)} entries from Protocolo enum.")
print("Python: Adding hyphenated aliases to PROTOCOLO_MAP for underscore-containing enum members...")
for member in Protocolo:
    enum_member_name_upper = member.name.upper()
    if '_' in enum_member_name_upper:
        hyphenated_alias_key = enum_member_name_upper.replace('_', '-')
        if hyphenated_alias_key not in PROTOCOLO_MAP:
            PROTOCOLO_MAP[hyphenated_alias_key] = member.value
            # print(f"  Added alias: GAN proto '{hyphenated_alias_key.lower()}' -> enum {member.name} (value: {member.value})")
        elif PROTOCOLO_MAP[hyphenated_alias_key] != member.value:
            print(f"  WARNING: Alias conflict for proto. Hyphenated key '{hyphenated_alias_key}' "
                  f"(for enum {member.name}) already exists in PROTOCOLO_MAP and maps to a different value "
                  f"({PROTOCOLO_MAP[hyphenated_alias_key]}). Not overwriting with value {member.value}.")
print("Python: Finished adding hyphenated aliases for Protocolo.")


STATE_MAP = {member.name.upper(): member.value for member in State}
ATTACK_CAT_MAP = {member.name.upper(): member.value for member in Attack_cat}

# --- SERVICO_MAP Initialization ---
SERVICO_MAP = {member.name.upper(): member.value for member in Servico}
print(f"Python: Initial SERVICO_MAP populated with {len(SERVICO_MAP)} entries from Servico enum.")
print("Python: Adding hyphenated aliases to SERVICO_MAP for underscore-containing enum members...")
for member in Servico:
    enum_member_name_upper = member.name.upper() 
    if '_' in enum_member_name_upper:
        hyphenated_alias_key = enum_member_name_upper.replace('_', '-')
        if hyphenated_alias_key not in SERVICO_MAP:
            SERVICO_MAP[hyphenated_alias_key] = member.value
            # print(f"  Added alias: GAN service '{hyphenated_alias_key.lower()}' -> enum {member.name} (value: {member.value})")
        elif SERVICO_MAP[hyphenated_alias_key] != member.value:
            print(f"  WARNING: Alias conflict for service. Hyphenated key '{hyphenated_alias_key}' "
                  f"(for enum {member.name}) already exists in SERVICO_MAP and maps to a different value "
                  f"({SERVICO_MAP[hyphenated_alias_key]}). Not overwriting with value {member.value}.")
print("Python: Finished adding hyphenated aliases for Servico.")

if hasattr(Servico, 'NOTHING'):
    if '-' not in SERVICO_MAP or SERVICO_MAP['-'] == Servico.NOTHING.value:
        SERVICO_MAP['-'] = Servico.NOTHING.value 
        print(f"Python: Ensured service '-' maps to Servico.NOTHING (value: {Servico.NOTHING.value})")
    else:
        print(f"Python: WARNING - Service '-' already mapped in SERVICO_MAP to a different value ({SERVICO_MAP['-']}). Not overriding for Servico.NOTHING.")
elif '-' not in SERVICO_MAP : 
    print("Python: WARNING - Servico.NOTHING not found in categorical.py. Cannot map GAN output '-' for service.")


def safe_categorical_to_int(value, enum_map, field_name="<unknown_field>", default_value=0):
    if value is None:
        # print(f"Debug Pack [{field_name}]: Input value is None, returning default {default_value}") # Reduced verbosity
        return default_value
    if isinstance(value, (int, float)): 
        return int(value)
    elif isinstance(value, str):
        lookup_val = value if field_name == 'service' and value == '-' else value.upper()
        if lookup_val not in enum_map:
            print(f"Debug Pack [{field_name}]: Value '{value}' (lookup as '{lookup_val}') not in enum_map. Keys: {list(enum_map.keys())}. Returning default {default_value}")
        return enum_map.get(lookup_val, default_value)
    elif hasattr(value, 'value') and isinstance(value.value, int): 
        return value.value
    print(f"Debug Pack [{field_name}]: Unexpected type for categorical value '{value}' (type: {type(value)}). Returning default: {default_value}")
    return default_value

def safe_numeric_conversion(value, target_type, field_name="<unknown_field>", default_value=0, min_val=None, max_val=None):
    if value is None:
        # print(f"Debug Pack [{field_name}]: Input value is None, returning default {default_value}") # Reduced verbosity
        return default_value
    try:
        if isinstance(value, float) and (math.isnan(value) or math.isinf(value)):
            # print(f"Debug Pack [{field_name}]: NaN/Inf detected for float value, returning default {default_value}") # Reduced verbosity
            return default_value
        original_value_for_debug = value 
        if target_type == int:
            if isinstance(value, str): 
                try:
                    value = float(value) 
                except ValueError: 
                    print(f"Debug Pack [{field_name}]: String to float conversion failed for '{original_value_for_debug}', returning default {default_value}")
                    return default_value
            converted_value = int(round(float(value))) 
            if min_val is not None and converted_value < min_val:
                # print(f"Debug Pack [{field_name}]: Value {converted_value} below min {min_val}, clamping to {min_val}")
                converted_value = min_val
            if max_val is not None and converted_value > max_val:
                # print(f"Debug Pack [{field_name}]: Value {converted_value} above max {max_val}, clamping to {max_val}")
                converted_value = max_val
            return converted_value
        elif target_type == float:
            return float(value)
        elif target_type == bool:
            if isinstance(value, str): 
                return value.lower() in ['true', '1', 't', 'y', 'yes', '1.0', 'on']
            if isinstance(value, (int, float)): 
                return bool(int(value)) 
            return bool(value) 
        else: 
            print(f"Debug Pack [{field_name}]: Unknown target_type {target_type} for value {original_value_for_debug}, returning default {default_value}")
            return default_value
    except (ValueError, TypeError) as e: 
        print(f"Debug Pack [{field_name}]: Conversion error for value '{original_value_for_debug}' to {target_type.__name__}. Error: {e}. Returning default: {default_value}")
        return default_value

def generate_synthetic_data(current_id_start: int) -> DataFrame:
    print(f"Python: Generating {NUM_SYNTHETIC_SAMPLES} synthetic samples...")
    synthetic_data_df = model.sample(num_rows=NUM_SYNTHETIC_SAMPLES)
    if not synthetic_data_df.empty:
        print(f"Python: Columns in GAN-generated DataFrame: {synthetic_data_df.columns.tolist()}")
    else:
        print("Python: Generated DataFrame is empty!")
        return synthetic_data_df 
    id_range = range(current_id_start, current_id_start + NUM_SYNTHETIC_SAMPLES)
    synthetic_data_df["id"] = Series(id_range, index=synthetic_data_df.index, dtype="int32")
    if "id" in synthetic_data_df.columns:
        cols = ["id"] + [col for col in synthetic_data_df.columns if col != "id"]
        synthetic_data_df = synthetic_data_df[cols]
    print("Python: Synthetic data generation complete.")
    return synthetic_data_df

def python_pub_publisher():
    context = zmq.Context()
    publisher_socket = context.socket(zmq.PUB)
    port = "5556" 
    publisher_socket.bind(f"tcp://*:{port}")
    print(f"Python PUB publisher bound to tcp://*:{port}")
    
    # ** INCREASED SLEEP DURATION **
    initial_sleep_duration = 5 # seconds
    print(f"Python: Initial sleep for {initial_sleep_duration} seconds to allow subscribers to connect...")
    time.sleep(initial_sleep_duration) 
    print("Python: Initial sleep complete. Starting data generation and sending.")

    current_id_counter = 0 
    U8_MAX = 255
    U16_MAX = 65535
    first_batch_check_done = False 

    try:
        while True:
            print("\nPython: --- Starting new batch generation ---")
            synthetic_df = generate_synthetic_data(current_id_counter)
            if synthetic_df.empty:
                print("Python: Generated an empty DataFrame. Skipping sending this batch.")
                current_id_counter += NUM_SYNTHETIC_SAMPLES 
                time.sleep(1) 
                continue
            current_id_counter += NUM_SYNTHETIC_SAMPLES 

            if not first_batch_check_done:
                print(f"Python: Verifying DataFrame columns from GAN against expected order for packing:")
                actual_df_cols = synthetic_df.columns.tolist()
                for expected_col in expected_df_columns_in_order:
                    if expected_col == 'id': 
                        continue
                    if expected_col not in actual_df_cols:
                        print(f"  WARNING: Expected packing column '{expected_col}' NOT FOUND in GAN's output DataFrame columns: {actual_df_cols}")
                print("  Expected packing order (from expected_df_columns_in_order):")
                for i, col_name in enumerate(expected_df_columns_in_order):
                    print(f"    Pack item {i}: '{col_name}' (for struct field type '{DATA_STRUCT_FORMAT[i+1]}')") 
                first_batch_check_done = True

            all_packed_data = b"" 
            sent_ids_in_batch = [] 
            rows_successfully_packed = 0

            for index, row in synthetic_df.iterrows():
                current_row_id = row.get('id', 'N/A_IN_ROW_DATA') 
                # print(f"\nPython: Processing DataFrame row index {index}, Record ID: {current_row_id}") # Reduced verbosity
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
                    for i in range(28, 34): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    for i in range(34, 38): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    for i in range(38, 41): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), bool, field_name=expected_df_columns_in_order[i]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[41]), PROTOCOLO_MAP, field_name=expected_df_columns_in_order[41]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[42]), STATE_MAP, field_name=expected_df_columns_in_order[42]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[43]), ATTACK_CAT_MAP, field_name=expected_df_columns_in_order[43])) 
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[44]), SERVICO_MAP, field_name=expected_df_columns_in_order[44]))
                    data_to_pack_tuple = tuple(_data_values)
                    if len(data_to_pack_tuple) != len(expected_df_columns_in_order):
                        print(f"Python: CRITICAL PACKING ERROR for row index {index}, ID {current_row_id}: "
                              f"Prepared data tuple has {len(data_to_pack_tuple)} items, "
                              f"but expected {len(expected_df_columns_in_order)}. Skipping row.")
                        continue 
                    packed_data_for_row = DATA_STRUCT.pack(*data_to_pack_tuple)
                    all_packed_data += packed_data_for_row
                    sent_ids_in_batch.append(current_row_id)
                    rows_successfully_packed += 1
                except struct.error as se:
                    print(f"Python: Struct packing error for row index {index}, ID {current_row_id}: {se}. Skipping row.")
                    continue 
                except Exception as ex:
                    print(f"Python: General error processing row index {index}, ID {current_row_id}: {ex}. Skipping row.")
                    import traceback
                    traceback.print_exc() 
                    continue
            
            print(f"Python: Finished processing batch. {rows_successfully_packed} of {NUM_SYNTHETIC_SAMPLES} rows successfully packed.")

            if all_packed_data:
                topic = b"data_batch" 
                full_message = topic + b" " + all_packed_data 
                print(f"Python: Sending ZMQ message. Topic size: {len(topic)}, Data payload size: {len(all_packed_data)}, Total ZMQ message size: {len(full_message)}")
                publisher_socket.send(full_message)
                if sent_ids_in_batch:
                    print(f"Python: Sent batch of {len(sent_ids_in_batch)} Data structs (IDs: {sent_ids_in_batch[0]} to {sent_ids_in_batch[-1]})")
                else: 
                    print(f"Python: Warning: all_packed_data is not empty, but sent_ids_in_batch is. This is unexpected.")
            else:
                print("Python: No data was packed in this batch.")

            # Regular sleep between batches
            time.sleep(2) 
    except KeyboardInterrupt:
        print("\nPython: Publisher stopped by user (Ctrl+C).")
    except Exception as e:
        print(f"Python: An unexpected error occurred in the main publishing loop: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("Python: Closing publisher socket and terminating context...")
        publisher_socket.close()
        context.term()
        print("Python: Python PUB publisher closed.")

if __name__ == "__main__":
    EXPECTED_FIELD_COUNT = 45 
    EXPECTED_STRUCT_SIZE_BYTES = 115 

    if len(expected_df_columns_in_order) != EXPECTED_FIELD_COUNT:
         print(f"CRITICAL PYTHON SETUP ERROR: 'expected_df_columns_in_order' has {len(expected_df_columns_in_order)} items, but EXPECTED_FIELD_COUNT is {EXPECTED_FIELD_COUNT}. These must match.")
         sys.exit(1)
    if DATA_STRUCT.size != EXPECTED_STRUCT_SIZE_BYTES:
         print(f"CRITICAL PYTHON SETUP ERROR: DATA_STRUCT.size is {DATA_STRUCT.size}, but EXPECTED_STRUCT_SIZE_BYTES is {EXPECTED_STRUCT_SIZE_BYTES}. Verify DATA_STRUCT_FORMAT against C++ sizeof(Data) and field counts.")
         sys.exit(1)
    if (len(DATA_STRUCT_FORMAT) -1) != EXPECTED_FIELD_COUNT: 
        print(f"CRITICAL PYTHON SETUP ERROR: DATA_STRUCT_FORMAT implies {len(DATA_STRUCT_FORMAT)-1} fields, but EXPECTED_FIELD_COUNT is {EXPECTED_FIELD_COUNT}.")
        sys.exit(1)

    python_pub_publisher()
