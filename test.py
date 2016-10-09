#!/usr/bin/python
#Copyright (c) 2014 Gary Furnish
#Licensed under the MIT License (MIT)

import platform, json, os
from utilities import printing_chdir, printing_mkdir, do_cmake, printing_system
import multiprocessing
num_cores=multiprocessing.cpu_count()
from multiprocessing import Pool

with open("settings.json") as settings_file:
    settings_json = json.loads(settings_file.read())
    build_location = settings_json["build_location"]
    build_location = os.path.abspath(build_location)

def run_test(location):
    printing_system(location+"/mcpputil/mcpputil_test/mcpputil_test")
    printing_system(location+"/mcpposutil/mcpposutil/mcpposutil_test/mcpposutil_test")
    printing_system(location+"/mcppalloc_sparse/mcppalloc_sparse_test/mcppalloc_sparse_test")
    printing_system(location+"/mcppalloc_bitmap_allocator/mcppalloc_bitmap_allocator_test/mcppalloc_bitmap_allocator_test")
    printing_system(location+"/cgc1_test/cgc1_test")

def test_linux():
    printing_chdir(build_location+"/cgc1/")
#    builds = ["unixmake_gcc_release","unixmake_gcc_debug","unixmake_clang_release","unixmake_clang_debug","unixmake_gcc_gcov"]
    builds = ["unixmake_clang_release","unixmake_clang_debug"]
    pool = Pool()
    pool.map(run_test,builds)

test_linux()
