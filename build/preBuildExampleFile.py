#!/usr/bin/env python3

from __future__ import print_function
from os import path
import inspect
from shutil import copyfile

def preBuildExampleFile():
    filename = inspect.getframeinfo(inspect.currentframe()).filename
    dir_path = path.dirname(path.abspath(filename))
    target = dir_path + "/../include/configuration.hpp"

    # If you save the file exatly after the build checks this line it will be overwritten
    if path.isfile(target): return
    try:
        copyfile(target + ".example", target)
    except Exception as e:
        raise Exception("Unable to create include/configuration.hpp file based on include/configuration.hpp.example: " + str(e))

if __name__ == "__main__":
    preBuildExampleFile()
