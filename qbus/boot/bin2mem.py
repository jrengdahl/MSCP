import sys

def bin_to_mem(bin_file, mem_file, word_size=16):
    with open(bin_file, "rb") as binf, open(mem_file, "w") as memf:
        word_bytes = word_size // 8
        while True:
            bytes_read = binf.read(word_bytes)
            if not bytes_read:
                break
            # Ensure we have the correct number of bytes, pad with zeros if necessary
            while len(bytes_read) < word_bytes:
                bytes_read += b'\x00'
            # Convert to integer assuming little-endian order
            word = int.from_bytes(bytes_read, byteorder='little')
            # Write to mem file in hex format
            memf.write(f"{word:0{word_size // 4}X}\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: bin_to_mem.py input.bin output.mem")
    else:
        bin_to_mem(sys.argv[1], sys.argv[2])

