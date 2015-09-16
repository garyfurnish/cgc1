#!/usr/bin/python3
import os, json
from utilities import printing_system
with open("settings.json") as settings_file:
    settings_json = json.loads(settings_file.read())
    build_location = settings_json["build_location"]
    build_location = os.path.abspath(build_location)
    gcov_location = settings_json["gcov_location"]
    gcov_location = os.path.abspath(gcov_location)
os.chdir(build_location+"/cgc1/unixmake_gcc_gcov/cgc1/CMakeFiles/cgc1.dir/src")
printing_system(gcov_location+"/gcov *.gcno *.gcda -p -f -m > gcov.tmp")
found_already = set()
empty_already = set()
with open('gcov.tmp', 'r') as f:
    line = f.readline()
    while line:
        if line in found_already:
            line = f.readline()
            continue
        is_cgc = line.startswith("Function 'cgc1::")
        if is_cgc:
            next_line = f.readline()
            if not next_line.startswith("Lines executed:100.00%"):
                if not line in empty_already:
                    print(line[:-1])
                    print(next_line[:-1])
                    empty_already.add(line)
            else:
                found_already.add(line)
        line = f.readline()
