
from setuptools.command.build_ext import build_ext as _build_ext

import numpy
from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
from Cython.Build import cythonize

include_dirs = ['../src',numpy.get_include()]
library_dirs = ['../build/src']

extra_compile_args = ['-std=c++11', "-mmacosx-version-min=10.9"]

ext = Extension(
	"speck3d",                 # name of extension
	["speck3d.pyx"],           # filename of our Pyrex/Cython source
	language="c++",              # this causes Pyrex/Cython to create C++ source
	include_dirs=include_dirs,          # usual stuff
	library_dirs=library_dirs,
	libraries=['sam_helper', 'SPECK3D', 'SPECK_Storage', 'CDF97', 'speck_helper'],
	extra_compile_args=extra_compile_args,
	extra_link_args=[]       # if needed
	#cmdclass = {'build_ext': build_ext}
)

setup(name='speck3d',ext_modules=cythonize(ext, language_level='3'))
