#!/usr/bin/python
#Copyright (c) 2014 Gary Furnish
#Licensed under the MIT License (MIT)

import platform, json, os, sys
from utilities import printing_chdir, printing_mkdir, do_cmake
import multiprocessing
num_cores=multiprocessing.cpu_count()


def build_linux():
    with open("settings.json") as settings_file:
        settings_json = json.loads(settings_file.read())
        build_location = settings_json["build_location"]
        build_location = os.path.abspath(build_location)
        printing_chdir(build_location+"/cgc1/")
        builds = ["unixmake_gcc_release","unixmake_gcc_debug","unixmake_clang_release","unixmake_clang_debug","unixmake_gcc_gcov"]
#        builds = ["unixmake_clang_release","unixmake_clang_debug"]
        for build in builds:
            printing_chdir(build)
            os.system("ninja")
            printing_chdir("..")

build_linux()
sys.exit(0)
