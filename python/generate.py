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
DATA_STRUCT_FORMAT = (
    "<" +  # Little-endian
    "I" +  # id (uint32_t)
    "f" * 11 +  # 11 floats
    "H" * 2 +  # 2 uint16_t
    "I" * 2 +  # 2 uint32_t
    "B" * 2 +  # 2 uint8_t
    "H" * 2 +  # 2 uint16_t
    "H" +  # 1 uint16_t (swin)
    "I" +  # 1 uint32_t (stcpb)
    "I" +  # 1 uint32_t (dtcpb)
    "H" +  # 1 uint16_t (dwin)
    "H" * 2 +  # 2 uint16_t
    "H" +  # 1 uint16_t (trans_depth)
    "I" +  # 1 uint32_t (response_body_len)
    "H" * 6 +  # 6 uint16_t
    "H" * 4 +  # 4 uint16_t
    "?" * 3 +  # 3 bools
    "B" * 4   # 4 uint8_t (enums)
)
DATA_STRUCT = struct.Struct(DATA_STRUCT_FORMAT)
print(f"Python: Binary stream format string: {DATA_STRUCT_FORMAT}")
print(f"Python: Expected size of one packed data record: {DATA_STRUCT.size} bytes") # Should be 115 based on this format

# --- Define expected DataFrame column order at module level ---
# This MUST match the order in data_to_pack tuple AND the C++ struct
expected_df_columns_in_order = [
    'id', 'dur', 'rate', 'sload', 'dload', 'sinpkt', 'dinpkt', 'sjit', 'djit',
    'tcprtt', 'synack', 'ackdat', 'spkts', 'dpkts', 'sbytes', 'dbytes',
    'sttl', 'dttl', 'sloss', 'dloss', 'swin', 'stcpb', 'dtcpb', 'dwin',
    'smean', 'dmean', 'trans_depth', 'response_body_len', 'ct_srv_src',
    'ct_state_ttl', 'ct_dst_ltm', 'ct_src_dport_ltm', 'ct_dst_sport_ltm',
    'ct_dst_src_ltm', 'ct_ftp_cmd', 'ct_flw_http_mthd', 'ct_src_ltm',
    'ct_srv_dst', 'is_ftp_login', 'is_sm_ips_ports', 'label',
    'proto', 'state', 'attack_category', 'service'
]
# Verify this list has 47 items, matching DATA_STRUCT_FORMAT components and C++ struct

try:
    model = CTGANSynthesizer.load(filepath=SAVED_MODEL_FILEPATH)
    print(f"Python: Successfully loaded CTGAN model from {SAVED_MODEL_FILEPATH}")
except Exception as e:
    print(f"Python: Error loading CTGAN model: {e}")
    # If model loading is critical, exit. Otherwise, you might want to allow mock data generation.
    # For now, we assume it's critical based on previous logs.
    sys.exit(1)

PROTOCOLO_MAP = {member.name.upper(): member.value for member in Protocolo}
STATE_MAP = {member.name.upper(): member.value for member in State}
ATTACK_CAT_MAP = {member.name.upper(): member.value for member in Attack_cat}
SERVICO_MAP = {member.name.upper(): member.value for member in Servico}

def safe_categorical_to_int(value, enum_map, field_name="<unknown_field>", default_value=0):
    """
    Converts a value to its corresponding integer enum value.
    Handles direct integer values, string names (case-insensitive), and enum members.
    If conversion fails or value is None, returns a default value.
    """
    if value is None:
        print(f"Debug Pack [{field_name}]: Input value is None, returning default {default_value}")
        return default_value
    if isinstance(value, (int, float)): 
        return int(value)
    elif isinstance(value, str):
        upper_val = value.upper()
        if upper_val not in enum_map:
            print(f"Debug Pack [{field_name}]: String value '{value}' not in enum_map. Keys: {list(enum_map.keys())}. Returning default {default_value}")
        return enum_map.get(upper_val, default_value)
    elif hasattr(value, 'value') and isinstance(value.value, int): 
        return value.value
    print(f"Debug Pack [{field_name}]: Unexpected type for categorical value '{value}' (type: {type(value)}). Returning default: {default_value}")
    return default_value

