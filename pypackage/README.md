# README

For the moment, please check the Jupyter notebook or read the doxygen documentation of the C++ package.

Disclaimer on building the package: The install script will download some dependencies like castXML and clone the development branch of flexfringe into src to run the python target.

Compiling the library for the package
=====================================
$ make clean python WITH_PYTHON=1

will build the shared library. 

Build instructions for the package
==================================
$ (sudo) python3 setup.py build

Install instructions
====================
$ (sudo) python3 setup.py install



Publish instructions
====================

Add files
=========
Add files to MANIFEST.in

Upload
======
Set version number in `setup.py` as in 

    setup(name='dfasat',
          version='0.0.1.40',  # increment this version number
          description='flexfringe',

and upload

    python3 setup.py sdist upload [-r https://testpypi.python.org/pypi]


