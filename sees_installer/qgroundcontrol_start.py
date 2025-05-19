#!/usr/bin/env python3
import requests
from dotenv import load_dotenv
import os
import argparse
import difflib
import subprocess
from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout, QHBoxLayout, QPushButton, QScrollArea, QRadioButton, QCheckBox
from PyQt5.QtCore import Qt
import sys
import time
import configparser
import yaml

VERSION = 1.2 # Added selection of drones, added config by yaml


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


class QGCStartWindow:
    def __init__(self):
        self.app = QApplication(sys.argv)
        self.continue_execution = False
        self.abort_execution = False
        self.drone_selected = None

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

        layoutHconfirm = QHBoxLayout()

        # Create continue button
        self.continue_button = QPushButton("Continue")
        self.continue_button.clicked.connect(self.on_continue_clicked)
        self.continue_button.setEnabled(False)  # Hidden by default
        layoutHconfirm.addWidget(self.continue_button)

        # Create abort button
        self.abort_button = QPushButton("Abort")
        self.abort_button.clicked.connect(self.on_abort_clicked)
        self.abort_button.setEnabled(True)  # Hidden by default
        layoutHconfirm.addWidget(self.abort_button)

        self.btn_uav_lst = []
        for uav in UAV_DICT.keys():
            self.btn_uav_lst.append(QRadioButton(uav))
        layoutHdrone = QHBoxLayout()
        for button in self.btn_uav_lst:
            layoutHdrone.addWidget(button)
            button.toggled.connect(self.btn_uav_toggled)

        self.chech_box_fpv = QCheckBox("FPV required")
        self.chech_box_fpv.setChecked(True)

        # Set layout and show
        layoutV.addWidget(QLabel("Select drone to Continue:"))
        layoutV.addLayout(layoutHdrone)
        layoutV.addWidget(self.chech_box_fpv)
        layoutV.addLayout(layoutHconfirm)
        self.window.setLayout(layoutV)
        self.window.show()
        self.app.processEvents()

    def btn_uav_toggled(self):
        for button in self.btn_uav_lst:
            if button.isChecked():
                self.drone_selected = button.text()
                print(f"Drone selected: {self.drone_selected}")
                self.continue_button.setEnabled(True)  # Hidden by default

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
        self.app.processEvents()

        # Wait until the button is clicked
        while not self.continue_execution and not self.abort_execution:
            self.app.processEvents()
            time.sleep(0.1)  # Small sleep to prevent high CPU usage

    def on_continue_clicked(self):
        """Called when the continue button is clicked"""
        self.continue_execution = True
        self.enable_fpv = self.chech_box_fpv.isChecked()
        print(f"Enable video: {self.enable_fpv}")

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

def update_config_comms(config, drone_name, RTSP_IP, RTSP_port, mavlink_IP, mavlink_port):
    print(f"Setting QGC connection to {drone_name} at {mavlink_IP}")
    if qgc_start_win.enable_fpv:   #ToDo, accessing qgc_start_win direclty is not pretty
        config['Video']['rtspUrl'] = f"rtsp://{RTSP_IP}:{RTSP_port}/fpv"
        config['Video']['videoSource'] = "RTSP Video Stream"
    else:
        config['Video']['videoSource'] = "Video Stream Disabled"
    config['LinkConfigurations']['Link0\\auto'] = "true"
    config['LinkConfigurations']['Link0\\host0'] = mavlink_IP
    config['LinkConfigurations']['Link0\\port0'] = mavlink_port
    config['LinkConfigurations']['Link0\\port'] = mavlink_port
    config['LinkConfigurations']['Link0\\name'] = f"Auto {drone_name} {mavlink_IP}"
    return config

if __name__ == "__main__":
    # Load settings
    config_file = open("qgroundcontrol_start.yaml")
    config = yaml.safe_load(config_file)
    UAV_DICT = config["UAVS"]
    CONFIG_CRITICAL_SECTIONS = config["QGC_INI"]["CONFIG_CRITICAL_SECTIONS"]
    IGNORE_PARAMS = config["QGC_INI"]["IGNORE_PARAMS"]
    RTSP_PORT = config["RTSP"]["PORT"]
    QGC_INI_PATH = config["QGC_INI"]["PATH"]

    # Create the progress window
    qgc_start_win = QGCStartWindow()

    # Prepare to download from github
    load_dotenv()
    branch = config["GIT"]["Branch"]
    qgc_github = QGCGitHub(branch)

    gitfilepath = "sees_installer/QGroundControl%20Daily.ini"

    try:
        qgc_ini_file = qgc_github.download_file(gitfilepath)
    except Exception as e:
        print(f"{e}\n\nPlease install a .env file in the script path with the Github API key which you can find in:\nhttps://www.notion.so/seesai/QGroundcontrol-Sees-flavour-17cf348e991880faa376c0317c9d3ce0")
        exit(1)
    # Save the file in case user wants to inspect and compare manually.
    file_output = "QGroundControl_github.ini"

    with open(file_output, 'wb') as file:
        file.write(qgc_ini_file)
    print(f"File downloaded and saved to {file_output}")

    config_github = configparser.ConfigParser()
    config_github.optionxform = str  # This preserves case
    config_github.read('QGroundControl_github.ini')
    github_ini_lines = section_to_text(config_github, CONFIG_CRITICAL_SECTIONS)

    config_local = configparser.ConfigParser()
    config_local.optionxform = str  # This preserves case
    config_local.read(QGC_INI_PATH)
    local_ini_lines = section_to_text(config_local, CONFIG_CRITICAL_SECTIONS)
    diff = list(difflib.unified_diff(github_ini_lines, local_ini_lines, fromfile='QGC github', tofile='QGC local', n=0))

    qgc_start_win.update_message("Checking QGroundControl config file...")
    time.sleep(1)  # Give time to display message

    if len(diff) > 0:
        # There are some differences
        diff_content = ''.join(diff)
        qgc_start_win.wait_for_user(f"The files are different, what do you want to do?\n{diff_content}")
        print(f"These are the differences found \n{diff_content}")
        if qgc_start_win.abort_execution:
            qgc_start_win.app.quit()
            sys.exit(1)
        qgc_start_win.update_message("Ignoring differences, starting QGC...")
        time.sleep(1)  # Display message
    else:
        qgc_start_win.wait_for_user("No differences found, select drone to start QGC...")
        time.sleep(1)  # Display message

    # Update config according to drone selection
    drone_info = UAV_DICT[qgc_start_win.drone_selected]
    drone_name = qgc_start_win.drone_selected


    update_config_comms(config_local, drone_name, drone_info["RTSP_IP"], RTSP_PORT, drone_info["MAVLINK_IP"], drone_info["MAVLINK_PORT"])


    # Update ini file with the drone selected
    with open( QGC_INI_PATH, 'w') as configfile:
        config_local.write(configfile)

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
    qgc_start_win.app.quit()
    sys.exit(0)
