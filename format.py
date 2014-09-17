#! /usr/bin/python3
#Copyright (c) 2014 Gary Furnish
#Licensed under the MIT License (MIT)

import os, fnmatch, json
from multiprocessing import Pool
from utilities import printing_system
with open("settings.json") as settings_file:
    settings_json = json.loads(settings_file.read())
    clang_install_location = settings_json["clang_install_location"]
    clang_install_location = os.path.abspath(clang_install_location)


#See https://stackoverflow.com/questions/2186525/use-a-glob-to-find-files-recursively-in-python
def find_files(directory, pattern):
    for root, dirs, files in os.walk(directory):
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                filename = os.path.join(root, basename)
                yield filename

def format(filename):
    printing_system(clang_install_location+"/clang-format -style=file -i " + filename)

pool = Pool()
files = list(find_files(".","*.hpp")) + list(find_files(".","*.cpp"))
pool.map(format,files)
