#! /usr/bin/python
#Copyright (c) 2014 Gary Furnish
#Licensed under the MIT License (MIT)

import os, fnmatch, json, sys
from multiprocessing import Pool
from utilities import printing_system
with open("settings.json") as settings_file:
    settings_json = json.loads(settings_file.read())
    clang_install_location = settings_json["clang_install_location"]
    clang_install_location = os.path.abspath(clang_install_location)


#See https://stackoverflow.com/questions/2186525/use-a-glob-to-find-files-recursively-in-python
def find_files(directories, patterns):
    for directory in directories:
        for pattern in patterns:
            for root, dirs, files in os.walk(directory):
                for basename in files:
                    if fnmatch.fnmatch(basename, pattern):
                        filename = os.path.join(root, basename)
                        yield filename

def format(filename):
    printing_system(clang_install_location+"/clang-format -style=file -i " + filename)

pool = Pool()
files = list(find_files(["cgc1","cgc1_test","cgc1_alloc_benchmark","mcppalloc_utils","mcppalloc_sparse","mcppalloc_sparse_test","mcppalloc_bitset"],["*.cpp","*.hpp"]))
pool.map(format,files)
sys.exit(0)