def safe_numeric_conversion(value, target_type, field_name="<unknown_field>", default_value=0, min_val=None, max_val=None):
    """
    Safely converts a value to a target numeric type (int, float, bool).
    Handles NaN/infinity for floats, None values, and string representations of numbers.
    Clamps integer values within min_val and max_val if provided.
    If conversion fails, returns the default_value.
    """
    if value is None:
        print(f"Debug Pack [{field_name}]: Input value is None, returning default {default_value}")
        return default_value
    try:
        if isinstance(value, float) and (math.isnan(value) or math.isinf(value)):
            print(f"Debug Pack [{field_name}]: NaN/Inf detected, returning default {default_value}")
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
                converted_value = min_val
            if max_val is not None and converted_value > max_val:
                converted_value = max_val
            return converted_value
        elif target_type == float:
            return float(value)
        elif target_type == bool:
            if isinstance(value, str): 
                return value.lower() in ['true', '1', 't', 'y', 'yes', '1.0']
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
    """
    Generates synthetic data using the loaded CTGAN model.
    Adjusts 'id' column to be sequential starting from current_id_start.
    """
    print(f"Python: Generating {NUM_SYNTHETIC_SAMPLES} synthetic samples...")
    synthetic_data = model.sample(num_rows=NUM_SYNTHETIC_SAMPLES)
    
    if not synthetic_data.empty:
        print(f"Python: Columns in generated DataFrame: {synthetic_data.columns.tolist()}")
    else:
        print("Python: Generated DataFrame is empty!")
        return synthetic_data 

    id_range = range(current_id_start, current_id_start + NUM_SYNTHETIC_SAMPLES)
    synthetic_data["id"] = Series(id_range, index=synthetic_data.index, dtype="int32")
    
    if "id" in synthetic_data.columns:
        cols = ["id"] + [col for col in synthetic_data.columns if col != "id"]
        synthetic_data = synthetic_data[cols]
    print("Python: Synthetic data generation complete.")
    return synthetic_data

