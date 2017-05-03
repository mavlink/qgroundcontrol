#!/bin/bash
find . -name "*.xml" -exec xmllint -format {} -o {} \;
