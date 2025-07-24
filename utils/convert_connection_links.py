import os
import sys
import struct
from pytablereader import TableFileLoader


def to_bin(input_md, output_bin):
    # Read markdown tables using pytablereader TableFileLoader
    try:
        loader = TableFileLoader(input_md, format_name="markdown")
        tables = list(loader.load())
    except Exception as e:
        print(f"Error reading markdown file: {e}")
        sys.exit(1)

    if len(tables) < 2:
        print("Expected at least two tables in the markdown file.")
        sys.exit(1)

    # First table: value mapping
    value_table = tables[0]
    value_header = [h.strip().lower() for h in value_table.headers]
    try:
        value_idx = value_header.index('value')
        command_idx = value_header.index('commands')
        states_idx = value_header.index('states')
        actions_idx = value_header.index('actions')
    except ValueError:
        print("First table must contain 'Commands', 'States', 'Actions', and 'Value' columns.")
        sys.exit(1)

    value_map = {}
    allowed_commands = set()
    allowed_states = set()
    allowed_actions = set()
    for row in value_table.rows:
        # Each row may have up to 3 fields before Value
        cmd = str(row[command_idx]).strip()
        state = str(row[states_idx]).strip()
        action = str(row[actions_idx]).strip()
        value = int(row[value_idx])
        if cmd:
            allowed_commands.add(cmd)
        if state:
            allowed_states.add(state)
        if action:
            allowed_actions.add(action)
        # Map all non-empty fields to value
        if cmd:
            value_map[cmd] = value
        if state:
            value_map[state] = value
        if action:
            value_map[action] = value

    # Second table: data
    data_table = tables[1]
    data_header = [h.strip() for h in data_table.headers]
    n_fields = 8
    if len(data_header) < n_fields:
        print(f"Second table must have at least {n_fields} fields.")
        sys.exit(1)

    # Indices for IN/OUT fields
    in_cmd_idx = data_header.index('IN Commands') if 'IN Commands' in data_header else None
    in_state_idx = data_header.index('IN States') if 'IN States' in data_header else None
    in_action_idx = data_header.index('IN Actions') if 'IN Actions' in data_header else None
    in_value_idx = data_header.index('IN Value') if 'IN Value' in data_header else None
    out_cmd_idx = data_header.index('OUT Cmd') if 'OUT Cmd' in data_header else None
    out_state_idx = data_header.index('OUT States') if 'OUT States' in data_header else None
    out_action_idx = data_header.index('OUT Actions') if 'OUT Actions' in data_header else None
    out_value_idx = data_header.index('OUT Value') if 'OUT Value' in data_header else None

    output_dir = os.path.dirname(os.path.abspath(output_bin))
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(output_bin, 'wb') as fout:
        value_table_len = len(value_table.rows)
        data_table_len = len(data_table.rows)
        # Write header: first 2 bytes are value_table_len (uint16 little-endian), next 2 bytes are data_table_len (uint16 little-endian), rest zeros to 64 bytes
        header = bytearray(struct.pack('<H', value_table_len) + struct.pack('<H', data_table_len))
        header.extend(b'\x00' * (64 - len(header)))
        fout.write(header)

        # Write value_table records (each 64 bytes)
        for row in value_table.rows:
            for idx in (command_idx, states_idx, actions_idx):
                s = str(row[idx]).strip() if idx is not None else ''
                s_bytes = s.encode('utf-8')[:15] + b'\x00'  # up to 15 bytes + null
                s_bytes = s_bytes.ljust(16, b'\x00')
                fout.write(s_bytes)
            value = int(row[value_idx])
            fout.write(struct.pack('<i', value))
            # Pad to 64 bytes
            pad_len = 64 - (16*3 + 4)
            fout.write(b'\x00' * pad_len)

        # Write data_table records (each 64 bytes)
        for row in data_table.rows:
            # Validate IN/OUT Commands, States, Actions
            if in_cmd_idx is not None:
                in_cmd = str(row[in_cmd_idx]).strip()
                if in_cmd and in_cmd not in allowed_commands:
                    print(f"Invalid IN Command: {in_cmd}")
                    sys.exit(1)
            if out_cmd_idx is not None:
                out_cmd = str(row[out_cmd_idx]).strip()
                if out_cmd and out_cmd not in allowed_commands:
                    print(f"Invalid OUT Command: {out_cmd}")
                    sys.exit(1)
            if in_state_idx is not None:
                in_state = str(row[in_state_idx]).strip()
                if in_state and in_state not in allowed_states:
                    print(f"Invalid IN State: {in_state}")
                    sys.exit(1)
            if out_state_idx is not None:
                out_state = str(row[out_state_idx]).strip()
                if out_state and out_state not in allowed_states:
                    print(f"Invalid OUT State: {out_state}")
                    sys.exit(1)
            if in_action_idx is not None:
                in_action = str(row[in_action_idx]).strip()
                if in_action and in_action not in allowed_actions:
                    print(f"Invalid IN Action: {in_action}")
                    sys.exit(1)
            if out_action_idx is not None:
                out_action = str(row[out_action_idx]).strip()
                if out_action and out_action not in allowed_actions:
                    print(f"Invalid OUT Action: {out_action}")
                    sys.exit(1)

            # Write 8 fields: 3 IN, 1 IN Value (int32), 3 OUT, 1 OUT Value (int32)
            record = bytearray()
            # IN fields
            for idx in (in_cmd_idx, in_state_idx, in_action_idx):
                name = str(row[idx]).strip() if idx is not None else ''
                val = value_map.get(name, 0)
                record.append(val & 0xFF)
            # IN Value (int32)
            if in_value_idx is not None:
                in_val = int(row[in_value_idx])
                record.extend(struct.pack('<i', in_val))
            else:
                record.extend(struct.pack('<i', 0))
            # OUT fields
            for idx in (out_cmd_idx, out_state_idx, out_action_idx):
                name = str(row[idx]).strip() if idx is not None else ''
                val = value_map.get(name, 0)
                record.append(val & 0xFF)
            # OUT Value (int32)
            if out_value_idx is not None:
                out_val = int(row[out_value_idx])
                record.extend(struct.pack('<i', out_val))
            else:
                record.extend(struct.pack('<i', 0))
            # Pad record to 64 bytes
            if len(record) < 64:
                record.extend(b'\x00' * (64 - len(record)))
            fout.write(record)

