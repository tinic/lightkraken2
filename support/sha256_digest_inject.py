import hashlib
import os
import sys

SHA256_DIGEST_SIZE = 32

ELF_MAGIC_HEX_STRING = '3412D51E2143D51E'
ELF_MAGIC_OFFSET = 16

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

def calculate_sha256(file_path, start_offset, end_offset):
    sha256 = hashlib.sha256()
    with open(file_path, "rb") as file:
        file.seek(start_offset)
        content = file.read(end_offset - start_offset)
        sha256.update(content)
    return sha256.digest()

def inject_digest_into_bin_file(file_path, digest):
    with open(file_path, "r+b") as file:
        file.seek(-SHA256_DIGEST_SIZE, os.SEEK_END) 
        file.write(digest)

def inject_digest_into_elf_file(file_path, digest):
    second_instance_index = find_second_instance(file_path, ELF_MAGIC_HEX_STRING)
    if second_instance_index:
        with open(file_path, "r+b") as file:
            file.seek(second_instance_index + ELF_MAGIC_OFFSET, os.SEEK_SET) 
            file.write(digest)

def inject(file_path_bin, file_path_elf):
    sha256_digest = calculate_sha256(file_path_bin, 0, os.path.getsize(file_path_bin) - SHA256_DIGEST_SIZE)
    inject_digest_into_bin_file(file_path_bin, sha256_digest)
    inject_digest_into_elf_file(file_path_elf, sha256_digest)

def read_sha256_digest_from_bin_file(file_path):
    with open(file_path, "rb") as binary_file:
        binary_file.seek(-SHA256_DIGEST_SIZE, os.SEEK_END)
        digest = binary_file.read(SHA256_DIGEST_SIZE)
    return digest

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python3 sha256.py <firmware.bin> <firmware.elf>")
        sys.exit(1)

    if sys.version_info[0] < 3:
        raise Exception("Must be using Python 3 or newer")
    else:
        print("You are running Python {}.".format(sys.version_info[0]))
    
    file_path_bin = sys.argv[1]
    file_path_elf = sys.argv[2]

    inject(file_path_bin, file_path_elf)

    print("SHA256 Digest:", read_sha256_digest_from_bin_file(file_path_bin).hex())
