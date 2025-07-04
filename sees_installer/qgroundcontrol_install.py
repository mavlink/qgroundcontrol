#!/usr/bin/env python3
import os
import shutil

VERSION = "1.2"
dest_qgc_start = '/opt/sees/apps/qgroundcontrol'
dest_qgc_desktop_shortcut = '/home/sees/.local/share/applications'
dest_qgc_ini = '/home/sees/.config/QGroundControl.org'

# Define destination directories
destinations = [
    dest_qgc_desktop_shortcut,
    dest_qgc_start,
    dest_qgc_ini
]

print(f"Script version {VERSION} starting installation. ")
os.system("pip install python-dotenv")
os.system("pip install pyqt5")
os.system("pip install ruamel.yaml")

from ruamel.yaml import YAML

# Check if directories exist and create if user confirms
for directory in destinations:
    if not os.path.exists(directory):
        response = input(f"Directory '{directory}' doesn't exist. Create it? (y/n): ")
        if response.lower() == 'y':
            os.makedirs(directory)
            print(f"Created directory: {directory}")
        else:
            print(f"Skipping creation of directory: {directory}")

# Copy the files
shutil.copy2('QGroundControl.png', dest_qgc_desktop_shortcut)
shutil.copy2('QGroundControl.desktop', dest_qgc_desktop_shortcut)
shutil.copy2('QGroundControl-v4.3.0-0.0.3.AppImage', dest_qgc_start)
shutil.copy2('qgroundcontrol_start.py', dest_qgc_start)

config_file = "qgroundcontrol_start.yaml"
yaml = YAML()
yaml.preserve_quotes = True  # preserves quotes in strings

with open(config_file, 'r') as file:
    config = yaml.load(file)
machine_types = config["QGC_INI"]["LOCAL_MACHINE_TYPES"]
machine_type = input(f"What is this machine going to be running as {machine_types}?")

if machine_type in machine_types:
    config["QGC_INI"]["LOCAL_MACHINE"] = machine_type
    with open(config_file, 'w') as file:
        yaml.dump(config, file)
else:
    Exception("Machine type not recognised.")

shutil.copy2(config_file, dest_qgc_start)
if not os.path.exists(os.path.join(dest_qgc_start, ".env")):
    print("No .env file available, copying template one. Please fill in the github API key")
    shutil.copy2('.env', dest_qgc_start)
else:
    print(".env file exists, won't overwrite it")
shutil.copy2('seesai-logo-narrow.jpg', dest_qgc_start)
if not os.path.exists(f"{dest_qgc_ini}/QGroundControl Daily.ini"):
    print("No .ini file available, copying template one")
    shutil.copy2('QGroundControl Daily.ini', f"{dest_qgc_ini}/QGroundControl Daily.ini")
else:
    print(".ini file exists, renaming it as .old")
    shutil.move(f"{dest_qgc_ini}/QGroundControl Daily.ini", f'{dest_qgc_ini}/QGroundControl Daily.old')
    shutil.copy2('QGroundControl Daily.ini', dest_qgc_ini)

print("Files copied successfully!")



