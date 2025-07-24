import os
import sys
import struct
from pytablereader import TableFileLoader


def to_bin(input_md, output_bin):
    # Read markdown table using pytablereader TableFileLoader
    try:
        loader = TableFileLoader(input_md, format_name="markdown")
        tables = list(loader.load())
    except Exception as e:
        print(f"Error reading markdown file: {e}")
        sys.exit(1)

    if not tables:
        print("No tables found in markdown file.")
        sys.exit(1)

    table = tables[0]
    header = [h.strip().lower() for h in table.headers]
    try:
        romid_idx = header.index('rom-id')
        name_idx = header.index('name')
    except ValueError:
        print("Table must contain 'rom-id' and 'name' columns.")
        sys.exit(1)

    output_dir = os.path.dirname(os.path.abspath(output_bin))
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(output_bin, 'wb') as fout:
        # first record is special, it has 64 bytes.
        # - first 2 bytes are amount of records (uint16, little-endian)
        # - rest of bytes is 62 bytes of 0x00
        fout.write(struct.pack('<H', len(table.rows)))
        fout.write(b'\x00' * 62)

        for row in table.rows:
            romid_str = str(row[romid_idx]).strip()
            # Accept hex with dashes or 0x, or decimal
            if '-' in romid_str:
                # Convert dash-separated hex bytes to int
                romid = int(romid_str.replace('-', ''), 16)
            elif romid_str.lower().startswith('0x'):
                romid = int(romid_str, 16)
            else:
                romid = int(romid_str)

            name = str(row[name_idx]).encode('utf-8')
            name_bin = name[:56].ljust(56, b'\0')

            fout.write(struct.pack('<Q', romid))
            fout.write(name_bin)

def to_md(input_bin, output_md):
    with open(input_bin, 'rb') as fin:
        header = fin.read(64)
        if len(header) < 64:
            print("Binary file too short.")
            sys.exit(1)
        n_records = struct.unpack('<H', header[0:2])[0]
        rows = []
        for _ in range(n_records):
            data = fin.read(64)
            if len(data) < 64:
                print("Unexpected end of file.")
                sys.exit(1)
            romid = struct.unpack('<Q', data[:8])[0]
            name = data[8:64].rstrip(b'\0').decode('utf-8', errors='replace')
            # Format romid as dash-separated hex bytes
            romid_hex = f'{romid:016X}'
            romid_str = '-'.join([romid_hex[i:i+2] for i in range(0, 16, 2)])
            rows.append((romid_str, name))

    # Write markdown table
    if not os.path.exists(os.path.dirname(output_md)):
        os.makedirs(os.path.dirname(output_md))

    with open(output_md, 'w', encoding='utf-8') as fout:
        fout.write('| rom-id             | name       |\n')
        fout.write('|--------------------|------------|\n')
        for romid, name in rows:
            fout.write(f'| {romid:<18} | {name:<10} |\n')

def main():
    if len(sys.argv) < 4:
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

if __name__ == "__main__":
    main()
