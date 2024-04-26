#!/usr/bin/env python3
"""
Program to build the Makefile.auto_defs, which includes the defines for automake.
This was easier than fixing autotools to handle all of the subdirs, and easier than learning cmake.
"""


import os
import fnmatch
import os.path


# Root configuration
config_root = {
    'root':'.',
    'outfile':'Makefile.auto_defs',
    'rules':[['doc', 'AUTO_DOC_FILES', ['*.pdf',  '*.html', '*.txt', '*.md', '*.tex', '*.gitignore']],
             ['etc', 'AUTO_ETC_FILES', ['*.bash', '*.py', '*.gitignore']],
             ['python', 'AUTO_PYTHON_FILES', ['*.py']],
             ['licenses', 'AUTO_LICENSES', ['*']]],
    'ignore_fnames':[],
    'ignore_paths':[]
    }

# the source files we want
config_src = {
    'root':'src',
    'outfile':'Makefile.auto_defs',
    # 'rules' is an array of (root, AUTOCONF_SYMBOL, GLOBs)
    'rules':[['be20_api/', 'AUTO_CPP_FILES',  ['*.cpp']],
             ['be20_api/', 'AUTO_H_FILES',    ['*.h']],
             ['tests/',    'AUTO_TESTS_DIST', ['*']],
             ['rar/',      'AUTO_RAR_FILES',  ['*.cpp','*.hpp']],
             ['re2/re2',   'AUTO_RE2_H_FILES', ['*.h']],
             ['re2/re2',   'AUTO_RE2_CC_FILES', ['*.cc']],
             ['.',         'AUTO_EXTRA_DIST', ['*.md','*.am','*.txt','*.py','*.bash','.gitignore']]],

    # defs are specific paths to define
    'defs':[['absl', 'AUTO_ABSL_H',
             ["absl/base/attributes.h",
              "absl/base/call_once.h"
              "absl/base/macros.h"
              "absl/base/thread_annotations.h"
              "absl/container/fixed_array.h"
              "absl/container/flat_hash_map.h"
              "absl/container/flat_hash_set.h"
              "absl/container/inlined_vector.h"
              "absl/flags/flag.h"
              "absl/strings/ascii.h"
              "absl/strings/escaping.h"
              "absl/strings/str_format.h"
              "absl/strings/string_view.h"
              "absl/synchronization/mutex.h"
              "absl/types/optional.h"
              "absl/types/span.h"]]],

    # Ignore these anywhere we find them
    'ignore_fnames':set([
        'Makefile.am',
        'Makefile.in',
        'dfxml_demo.cpp',
        'dfxml_version.cpp',
        'iblkfind.cpp',
        'test_dfxml.cpp',
        'smoke.cpp',
        'test_be20_api.cpp',
        'test_be20_threadpool.cpp'
    ]),

    # ignore_paths is a set of paths to ignore. It typically has tests and utilities we aren't using
    'ignore_paths':set([
        'be20_api/utfcpp/tests',
        'be20_api/utfcpp/samples',
        'be20_api/tests',
        'be20_api/demos',
        're2/app',
        're2/benchlog',
        're2/doc',
        're2/lib',
        're2/python',
        're2/re2/fuzzing',
        're2/re2/testing',
        're2/util',
        'abseil-cpp/ci',

    ])}

def build(config):
    cwd = os.getcwd()
    os.chdir(config['root'])
    ignore_fnames = config['ignore_fnames']
    ignore_paths = config['ignore_paths']

    def should_ignore(full_path):
        if os.path.basename(full_path) in ignore_fnames:
            return True
        for ip in ignore_paths:
            if ip in full_path:
                return True
        return False

    vars = dict()
    for (start,name,pats) in config['rules']:
        matches = []
        for (root, dirs, files) in os.walk(start, topdown=False):
            for fn in files:
                for pat in pats:
                    full_path = os.path.relpath(os.path.join(root,fn))
                    if fnmatch.fnmatch(fn, pat) and not should_ignore(full_path):
                        matches.append( full_path )
        vars[name] = sorted(matches)
    with open(config['outfile'],'w') as f:
        for (name,matches) in vars.items():
            if len(matches)==0:
                continue
            f.write(f"\n{name} = ")
            for fname in matches:
                f.write(f" \\\n\t{fname}")
            f.write("\n")
    os.chdir(cwd)

if __name__=="__main__":
    build(config_root)
    build(config_src)
