import os
import datetime
import re

# Define the file extensions you want to target
TARGET_EXTENSIONS = ['.cpp', '.cc', '.h', '.qml']  # Add other extensions as needed

# Define the new header with a placeholder for the updated year
HEADER_TEMPLATE = '''/****************************************************************************
 *
 * (c) 2009-{year} QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
'''


def update_header_in_file(file_path, header_template):
    # Get the current year
    current_year = datetime.datetime.now().year
    new_header = header_template.format(year=current_year)

    with open(file_path, 'r') as file:
        content = file.read()

    # Regex pattern to match both "2009-XXXX" and just "XXXX"
    header_pattern = re.compile(r'\(c\)\s*(2009-\d{4}|\d{4}) QGROUNDCONTROL')

    # Check if the header already exists
    match = header_pattern.search(content)

    if match:
        # Update the header to include "2009-current_year" if it is in the wrong format
        updated_content = header_pattern.sub(f'(c) 2009-{current_year} QGROUNDCONTROL', content)
    else:
        # Prepend the new header if it's missing
        updated_content = new_header + "\n" + content

    with open(file_path, 'w') as file:
        file.write(updated_content)


def process_directory(directory):
    for root, _, files in os.walk(directory):
        for file in files:
            if any(file.endswith(ext) for ext in TARGET_EXTENSIONS):
                file_path = os.path.join(root, file)
                update_header_in_file(file_path, HEADER_TEMPLATE)


if __name__ == "__main__":
    current_directory = os.path.dirname(os.path.abspath(__file__))
    source_directory = current_directory + "/../src"
    print(source_directory)
    process_directory(source_directory)
    print("Headers updated.")
