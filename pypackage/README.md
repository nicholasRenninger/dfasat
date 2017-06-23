# README

For the moment, please check the Jupyter notebook or read the doxygen documentation of the C++ package.


Publish instructions:

Add files
=========
Add files to MANIFEST.in

Upload
======
Set version number in `setup.py`:

    setup(name='dfasat',
          version='0.0.1.40',  # increment this version number
          description='DFASAT',

To upload:

    python3 setup.py sdist upload [-r https://testpypi.python.org/pypi]


