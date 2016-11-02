#Copyright (c) 2014 Gary Furnish
#Licensed under the MIT License (MIT)
import os, errno

def printing_chdir(path):
    print("cd " + path)
    os.chdir(path)
def printing_mkdir(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise  # raises the error again

def printing_system(cmd,throw_except_on_error=True):
    print(cmd)
    if os.system(cmd) and throw_except_on_error:
        raise Exception("Error with cmd: " + cmd)

def do_cmake(current_directory, generator_string, ddict):
    sys_str = 'cmake ' + current_directory + ' -G "' + generator_string +'"'
    for key,value in ddict.iteritems():
        sys_str += ' -D' + key + '="' + value + '"'
    sys_str+=' -DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
    printing_system(sys_str)
