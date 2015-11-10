#!/usr/bin/python
#Copyright (c) 2014 Gary Furnish
#Licensed under the MIT License (MIT)

import platform, json, os
from utilities import printing_chdir, printing_mkdir, do_cmake

def generate_linux():
    with open("settings.json") as settings_file:
        settings_json = json.loads(settings_file.read())
        current_directory = os.getcwd()
        gcc_install_location = settings_json["gcc_install_location"]
        gcc_install_location = os.path.abspath(gcc_install_location)
        clang_install_location = settings_json["clang_install_location"]
        clang_install_location = os.path.abspath(clang_install_location)
        bandit_include_path = settings_json["bandit_include_path"]
        bandit_include_path = os.path.abspath(bandit_include_path)
        build_location = settings_json["build_location"]
        build_location = os.path.abspath(build_location)
        install_path = settings_json["install_path"]
        install_path = os.path.abspath(install_path)
        boehm_path = settings_json["boehm_path"]
        boehm_path = os.path.abspath(boehm_path)

    printing_mkdir(build_location+'/cgc1/')
    printing_chdir(build_location+'/cgc1/')
    ddict = {}
    ddict["BOEHM_LIB"]=boehm_path+'/lib'
    ddict["BOEHM_INCLUDE"]=boehm_path+'/include'
    ddict["CMAKE_C_COMPILER"] = gcc_install_location+'/gcc'
    ddict["CMAKE_CXX_COMPILER"] = gcc_install_location+'/g++'
    ddict["CMAKE_CXX_FLAGS"] = "-DCGC1_THREAD_SAFETY "
    ddict["BANDIT_INCLUDE_PATH"] = bandit_include_path
    ddict["CMAKE_INSTALL_PREFIX"] = install_path
    ddict["CMAKE_BUILD_TYPE"]="RelWithDebInfo"
    printing_mkdir("unixmake_gcc_release")
    printing_chdir("unixmake_gcc_release")
    do_cmake(current_directory, "Ninja", ddict)
    printing_chdir("../")
    printing_mkdir("unixmake_gcc_debug")
    printing_chdir("unixmake_gcc_debug")
    ddict["CMAKE_BUILD_TYPE"]="Debug"
    do_cmake(current_directory, "Ninja", ddict)
    ddict["CMAKE_C_COMPILER"] = clang_install_location+'/clang'
    ddict["CMAKE_CXX_COMPILER"] = clang_install_location+'/clang++'
    printing_chdir("../")
    printing_mkdir("unixmake_clang_release")
    printing_chdir("unixmake_clang_release")
    ddict["CMAKE_BUILD_TYPE"]="RelWithDebInfo"
    do_cmake(current_directory, "Ninja", ddict)
    printing_chdir("../")
    printing_mkdir("unixmake_clang_debug")
    printing_chdir("unixmake_clang_debug")
    ddict["CMAKE_BUILD_TYPE"]="Debug"
    do_cmake(current_directory, "Ninja", ddict)
    ddict["CMAKE_C_COMPILER"] = gcc_install_location+'/gcc'
    ddict["CMAKE_CXX_COMPILER"] = gcc_install_location+'/g++'
    ddict["CMAKE_CXX_FLAGS"] = ddict["CMAKE_CXX_FLAGS"] + "-O0 -fno-inline -fprofile-arcs -ftest-coverage -DCGC1_NO_INLINES"
    ddict["CMAKE_EXE_LINKER_FLAGS"] ="-fprofile-arcs -ftest-coverage"
    ddict["CMAKE_SHARED_LINKER_FLAGS"]="-fprofile-arcs -ftest-coverage"
    ddict["CMAKE_MODULE_LINKER_FLAGS"]="-fprofile-arcs -ftest-coverage"
    printing_chdir("../")
    printing_mkdir("unixmake_gcc_gcov")
    printing_chdir("unixmake_gcc_gcov")
    do_cmake(current_directory, "Ninja", ddict)

generate_linux()
