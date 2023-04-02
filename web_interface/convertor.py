import os
import glob
import gzip

allowed_suffix = [".html", ".css", ".js"]

path = input("Enter directory path: ")

while not os.path.exists(path):
    print("Directory path doesn't exist!")
    path = input("Enter directory path: ")

output_dir = os.path.join(path, 'output')
files = os.listdir(path)
if any(file.endswith(".gzip") for file in files): 
    while True:
        confirm_rem_input = input("Do you want to delete old gzip files(y/n): ")
        if confirm_rem_input.lower() in ['yes', 'y']:
            gz_files = glob.glob(os.path.join(path, "*.gzip"))
            for gfile in gz_files:
                os.remove(gfile)
            files = os.listdir() #refresh files content
            print("All files with .gzip suffix deleted.")
            break
        elif confirm_rem_input.lower() in ['no', 'n']:
            break
        else:
            print("Invalid input. Please enter 'y', 'yes' or 'n', 'no'")

if not os.path.isdir(output_dir):
    os.mkdir(output_dir)

web_files_content = ""
for file in files:
    output_file =  f"{os.path.join(output_dir, file)}.gz"
    file = os.path.join(path, file)
    if os.path.isfile(file) and os.path.splitext(file)[1].lower() in allowed_suffix:   
        print(f"{os.path.basename(output_file)} Created")
        with open(file, 'rb') as f_input:
            with gzip.open(output_file, 'wb') as f_output:
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


print('Down.')