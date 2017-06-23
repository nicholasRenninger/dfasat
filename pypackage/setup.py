from __future__ import print_function
import sys
from setuptools import setup, find_packages

from distutils.command.build import build
from subprocess import call

import os

with open('requirements.txt') as f:
    INSTALL_REQUIRES = [l.strip() for l in f.readlines() if l]
with open('dependencies.txt') as f:
    DEPENDENCY_LINKS = [l.strip() for l in f.readlines() if l]
with open('setup.txt') as f:
    SETUP_REQUIRES = [l.strip() for l in f.readlines() if l]

BASEPATH = os.path.dirname(os.path.abspath(__file__))
SOURCE_DIR = 'src'

import sys, subprocess, pkg_resources

def missing_requirements(specifiers):
    for specifier in specifiers:
        try:
            pkg_resources.require(specifier)
        except pkg_resources.DistributionNotFound:
            yield specifier

def install_requirements_ez(specifiers):
    for to_install in list(specifiers):
        subprocess.call(["easy_install", to_install])

def install_requirements_pip(specifiers):
    for to_install in list(specifiers):
        subprocess.call([sys.executable, "-m", "pip", "install", to_install])

install_requirements_pip(missing_requirements(SETUP_REQUIRES))
install_requirements_ez(DEPENDENCY_LINKS)

class DFASATBuild(build):
    def run(self):
        # run original build code
        build.run(self)

        def exec_(cmd, cwd=None):
            call(cmd, cwd=cwd)

        # download DFASAT
        cmd = ['git',
                'clone',
                'https://bitbucket.org/chrshmmmr/dfasat.git',
                SOURCE_DIR
        ]

        self.execute(exec_, [cmd], 'Cloning dfasat')

        cmd = ['git', 'checkout', 'development'] # TODO: remove once python version is merged into master
        self.execute(exec_, [cmd, SOURCE_DIR], 'Checking out development')

        # build DFASAT
        build_path = os.path.abspath(self.build_temp)
        self.mkpath(build_path)

        cmd = [
            'make',
            'OUTDIR=' + build_path,
            'WITH_PYTHON=1'
        ]


        targets = ['clean', 'python']
        cmd.extend(targets)

        target_files = [os.path.join(build_path, file_) for file_ in ['flexfringe.lib.so', 'flexfringe.so']]

        def compile():
            call(cmd, cwd=os.path.join(BASEPATH, SOURCE_DIR))

        self.execute(compile, [], 'Compiling flexfringe')

        lib_folder = os.path.join(self.build_lib, 'flexfringe', 'lib')

        # copy resulting tool to library build folder
        self.mkpath(lib_folder)

        if not self.dry_run:
            for target in target_files:
                self.copy_file(target, lib_folder)

print(find_packages())

setup(name='flexfringe',
      version='0.0.1.51',
      description='FLEXFRINGE',
      author='Christian Hammerschmidt, Benjamin Loos',
      packages=find_packages(),
#      dependency_links=DEPENDENCY_LINKS,
      install_requires=INSTALL_REQUIRES,
#      setup_requires=SETUP_REQUIRES,
      author_email='benjamin.loos.001@student.uni.lu, christian.hammerschmidt@uni.lu',
      package_dir = {'flexfringe': 'flexfringe'},
      zip_safe=False,
      #package_data = { 'dfasat': ["lib/dfasat.so", "lib/dfasat.lib.so"] },
      cmdclass = { 'build': DFASATBuild },
      )