def python_pub_publisher():
    """
    Implements a ZeroMQ PUB socket to send structured messages (binary stream)
    representing network traffic data generated by a GAN model to SUB subscribers.
    """
    context = zmq.Context()
    publisher_socket = context.socket(zmq.PUB)
    port = "5556"
    publisher_socket.bind(f"tcp://*:{port}")
    print(f"Python PUB publisher bound to tcp://*:{port}")
    time.sleep(2) 

    current_id_counter = 0
    U8_MAX = 255
    U16_MAX = 65535
        
    first_batch = True

    try:
        while True:
            print("\nPython: --- Starting new batch generation ---")
            synthetic_df = generate_synthetic_data(current_id_counter)
            current_id_counter += NUM_SYNTHETIC_SAMPLES
            
            if synthetic_df.empty:
                print("Python: Generated an empty DataFrame. Skipping sending this batch.")
                time.sleep(1)
                continue

            if first_batch:
                print(f"Python: Verifying DataFrame columns against expected order for packing:")
                df_cols = synthetic_df.columns.tolist()
                if 'id' in df_cols and df_cols[0] != 'id':
                    df_cols.remove('id')
                    df_cols.insert(0, 'id')

                for i, expected_col in enumerate(expected_df_columns_in_order):
                    actual_col = df_cols[i] if i < len(df_cols) else "COLUMN_MISSING_IN_DF"
                    status = "OK" if actual_col == expected_col else f"MISMATCH (DF has: '{actual_col}')"
                    print(f"  Pack item {i+1}: Expected DF col '{expected_col}' -> {status}")
                if len(df_cols) != len(expected_df_columns_in_order):
                    print(f"Python: WARNING! DataFrame has {len(df_cols)} columns, expected {len(expected_df_columns_in_order)} for packing.")
                first_batch = False

            all_packed_data = b""
            sent_ids = []
            rows_successfully_packed = 0

            for index, row in synthetic_df.iterrows():
                print(f"\nPython: Processing row index {index}, ID from df: {row.get('id', 'N/A')}")
                try:
                    data_to_pack = tuple(
                        safe_numeric_conversion(row.get(col_name), int, field_name=col_name) if fmt_char == 'I' and col_name.endswith('bytes') else
                        safe_numeric_conversion(row.get(col_name), int, field_name=col_name, min_val=0) if fmt_char == 'I' else
                        safe_numeric_conversion(row.get(col_name), float, field_name=col_name) if fmt_char == 'f' else
                        safe_numeric_conversion(row.get(col_name), int, field_name=col_name, min_val=0, max_val=U16_MAX) if fmt_char == 'H' else
                        safe_numeric_conversion(row.get(col_name), int, field_name=col_name, min_val=0, max_val=U8_MAX) if fmt_char == 'B' else
                        safe_numeric_conversion(row.get(col_name), bool, field_name=col_name) if fmt_char == '?' else
                        safe_categorical_to_int(row.get(col_name), PROTOCOLO_MAP, field_name=col_name) if col_name == 'proto' else
                        safe_categorical_to_int(row.get(col_name), STATE_MAP, field_name=col_name) if col_name == 'state' else
                        safe_categorical_to_int(row.get(col_name), ATTACK_CAT_MAP, field_name=col_name) if col_name == 'attack_category' else
                        safe_categorical_to_int(row.get(col_name), SERVICO_MAP, field_name=col_name) if col_name == 'service' else
                        row.get(col_name) # Fallback, should not be reached if all types covered
                        for col_name, fmt_char in zip(expected_df_columns_in_order, DATA_STRUCT_FORMAT[1:]) # Skip '<'
                    )
                    
                    # Adjusting the data_to_pack generation to be more explicit and less reliant on complex conditional logic inside tuple comprehension
                    # This makes it easier to debug and verify.
                    
                    _data_values = []
                    # id (I)
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[0]), int, field_name=expected_df_columns_in_order[0]))
                    # 11 floats (dur to ackdat)
                    for i in range(1, 12): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), float, field_name=expected_df_columns_in_order[i]))
                    # spkts, dpkts (2H)
                    for i in range(12, 14): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    # sbytes, dbytes (2I)
                    for i in range(14, 16): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0))
                    # sttl, dttl (2B)
                    for i in range(16, 18): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U8_MAX))
                    # sloss, dloss (2H)
                    for i in range(18, 20): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    # swin (H)
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[20]), int, field_name=expected_df_columns_in_order[20], min_val=0, max_val=U16_MAX))
                    # stcpb, dtcpb (2I)
                    for i in range(21, 23): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0))
                    # dwin (H)
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[23]), int, field_name=expected_df_columns_in_order[23], min_val=0, max_val=U16_MAX))
                    # smean, dmean (2H)
                    for i in range(24, 26): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    # trans_depth (H)
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[26]), int, field_name=expected_df_columns_in_order[26], min_val=0, max_val=U16_MAX))
                    # response_body_len (I)
                    _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[27]), int, field_name=expected_df_columns_in_order[27], min_val=0))
                    # 6 H's (ct_srv_src to ct_dst_src_ltm)
                    for i in range(28, 34): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    # 4 H's (ct_ftp_cmd to ct_srv_dst)
                    for i in range(34, 38): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), int, field_name=expected_df_columns_in_order[i], min_val=0, max_val=U16_MAX))
                    # 3 ?'s (booleans)
                    for i in range(38, 41): _data_values.append(safe_numeric_conversion(row.get(expected_df_columns_in_order[i]), bool, field_name=expected_df_columns_in_order[i]))
                    # 4 B's (enums)
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[41]), PROTOCOLO_MAP, field_name=expected_df_columns_in_order[41]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[42]), STATE_MAP, field_name=expected_df_columns_in_order[42]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[43]), ATTACK_CAT_MAP, field_name=expected_df_columns_in_order[43]))
                    _data_values.append(safe_categorical_to_int(row.get(expected_df_columns_in_order[44]), SERVICO_MAP, field_name=expected_df_columns_in_order[44]))
                    
                    data_to_pack = tuple(_data_values)


                    if len(data_to_pack) != len(expected_df_columns_in_order):
                        print(f"Python: CRITICAL PACKING ERROR for row index {index}, ID {row.get('id', 'N/A')}: Generated data_to_pack has {len(data_to_pack)} items, but expected {len(expected_df_columns_in_order)}. Skipping row.")
                        # print(f"Data tuple that was prepared: {data_to_pack}")
                        continue

                    packed_data = DATA_STRUCT.pack(*data_to_pack)
                    all_packed_data += packed_data
                    sent_ids.append(row.get('id'))
                    rows_successfully_packed += 1

                except struct.error as se:
                    print(f"Python: Struct packing error for row index {index}, ID {row.get('id', 'N/A')}: {se}")
                    continue 
                except Exception as ex:
                    print(f"Python: General error processing row index {index}, ID {row.get('id', 'N/A')}: {ex}")
                    continue
            
            print(f"Python: Finished processing batch. {rows_successfully_packed} rows successfully packed.")

            if all_packed_data:
                topic = b"data_batch"
                full_message = topic + b" " + all_packed_data
                print(f"Python: Sending full_message. Topic size: {len(topic)}, Data size: {len(all_packed_data)}, Total ZMQ message size: {len(full_message)}")
                publisher_socket.send(full_message)
                if sent_ids:
                    print(f"Python: Sent batch of {len(sent_ids)} Data structs (IDs: {sent_ids[0]} to {sent_ids[-1]})")
                else: 
                    print(f"Python: all_packed_data is not empty, but sent_ids is. This is unexpected.")
            else:
                print("Python: No data was packed in this iteration (all rows might have failed, or NUM_SYNTHETIC_SAMPLES is 0).")

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
    EXPECTED_FIELD_COUNT = 47 
    EXPECTED_STRUCT_SIZE_BYTES = 115 # Recalculated based on the format string: <I11f2H2I2B2H1H2I1H2H1H1I6H4H3?4B

    if len(expected_df_columns_in_order) != EXPECTED_FIELD_COUNT:
         print(f"CRITICAL PYTHON SETUP ERROR: 'expected_df_columns_in_order' has {len(expected_df_columns_in_order)} items, but expected {EXPECTED_FIELD_COUNT}. Please verify this list against your C++ struct and DATA_STRUCT_FORMAT.")
    if DATA_STRUCT.size != EXPECTED_STRUCT_SIZE_BYTES:
         print(f"CRITICAL PYTHON SETUP ERROR: DATA_STRUCT.size is {DATA_STRUCT.size}, but expected {EXPECTED_STRUCT_SIZE_BYTES}. Your DATA_STRUCT_FORMAT is: '{DATA_STRUCT_FORMAT}'. Please verify this against C++ sizeof(Data) EXACTLY.")
    
    python_pub_publisher()

