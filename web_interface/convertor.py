import os
import glob
import gzip

allowed_suffix = [".html", ".css", ".js"]

msg = "Enter directory path(D= Current Path): "
path = input(msg)

if path.lower() == 'd':
    path = os.path.dirname(os.path.realpath(__file__))
else:
    while not os.path.exists(path):
        print("Directory path doesn't exist!")
        path = input(msg)

output_dir = os.path.join(path, 'output')
os.makedirs(output_dir, exist_ok=True)

files = os.listdir(path)
if any(file.endswith(".gz") for file in os.listdir(output_dir)): 
    while True:
        confirm_rem_input = input("Do you want to delete old gzip files(y/n): ")
        if confirm_rem_input.lower() in ['yes', 'y']:
            gz_files = glob.glob(os.path.join(output_dir, "*.gz"))

            for gfile in gz_files:
                os.remove(gfile)
            len_gz_files = len(gz_files)
            print(f"{len_gz_files} {'file' if len_gz_files == 1 else 'files'} with .gz suffix deleted")
            break
        elif confirm_rem_input.lower() in ['no', 'n']:
            break
        else:
            print("Invalid input. Please enter 'y', 'yes' or 'n', 'no'")

web_files_content = ""
for file in os.listdir(path):
    if file.endswith(tuple(allowed_suffix)):
        input_file = os.path.join(path, file)
        output_file = os.path.join(output_dir, f"{file}.gz")
        print(f"{os.path.basename(output_file)} Created")
        with open(input_file, 'rb') as f_input, gzip.open(output_file, 'wb') as f_output:
            f_output.writelines(f_input)
        with open(output_file, 'rb') as f_output:
            data = f_output.read()
            byte_array = bytearray(data)
            hex_array = [hex(b) for b in byte_array]
            arrayName = os.path.basename(output_file).replace('.','_')
            web_files_content += "const uint8_t "+arrayName+"[] PROGMEM = { " + ', '.join(hex_array) + " };\n"
            web_files_content += f"const int {arrayName}_len = sizeof({arrayName});\n"
            web_files_content += f"const String {arrayName.replace('_gz', '')}_route = \"/{os.path.basename(output_file)}\";\n\n"


if len(web_files_content) != 0:
    with open(os.path.join(output_dir, 'webfiles.h'), 'w') as f_output:
        f_output.write("#ifndef webfiles_h\n")
        f_output.write("#define webfiles_h\n\n")
        f_output.write(web_files_content)
        f_output.write("#endif")


print('Done.')