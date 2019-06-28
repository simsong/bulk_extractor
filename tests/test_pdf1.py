#!/usr/bin/env python3
#
# first test of bulk_extractor using python
EXIT_SUCCESS = 0
EXIT_SKIPPED = 77
EXIT_HARD_ERROR = 99

import os
import sys
import subprocess

LOOKING_FOR = """
71-PDF-0 qwertyuiop
71-PDF-11 asdfghjkl
71-PDF-21 zxcvbnm    
"""

looking_for = set(LOOKING_FOR.strip().split('\n'))

if __name__=="__main__":
    r = subprocess.check_call(['../src/bulk_extractor','-Z','-E','wordlist','-e','pdf','-o','test_pdf1.out','Data/pdf_words1.pdf'])

    # Now look for the feature files
    with open("test_pdf1.out/wordlist.txt","r") as f:
        for line in f:
            line = line.strip().replace("\t"," ")
            print('remove',line)
            try:
                looking_for.remove(line)
            except KeyError:
                pass
    if looking_for:
        exit(EXIT_HARD_ERROR)
    exit(EXIT_SUCCESS)
    
