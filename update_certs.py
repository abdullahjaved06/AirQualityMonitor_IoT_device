import os
import re

# Path setup
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CERTS_DIR = os.path.join(BASE_DIR, "components", "certificates")
CERTS_C_PATH = os.path.join(CERTS_DIR, "certificates.c")

# Map cert files to variable names
CERT_FILES = {
    "certificate.pem.crt": "device_cert",
    "private.pem.key": "private_key",
    "AmazonRootCA1.pem": "root_ca"
}

def convert_cert_to_c_format(file_path, var_name):
    with open(file_path, "r") as f:
        lines = f.readlines()
    lines = [line.strip() for line in lines if line.strip()]
    lines = ['"{}\\n"'.format(line.replace('"', '\\"')) for line in lines]
    return f'static const char {var_name}[] =\n' + '\n'.join(lines) + ';\n\n'

def update_certs_file(original_content, cert_blocks):
    # Ensure #include stays at the top and alone
    lines = original_content.splitlines()
    preserved_lines = []
    include_line = None

    for line in lines:
        if line.strip().startswith('#include') and 'certificates.h' in line:
            include_line = line
        elif not re.match(r'static const char (device_cert|private_key|root_ca)\[\]', line):
            preserved_lines.append(line)

    updated = ""
    if include_line:
        updated += include_line + "\n\n"
    updated += '\n'.join(preserved_lines).strip() + '\n\n'

    for block in cert_blocks.values():
        updated += block

    return updated


def main():
    # Read original certs.c
    if not os.path.exists(CERTS_C_PATH):
        print("certificates.c not found!")
        return

    with open(CERTS_C_PATH, "r") as f:
        original = f.read()

    cert_blocks = {}
    for filename, var_name in CERT_FILES.items():
        file_path = os.path.join(CERTS_DIR, filename)
        if os.path.exists(file_path):
            cert_blocks[var_name] = convert_cert_to_c_format(file_path, var_name)
        else:
            print(f"Warning: {filename} not found.")

    updated = update_certs_file(original, cert_blocks)

    with open(CERTS_C_PATH, "w") as f:
        f.write(updated)

    print("certificates.c has been updated successfully.")

if __name__ == "__main__":
    main()
