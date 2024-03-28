import os
from distutils.core import setup, Extension

cpp_args = ['-std=c++17', '-lpthread']

README_PATH = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'README.md')
with open(README_PATH) as readme_file:
    README = readme_file.read()

ext_modules = [
    Extension(
        'eratosthenes',
        ['src/prime_gen.cpp', "src/dispatchqueue.cpp"],
        include_dirs=['src/include', 'pybind11/include'],
        language='c++',
        extra_compile_args = cpp_args,
    ),
]

setup(
    name='Eratosthenes',
    version='2.1.4',
    author='Dan Strano',
    author_email='dan@unitary.fund',
    description='Fast prime generation for Python based on Sieve of Eratosthenes and Trial Division',
    long_description=README,
    long_description_content_type='text/markdown',
    url="https://github.com/vm6502q/Eratosthenes",
    license="MIT",
    classifiers=[
        "Environment :: Console",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Topic :: Scientific/Engineering",
    ],
    ext_modules=ext_modules,
)
