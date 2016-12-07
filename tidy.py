#!/usr/bin/python
import json, os

with open("settings.json") as settings_file:
    settings_json = json.loads(settings_file.read())
    clang_install_location = settings_json["clang_install_location"]
    clang_install_location = os.path.abspath(clang_install_location)
    build_location = settings_json["build_location"]

cmake_build = build_location+"cgc1/unixmake_clang_debug"

checks = "boost-*,cert-*,cppcoreguildlines-*,clang-analyzer-*,modernize-*,performance-*,readability-*,-readability-named-parameter,-clang-analyzer-alpha.clone.CloneChecker,-google-build-using-namespace,-clang-analyzer-alpha.core.CastSize"


src_files = ["mcpputil/mcpputil/src/*.cpp","mcpputil/mcpputil_test/*.cpp", "mcppalloc_bitmap_allocator/mcppalloc_bitmap_allocator_test/*.cpp", "mcppalloc_sparse/mcppalloc_sparse_test/*.cpp", "cgc1/src/*.cpp","cgc1_alloc_benchmark/*.cpp", "cgc1_test/*.cpp"]
src_str = ' '.join(map(str, src_files))
header_filter = ["mcpputil/*","mcppalloc/*","mcppalloc_bitmap/*","mcppalloc_bitmap_allocator/*","mcppalloc_slab_allocator/*","mcppalloc_sparse/*","mcpposutil/*", "cgc1/*"]
header_filter_str = ','.join(map(str, header_filter))

cmd = clang_install_location+"/clang-tidy -p=" + cmake_build + " " + src_str + " -checks="+checks+" --header-filter=" +header_filter_str + " -fix"

print cmd
os.system(cmd)
