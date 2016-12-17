#!/usr/bin/python
import json, os

with open("settings.json") as settings_file:
    settings_json = json.loads(settings_file.read())
    clang_install_location = settings_json["clang_install_location"]
    clang_install_location = os.path.abspath(clang_install_location)
    build_location = settings_json["build_location"]

cmake_build = build_location+"cgc1/unixmake_clang_debug"

checks = "boost-*,cert-*,cppcoreguildlines-*,clang-analyzer-*,modernize-*,performance-*,readability-*,-readability-named-parameter,-clang-analyzer-alpha.clone.CloneChecker,-clang-analyzer-alpha.core.CastSize,-clang-analyzer-alpha.core.SizeofPtr,-cert-err58-cpp,-clang-analyzer-alpha.core.CastToStruct,-clang-analyzer-alpha.deadcode.UnreachableCode,-readability-non-const-parameter,-readability-avoid-const-params-in-decls,-modernize-use-using,-modernize-redundant-void-arg,-modernize-use-default"
#The following do not exclude system headers so are excluded
#-modernize-use-using,-modernize-redundant-void-arg
#checks = "readability-*,-readability-named-parameter,-modernize-use-default"
test_file_checks = checks+",-google-build-using-namespace"

src_files = ["mcpputil/mcpputil/src/*.cpp","mcppconcurrency/mcppconcurrency/src/*.cpp","cgc1/src/*.cpp"]
test_files = ["mcpputil/mcpputil_test/*.cpp", "mcppconcurrency/mcppconcurrency_test/*.cpp", "mcppalloc/mcppalloc_bitmap_allocator/mcppalloc_bitmap_allocator_test/*.cpp", "mcppalloc/mcppalloc_sparse/mcppalloc_sparse_test/*.cpp", "cgc1_alloc_benchmark/*.cpp", "cgc1_test/*.cpp"]


src_str = ' '.join(map(str, src_files))
test_files_str = ' '.join(map(str, test_files))
header_filter = ["mcpputil/*","mcpputil/mcpputil/include/mcpputil/mcpputil/.*","mcppalloc/*","mcpposutil/*","mcppconcurrency","cgc1/*"]
header_filter_str = ','.join(map(str, header_filter))

cmd = clang_install_location+"/clang-tidy -p=" + cmake_build + " " + src_str + " -checks="+checks+" --header-filter=\".mcpp.*|cgc1/cgc1.*\""

print cmd
os.system(cmd)
cmd = clang_install_location+"/clang-tidy -p=" + cmake_build + " " + test_files_str + " -checks="+test_file_checks+" --header-filter=\".mcpp.*|cgc1/cgc1.*\""

#print cmd
#os.system(cmd)
