#!/usr/bin/env python3
import os
import shutil

VERSION = "1.0"
dest_install = '/home/sees/Work/Sees/QGroundControl'

# Define destination directories
destinations = [
    '/home/sees/.local/share/applications',
    dest_install,
    '/home/sees/.config/QGroundControl.org'
]

print(f"Script version {VERSION} starting installation. ")
os.system("pip install python-dotenv")

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
shutil.copy2('QGroundControl.png', '/home/sees/.local/share/applications')
shutil.copy2('QGroundControl.desktop', '/home/sees/.local/share/applications')
shutil.copy2('QGroundControl-v4.3.0-0.0.3.AppImage', dest_install)
shutil.copy2('qgroundcontrol_start.sh', dest_install)
if not os.path.exists(os.path.join(dest_install,".env")):
	print("No .env file available, copying template one. Please fill in the github API key")
	shutil.copy2('.env', dest_install)
else:
	print(".env file exists, won't overwrite it")
shutil.copy2('seesai-logo-narrow.jpg', dest_install)
if not os.path.exists('/home/sees/.config/QGroundControl.org/QGroundControl Daily.ini'):
	print("No .ini file available, copying template one")
	shutil.copy2('QGroundControl Daily.ini', '/home/sees/.config/QGroundControl.org')
else:
	print(".ini file exists, renaming it as .old")
	shutil.move('/home/sees/.config/QGroundControl.org/QGroundControl Daily.ini', '/home/sees/.config/QGroundControl.org/QGroundControl Daily.old')
	shutil.copy2('QGroundControl Daily.ini', '/home/sees/.config/QGroundControl.org')

print("Files copied successfully!")



