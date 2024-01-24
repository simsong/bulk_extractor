#!/usr/bin/env python3

import os
import fnmatch
import os.path


OUTFILE = "Makefile.auto_defs"

RULES = [['be20_api/', 'AUTO_CPP_FILES', ['*.cpp']],
         ['be20_api/', 'AUTO_H_FILES', ['*.h']],
         ['tests/',    'AUTO_TESTS_DIST', ['*']],
         ['.',          'AUTO_EXTRA_DIST', ['*.md','*.am','*.txt','*.py','*.bash','.gitignore']]]

IGNORE = set(['Makefile.am','Makefile.in', 'dfxml_demo.cpp'])

if __name__=="__main__":
    vars = dict()
    for (start,name,pats) in RULES:
        matches = []
        for (root, dirs, files) in os.walk(start, topdown=False):
            for fn in files:
                for pat in pats:
                    if fnmatch.fnmatch(fn, pat) and fn not in IGNORE:
                        matches.append( os.path.relpath(os.path.join(root,fn) ))
        vars[name] = sorted(matches)
    with open(OUTFILE,'w') as f:
        for (name,matches) in vars.items():
            if len(matches)==0:
                continue
            f.write(f"\n{name} = ")
            for fname in matches:
                f.write(f" \\\n\t{fname}")
            f.write("\n")
