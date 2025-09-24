#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import os
import sys
import re

from dict_zh_CN import DictZhCN

LOG_FORMAT_CONSOLE = '%(asctime)s.%(msecs)03d|%(levelname)s|%(message)s'

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
console_handler = logging.StreamHandler(sys.stdout)
console_formatter = logging.Formatter(fmt=LOG_FORMAT_CONSOLE,datefmt='%H:%M:%S')
console_handler.setFormatter(console_formatter)
logger.addHandler(console_handler)

class QgcTranslater:

    def __init__(self):
        super().__init__()
        self.dictionary = DictZhCN()

    def translate_text(self, english_text):
        return self.dictionary.translate_text(english_text)

    def process_qt_file(self, source_path, target_path):
        logger.info(f"dict include {len(self.dictionary.get_dict())} items")

        logger.info(f"load qt file: {source_path}")

        with open(source_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Count the number of messages processed
        self.processed_count = 0
        self.translated_count = 0

        # save the original text for which no translation was found, making it convenient for AI translation.
        self.missings = set()

        message_pattern = r'(<message>.*?</message>)'
        new_content = re.sub(message_pattern, self.process_message, content, flags=re.DOTALL)

        # write translate result
        with open(target_path, 'w', encoding='utf-8') as f:
            f.write(new_content)

        missing_file = target_path + "_missing.txt"
        if len(self.missings) > 0:
            with open(missing_file, 'wt', encoding='utf-8') as f:
                lines = []
                for m in self.missings:
                    lines.append(rf'"{m}": "TODO",'+"\n")
                f.writelines(lines)

        logger.info(f"handle {source_path} complete")
        logger.info(f"  handle {self.processed_count} messages")
        logger.info(f"  translate {self.translated_count} items")
        logger.info(f"  found {len(self.missings)} missing, saved in {missing_file}")

    def process_message(self, match):
        # nonlocal processed_count, translated_count
        self.processed_count += 1

        message_content = match.group(1)

        # extract source content
        source_match = re.search(r'<source>(.*?)</source>', message_content, re.DOTALL)
        if not source_match:
            return message_content

        source_text = source_match.group(1).strip()
        # logger.info(f"handle source_text={source_text}")

        if 'type="unfinished"' not in message_content:
            return message_content  # already translate

        # try translate
        translate_result = self.translate_text(source_text)
        if translate_result is None:
            # can not translate, just add the original text to missing set
            self.missings.add(source_text.strip())
            return message_content

        self.translated_count += 1

        # Display only the first 50 characters to avoid overly long output.
        source_preview = source_text[:50] + '...' if len(source_text) > 50 else source_text
        translation_preview = translate_result[:50] + '...' if len(translate_result) > 50 else translate_result
        logger.info(f"[{self.translated_count}]: '{source_preview}' => '{translation_preview}'")

        # Replace the content of the translation and remove type="unfinished"
        return re.sub(
            r'<translation type="unfinished">.*?</translation>',
            f'<translation>{translate_result}</translation>',
            message_content,
            flags=re.DOTALL
        )


    def do_demo(self):
        logger.info(f"there are {len(self.dictionary.get_dict())} translate items")

        sample_keys = list(self.dictionary.get_dict().keys())[:10]
        logger.info("translate items demos:")
        for key in sample_keys:
            logger.info(f"'{key}' -> '{self.dictionary.get_dict()[key]}'")

        return 0

def main():
    base_path = os.path.join("..", "..", "translations" )
    files_to_process = [
        'qgc_json_zh_CN.ts',
        'qgc_source_zh_CN.ts',
        # 'qgc_zh_CN.ts',
        #'qgc-json.ts'
    ]
    try:
        logger.info("start translate")
        translater = QgcTranslater()
        for file_name in files_to_process:
            source_path = os.path.join(base_path, file_name)
            target_path = source_path # + ".new"

            logger.info(f"source_path={source_path}, target_path={target_path}")
            if os.path.exists(source_path):
                translater.process_qt_file(source_path, target_path)
            else:
                logger.warning(f"file not exist：{source_path}")
        sys.exit(0)
    except KeyboardInterrupt:
        logger.warning("user break")
        sys.exit(1)
    except Exception as e:
        logger.error(f"there is error while translate: {e}")
        logger.exception("error details:")
        sys.exit(1)

def check_dict_duplicate(dict_path: str):
    check_dict = {}
    duplicate_count = 0
    with open(dict_path, 'r', encoding='utf-8') as f:
        for idx, line in enumerate(f):
            # logger.info(f"[{idx}]line={line.strip()}")
            if ":" in line:
                split_results = line.split(":", 1)
                key = split_results[0].strip()
                if not key in check_dict:
                    check_dict[key] = split_results[1]
                else:
                    logger.warning(f"duplicate: idx={idx}, key={key}")
                    duplicate_count += 1
    logger.info(f"duplicate_count={duplicate_count}")

if __name__ == "__main__":
   main()
   # check_dict_duplicate("dict_zh_CN.py")
