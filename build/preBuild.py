#!/usr/bin/env python3

Import('env')
from shutil import copyfile
import subprocess
import inspect, os.path
from os.path import join, realpath

from preBuildCertificates import preBuildCertificates
from preBuildExampleFile import preBuildExampleFile

filename = inspect.getframeinfo(inspect.currentframe()).filename
dir_path = os.path.dirname(os.path.abspath(filename))

preBuildExampleFile()
preBuildCertificates(env)