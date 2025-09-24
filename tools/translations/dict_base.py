#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from abc import ABC, abstractmethod

class DictBase(ABC):
    def __init__(self):
        self.name = "base"
        self.dictionary = None

    def translate_text(self, english_text: str, keep_space: bool = False):
        if not self.dictionary:
            d = self.get_dict()
            self.dictionary = {}
            for k, v in d.items():
                self.dictionary[k.strip()] = v.strip()

        # Remove leading and trailing spaces for matching
        stripped_text = english_text.strip()

        # Convert character entity references to plain characters for matching
        compare_text = self.decode_html_entities(stripped_text)

        if compare_text in self.dictionary:
            if  keep_space:
                leading_spaces = len(english_text) - len(english_text.lstrip())
                trailing_spaces = len(english_text) - len(english_text.rstrip())
                translation = self.dictionary[compare_text]
                return self.encode_html_entities(' ' * leading_spaces + translation + ' ' * trailing_spaces)

            return self.encode_html_entities(self.dictionary[compare_text])

        # return None to indicate untranslatable
        return None

    def decode_html_entities(self, text):
        # Convert character entity references to plain characters for matching
        replacements = {
            '&apos;': "'",
            '&quot;': '"',
            '&amp;': '&',
            '&lt;': '<',
            '&gt;': '>',
        }
        result = text
        for entity, char in replacements.items():
            result = result.replace(entity, char)
        return result

    def encode_html_entities(self, text):
        replacements = {
            "'":'&apos;',
            '"':'&quot;',
            # '&':'&amp;',   # don't encode this
            '<':'&lt;',
            '>':'&gt;',
        }
        result = text
        for entity, char in replacements.items():
            result = result.replace(entity, char)
        return result

    @abstractmethod
    def get_dict(self) -> dict[str, str]:
        return None