def to_md(input_bin, output_md):
    with open(input_bin, 'rb') as fin:
        header = fin.read(64)
        if len(header) < 64:
            print("Binary file too short.")
            sys.exit(1)
        value_table_len = struct.unpack('<H', header[0:2])[0]
        data_table_len = struct.unpack('<H', header[2:4])[0]
        # Read value_table records
        value_rows = []
        cmd_dict = {}
        state_dict = {}
        action_dict = {}
        for _ in range(value_table_len):
            vdata = fin.read(64)
            if len(vdata) < 64:
                print("Unexpected end of file in value_table.")
                sys.exit(1)
            cmd = vdata[0:16].split(b'\x00', 1)[0].decode('utf-8', errors='replace')
            state = vdata[16:32].split(b'\x00', 1)[0].decode('utf-8', errors='replace')
            action = vdata[32:48].split(b'\x00', 1)[0].decode('utf-8', errors='replace')
            value = struct.unpack('<i', vdata[48:52])[0]
            value_rows.append((cmd, state, action, value))
            cmd_dict[value] = cmd
            state_dict[value] = state
            action_dict[value] = action
        # Read data_table records
        rows = []
        for _ in range(data_table_len):
            data = fin.read(64)
            if len(data) < 64:
                print("Unexpected end of file in data_table.")
                sys.exit(1)
            in_cmd = data[0]
            in_state = data[1]
            in_action = data[2]
            in_value = struct.unpack('<i', data[3:7])[0]
            out_cmd = data[8]
            out_state = data[9]
            out_action = data[10]
            out_value = struct.unpack('<i', data[11:15])[0]
            print(f"Read row: {in_cmd}, {in_state}, {in_action}, {in_value}, {out_cmd}, {out_state}, {out_action}, {out_value}")
            rows.append((in_cmd, in_state, in_action, in_value, out_cmd, out_state, out_action, out_value))

    # Write markdown table with correct headers
    value_headers = ['Commands', 'States', 'Actions', 'Value']
    headers = ['IN Commands', 'IN States', 'IN Actions', 'IN Value', 'OUT Cmd', 'OUT States', 'OUT Actions', 'OUT Value']

    if not os.path.exists(os.path.dirname(output_md)):
        os.makedirs(os.path.dirname(output_md))

    with open(output_md, 'w', encoding='utf-8') as fout:
        # First table: value_table, columns 16 characters wide
        fout.write('## Value Table\n\n')
        fout.write('| ' + ' | '.join(f"{h:<14}" for h in value_headers) + ' |\n')
        fout.write('|' + '|'.join(['-' * 16] * len(value_headers)) + '|\n')
        for row in value_rows:
            fout.write('| ' + ' | '.join(f"{str(v):<14}" for v in row) + ' |\n')
        fout.write('\n')
        # Second table: data_table
        fout.write('## Data Table\n\n')
        headers = ['IN Commands', 'IN States     ', 'IN Actions', 'IN Value', 'OUT Cmd   ', 'OUT States    ', 'OUT Actions', 'OUT Value']
        fout.write('| ' + ' | '.join(headers) + ' |\n')
        fout.write('|' + '|'.join(['-' * (len(header)+2) for header in headers]) + '|\n')
        for row in rows:
            print(row)
            fout.write(f'| {cmd_dict[row[0]]:<11} | {state_dict[row[1]]:<14} | {action_dict[row[2]]:<10} | {str(row[3]):<8} | {cmd_dict[row[4]]:<10} | {state_dict[row[5]]:<14} | {action_dict[row[6]]:<11} | {str(row[7]):<9} |\n')



if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <to-bin|to-md> <input_file> <output_file>")
        sys.exit(1)

    mode = sys.argv[1]
    input_file = sys.argv[2]
    output_file = sys.argv[3]

    if mode == 'to-bin':
        to_bin(input_file, output_file)
    elif mode == 'to-md':
        to_md(input_file, output_file)
    else:
        print(f"Unknown mode: {mode}")
        sys.exit(1)