#!/usr/bin/env python3
import requests
from dotenv import load_dotenv
import os
import argparse
import difflib
import subprocess
from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout, QHBoxLayout, QPushButton, QScrollArea
from PyQt5.QtCore import Qt
import sys
import time
import configparser
VERSION = 1.0

#FILE SECTIONS = ['LinkManager', 'Video', 'General', 'MainWindowState', 'JoystickManager', 'QGC_MAVLINK_PROTOCOL', 'Units', 'DEFAULT', 'FlightMapPosition', 'QGCQml', 'Vehicle14', 'LinkConfigurations', 'MAVLinkLogGroup', 'Vehicle12', 'LoggingFilters', 'Vehicle15', 'Branding', 'Joysticks', 'FlyView', 'TelemetryBarUserSettings-2', 'RadioCalibration']
CONFIG_CRITICAL_SECTIONS = ['Joysticks', 'Video','TelemetryBarUserSettings-2', 'JoystickManager', 'LinkManager' ]
IGNORE_PARAMS = ["Xbox%20Series%20X%20Controller\Axis"]

class QGCGitHub():
    def __init__(self, branch):
        GITHUB_TOKEN = os.getenv("GITHUB_TOKEN", "")  # This is fine-grained personal access token (whatever that means)
        self.branch = branch
        # Create headers with authentication
        self.headers = {
            'Authorization': f'token {GITHUB_TOKEN}',
            'Accept': 'application/vnd.github.v3.raw'
        }

    def download_file(self,filepath):
        url = f'https://api.github.com/repos/SEESAI/qgroundcontrol/contents/{filepath}?ref={self.branch}'
        # Download the file
        response = requests.get(url, headers=self.headers)
        response.raise_for_status()  # Check for errors
        return(response.content)


class ProgressWindow:
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.continue_execution = False
        self.abort_execution = False

        # Create window
        self.window = QWidget()
        self.window.setWindowTitle(f"QGC config script checker v{VERSION}")
        self.window.setGeometry(100, 100, 400, 200)

        # Create layout
        layoutV = QVBoxLayout()

        # Create message label
        self.message_label = QLabel("Checking QGroundControl config file...")
        self.message_label.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        self.message_label.setWordWrap(True)  # Enable word wrapping

        # Create a scroll area
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)  # Important!
        scroll_area.setWidget(self.message_label)  # Set the label as the widget inside scroll area

        layoutV.addWidget(scroll_area)

        layoutH = QHBoxLayout()

        # Create continue button
        self.continue_button = QPushButton("Continue")
        self.continue_button.clicked.connect(self.on_continue_clicked)
        self.continue_button.setVisible(False)  # Hidden by default
        layoutH.addWidget(self.continue_button)

        # Create abort button
        self.abort_button = QPushButton("Abort")
        self.abort_button.clicked.connect(self.on_abort_clicked)
        self.abort_button.setVisible(False)  # Hidden by default
        layoutH.addWidget(self.abort_button)

        # Set layout and show
        layoutV.addLayout(layoutH)
        self.window.setLayout(layoutV)
        self.window.show()
        self.app.processEvents()

    def update_message(self, message, align = Qt.AlignCenter):
        """Update the displayed message"""
        self.message_label.setText(message)
        self.message_label.setAlignment( align | Qt.AlignVCenter)
        self.app.processEvents()  # Process UI events to update display

    def wait_for_user(self, message="Press Continue to proceed"):
        """Show a button and wait for user to click it before continuing"""
        self.update_message(message, align=Qt.AlignLeft)
        self.continue_execution = False
        self.abort_execution = False
        self.continue_button.setVisible(True)
        self.abort_button.setVisible(True)
        self.app.processEvents()

        # Wait until the button is clicked
        while not self.continue_execution and not self.abort_execution:
            self.app.processEvents()
            time.sleep(0.1)  # Small sleep to prevent high CPU usage

        # Hide the button again
        self.continue_button.setVisible(False)
        self.abort_button.setVisible(False)

    def on_continue_clicked(self):
        """Called when the continue button is clicked"""
        self.continue_execution = True

    def on_abort_clicked(self):
        """Called when the continue button is clicked"""
        self.abort_execution = True


    def run(self):
        """Start the event loop"""
        return self.app.exec_()

