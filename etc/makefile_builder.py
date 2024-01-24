#!/usr/bin/env python3

import os
import fnmatch
import os.path


config_src = {
    'root':'src',
    'outfile':'Makefile.auto_defs',
    'rules':[['be20_api/', 'AUTO_CPP_FILES', ['*.cpp']],
             ['be20_api/', 'AUTO_H_FILES', ['*.h']],
             ['tests/',    'AUTO_TESTS_DIST', ['*']],
             ['rar/',      'AUTO_RAR_FILES', ['*.cpp','*.hpp']],
             ['.',          'AUTO_EXTRA_DIST', ['*.md','*.am','*.txt','*.py','*.bash','.gitignore']]],
    'ignore_fnames':set(['Makefile.am','Makefile.in',
                         'dfxml_demo.cpp',
                         'dfxml_version.cpp',
                         'iblkfind.cpp',
                         'test_dfxml.cpp',
                         'smoke.cpp',
                         'test_be20_api.cpp',
                         'test_be20_threadpool.cpp'
                         ]),
    'ignore_paths':set(['be20_api/utfcpp/tests',
                    'be20_api/utfcpp/samples',
                    'be20_api/tests',
                    'be20_api/demos'])}

config_root = {
    'root':'.',
    'outfile':'Makefile.auto_defs',
    'rules':[['doc', 'AUTO_DOC_FILES', ['*.pdf',  '*.html', '*.txt', '*.md', '*.tex', '*.gitignore']],
             ['etc', 'AUTO_ETC_FILES', ['*.bash', '*.py', '*.gitignore']],
             ['licenses', 'AUTO_LICENSES', ['*']]],
    'ignore_fnames':[],
    'ignore_paths':[]
    }

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
