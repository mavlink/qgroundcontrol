#!/bin/bash
xcodebuild -version 2>&1 | (head -n1) | awk  '{print $2}'

