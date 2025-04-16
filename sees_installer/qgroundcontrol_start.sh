#!/usr/bin/env python3
import requests
from dotenv import load_dotenv
import os
import argparse
import difflib
import subprocess
from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout, QHBoxLayout, QPushButton
from PyQt5.QtCore import Qt
import sys
import time


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
        self.window.setWindowTitle("Feedback")
        self.window.setGeometry(100, 100, 400, 200)

        # Create layout
        layoutV = QVBoxLayout()

        # Create message label
        self.message_label = QLabel("Checking QGroundControl config file...")
        self.message_label.setAlignment(Qt.AlignCenter | Qt.AlignVCenter)
        layoutV.addWidget(self.message_label)

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


# Example usage
if __name__ == "__main__":
    # Create the progress window
    progress_win = ProgressWindow()

    # Prepare to download from github
    load_dotenv()
    branch = "v4.3.0-dev"
    qgc_github = QGCGitHub(branch)

    filepath = "sees_installer/QGroundControl%20Daily.ini"
    qgc_ini_file = qgc_github.download_file(filepath)
    # Save the file in case user wants to inspect and compare manually.
    file_output = "QGroundControl_github.ini"
    with open(file_output, 'wb') as file:
        file.write(qgc_ini_file)
    print(f"File downloaded and saved to {file_output}")


    with open('QGroundControl_github.ini', 'r') as file1, open('/home/sees/.config/QGroundControl.org/QGroundControl Daily.ini', 'r') as file2:
        file1_lines = file1.readlines()
        file2_lines = file2.readlines()
        diff = list(difflib.unified_diff(file1_lines, file2_lines, fromfile='QGC.ini', tofile='QGC-test.ini', n=0))

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
    subprocess.Popen(
        cmd_lst,
        start_new_session=True,
        text=True,
        bufsize=1
    )

    # Keep window open until user closes it
    #sys.exit(progress_win.run())
    # Sleep to give QGC time to start
    time.sleep(2)
    # Close windows
    progress_win.app.quit()
    sys.exit(0)
