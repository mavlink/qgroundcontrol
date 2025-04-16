#!/usr/bin/env python3
import os
import shutil

# Define destination directories
destinations = [
    '/home/sees/.local/share/applications',
    '/home/sees/Work/Sees/QGroundControl',
    '/home/sees/.config/QGroundControl.org'
]

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
shutil.copy2('QGroundControl-v4.3.0-0.0.3.AppImage', '/home/sees/Work/Sees/QGroundControl')
shutil.copy2('qgroundcontrol_start.sh', '/home/sees/Work/Sees/QGroundControl')
shutil.copy2('QGroundControl Daily.ini', '/home/sees/.config/QGroundControl.org')


print("Files copied successfully!")
