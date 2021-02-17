from setuptools import setup, Extension, find_packages

from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir

import sys 

__version__ = "0.0.1"


ext_modules = [
	Pybind11Extension("_hierarchical_umap",
		["src/cpp/external/efanna/index.cpp", "src/cpp/external/efanna/index_graph.cpp", "src/cpp/external/efanna/index_kdtree.cpp", "src/cpp/external/efanna/index_random.cpp", "src/cpp/utils.cpp", "src/cpp/umap.cpp", "src/cpp/hierarchical_umap.cpp", "src/cpp/umap_binding.cpp"],
		language='c++',
		extra_compile_args = ['-O3', '-shared', '-std=c++11', '-fPIC', '-fopenmp', '-DEIGEN_DONT_PARALLELIZE', '-march=native', '-DINFO'],
        extra_link_args = ['-O3', '-shared', '-std=c++11', '-fPIC', '-fopenmp', '-DEIGEN_DONT_PARALLELIZE', '-march=native', '-DINFO'],
		define_macros = [('VERSION_INFO', __version__)],
		),

]


setup(
    name="humap",
    version=__version__,
    author="Wilson E. Marcílio-Jr",
    author_email="wilson_jr@outlook.com",
    url="https://github.com/wilsonjr/hierarchical_umap",
    description="Hierarchical Uniform Manifold Approximation and Projection",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={"build_ext": build_ext},
    install_requires=['numpy', 'pybind11'],
    packages=['humap'],
    zip_safe=False,
)