def section_to_text(config, sections):
    result = []
    for section_name in sections:
        # Create section header
        result += [f"[{section_name}]\n"]

        # Get section as a dictionary and add each key-value pair
        section_dict = dict(config[section_name])
        for key, value in section_dict.items():
            ignore_param = False
            for param in IGNORE_PARAMS:
                if param in key:
                    ignore_param = True
            if not ignore_param:
                    result += [f"{key}={value}\n"]
            else:
                pass
    return result

if __name__ == "__main__":
    # Create the progress window
    progress_win = ProgressWindow()

    # Prepare to download from github
    load_dotenv()
    branch = "v4.3.0-dev"
    qgc_github = QGCGitHub(branch)

    filepath = "sees_installer/QGroundControl%20Daily.ini"
    try:
        qgc_ini_file = qgc_github.download_file(filepath)
    except Exception as e:
        print(f"{e}\n\nPlease install a .env file in the script path with the Github API key which you can find in:\nhttps://www.notion.so/seesai/QGroundcontrol-Sees-flavour-17cf348e991880faa376c0317c9d3ce0")
        exit(1)
    # Save the file in case user wants to inspect and compare manually.
    file_output = "QGroundControl_github.ini"

    with open(file_output, 'wb') as file:
        file.write(qgc_ini_file)
    print(f"File downloaded and saved to {file_output}")

    #with open('QGroundControl_github.ini', 'r') as file1, open(''/home/sees/.config/QGroundControl.org/QGroundControl Daily.ini', 'r') as file2:
    #    github_ini_lines = file1.readlines()
    #    local_ini_lines = file2.readlines()

    config_github = configparser.ConfigParser()
    config_github.optionxform = str  # This preserves case
    config_github.read('QGroundControl_github.ini')
    github_ini_lines = section_to_text(config_github, CONFIG_CRITICAL_SECTIONS)

    config_local = configparser.ConfigParser()
    config_local.optionxform = str  # This preserves case
    config_local.read('/home/sees/.config/QGroundControl.org/QGroundControl Daily.ini')
    local_ini_lines = section_to_text(config_local, CONFIG_CRITICAL_SECTIONS)
    diff = list(difflib.unified_diff(github_ini_lines, local_ini_lines, fromfile='QGC github', tofile='QGC local', n=0))

    progress_win.update_message("Checking QGroundControl config file...")
    time.sleep(1)  # Simulate doing more work


    if len(diff) > 0:
        # There are some differences
        diff_content = ''.join(diff)
        progress_win.wait_for_user(f"The files are different, what do you want to do?\n{diff_content}")
        print(f"These are the differences found \n{diff_content}")
        if progress_win.abort_execution:
            progress_win.app.quit()
            sys.exit(1)
        progress_win.update_message("Ignoring differences, starting QGC...")
        time.sleep(1)  # Simulate doing more work
    else:
        progress_win.update_message("No differences found, starting QGC...")
        time.sleep(1)  # Simulate doing more work

    # There are no differences or the user is happy to continue
    # No differences in file,can go ahead and run QGC
    cmd_lst = ["./QGroundControl-v4.3.0-0.0.3.AppImage"]
    try:
        subprocess.Popen(
		    cmd_lst,
        start_new_session=True,
        text=True,
        bufsize=1
          )
    except Exception as e:
        print(f"{e}\n\n. Error starting QGroundControl, have you chmod +x the QGroundControl-vXXX.AppImage file?")
    # Keep window open until user closes it
    #sys.exit(progress_win.run())
    # Sleep to give QGC time to start
    time.sleep(2)
    # Close windows
    progress_win.app.quit()
    sys.exit(0)
