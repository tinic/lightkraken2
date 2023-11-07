import hashlib
import os
import sys

def find_second_instance(file_path, hex_string):
    search_bytes = bytes.fromhex(hex_string)
    with open(file_path, 'rb') as file:
        content = file.read()
        first_index = content.find(search_bytes)
        if first_index == -1:
            return None
        second_index = content.find(search_bytes, first_index + len(search_bytes))
        if second_index == -1:
            return None
        return second_index

def read_sha256_digest(file_path):
    with open(file_path, "rb") as binary_file:
        binary_file.seek(-32, os.SEEK_END)
        digest = binary_file.read(32)
    
    return digest

def calculate_sha256(file_path, start_offset, end_offset):
    sha256 = hashlib.sha256()
    
    with open(file_path, "rb") as file:
        file.seek(start_offset)
        content = file.read(end_offset - start_offset)
        sha256.update(content)

    return sha256.digest()

def inject_digest_into_file(file_path, digest):
    with open(file_path, "r+b") as file:
        file.seek(-32, os.SEEK_END) 
        file.write(digest)

def inject_digest_into_elf_file(file_path, digest):
    second_instance_index = find_second_instance(file_elf_path_to_modify, '3412D51E2143D51E')
    if second_instance_index:
        with open(file_elf_path_to_modify, "r+b") as file:
            file.seek(second_instance_index + 16, os.SEEK_SET) 
            file.write(digest)

def main(file_path, file_path_elf, start_offset, end_offset):
    sha256_digest = calculate_sha256(file_path, start_offset, end_offset)
    inject_digest_into_file(file_path, sha256_digest)
    inject_digest_into_elf_file(file_path_elf, sha256_digest)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python3 sha256.py <firmware.bin> <firmware.elf>")
        sys.exit(1)
    
    file_path_to_modify = sys.argv[1]
    file_elf_path_to_modify = sys.argv[2]
    file_size = os.path.getsize(file_path_to_modify)

    main(file_path_to_modify, file_elf_path_to_modify, 0, file_size - 32)

    digest = read_sha256_digest(file_path_to_modify)
    print("SHA256 Digest:", digest.hex())
