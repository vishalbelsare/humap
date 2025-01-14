.. -*- mode: rst -*-

|conda_version|_ |conda_downloads|_ |pypi_version|_ |pypi_downloads|_

.. |pypi_version| image:: https://img.shields.io/pypi/v/humap.svg
.. _pypi_version: https://pypi.python.org/pypi/humap/

.. |pypi_downloads| image:: https://pepy.tech/badge/humap
.. _pypi_downloads: https://pepy.tech/project/humap

.. |conda_version| image:: https://anaconda.org/conda-forge/humap/badges/version.svg
.. _conda_version: https://anaconda.org/conda-forge/humap

.. |conda_downloads| image:: https://anaconda.org/conda-forge/humap/badges/downloads.svg
.. _conda_downloads: https://anaconda.org/conda-forge/humap

.. image:: images/fmnist-cover.png
	:alt: HUMAP exploration on Fashion MNIST dataset

=====
HUMAP
=====

Hierarchical Manifold Approximation and Projection (HUMAP) is a technique based on `UMAP <https://github.com/lmcinnes/umap/>`_ for hierarchical non-linear dimensionality reduction. HUMAP allows to:


1. Focus on important information while reducing the visual burden when exploring whole datasets;
2. Drill-down the hierarchy according to information demand.

The details of the algorithm can be found in our paper on `ArXiv <https://arxiv.org/abs/2106.07718>`_. This repository also features a C++ UMAP implementation.


-----------
Installation
-----------

HUMAP was written in C++ for performance purposes, and it has an intuitive Python interface. It depends upon common machine learning libraries, such as ``scikit-learn`` and ``NumPy``. It also needs the ``pybind11`` due to the interface between C++ and Python.


Requirements:

* Python 3.6 or greater
* numpy
* scipy
* scikit-learn
* pybind11
* pynndescent (for reproducible results)
* Eigen (C++)

If you have these requirements installed, use PyPI:

.. code:: bash

    pip install humap
    
Alternatively (and preferable), you can use conda to install:

.. code:: bash

    conda install humap


**For Windows users**:

If using *pip* The `Eigen <https://eigen.tuxfamily.org/>`_ library does not have to be installed. Just add the files to C:\\Eigen or use the manual installation to change Eigen location.

**Manual installation**: 

For manually installing HUMAP, download the project and proceed as follows:

.. code:: bash
 	
 	python setup.py bdist_wheel

.. code:: bash

 	pip install dist/humap*.whl


--------------
Usage examples
--------------

HUMAP package follows the same idea of sklearn classes, in which you need to fit and transform data.

**Fitting the hierarchy**

.. code:: python

	import humap
	from sklearn.datasets import fetch_openml


	X, y = fetch_openml('mnist_784', version=1, return_X_y=True)

	hUmap = humap.HUMAP()
	hUmap.fit(X, y)

.. image:: images/mnist_top.png
	:alt: HUMAP embedding of top-level MNIST digits

By now, you can control six parameters related to the hierarchy construction and the embedding performed by UMAP.

 -  ``levels``: Controls the number of hierarchical levels + the first one (whole dataset). This parameter also controls how many data points are in each hierarchical level. The default is ``[0.2, 0.2]``, meaning the HUMAP will produce three levels: The first one with the whole dataset, the second one with 20% of the first level, and the third with 20% of the second level.

 -  ``n_neighbors``: This parameter controls the number of neighbors for approximating the manifold structures. Larger values produce embedding that preserves more of the global relations. In HUMAP, we recommend and set the default value to be ``100``.

 -  ``min_dist``: This parameter, used in UMAP dimensionality reduction, controls the allowance to cluster data points together. According to UMAP documentation, larger values allow evenly distributed embeddings, while smaller values encode the local structures better. We set this parameter as ``0.15`` as default.

 -  ``knn_algorithm``: Controls which knn approximation will be used, in which ``NNDescent`` is the default. Another option is ``ANNOY`` or ``FLANN`` if you have Python installations of these algorithms at the expense of slower run-time executions than NNDescent.

 -  ``init``: Controls the method for initing the low-dimensional representation. We set ``Spectral`` as default since it yields better global structure preservation. You can also use ``random`` initialization.

 -  ``verbose``: Controls the verbosity of the algorithm.


**Embedding a hierarchical level**

After fitting the dataset, you can generate the embedding for a hierarchical level by specifying the level.

.. code:: python

	embedding_l2 = hUmap.transform(2)
	y_l2 = hUmap.labels(2)

Notice that the ``.labels()`` method only works for levels equal or greater than one.


**Drilling down the hierarchy by embedding a subset of data points based on indices**

.. image:: images/example_drill.png
	:alt: Embedding data subsets throughout HUMAP hierarchy

When interested in a set of data samples, HUMAP allows for drilling down the hierarchy for those samples.


.. code:: python

	embedding, y, indices = hUmap.transform(2, indices=indices_of_interest)

This method returns the ``embedding`` coordinates, the labels (``y``), and the data points' ``indices`` in the current level. Notice that the current level is now level 1 since we used the hierarchy level ``2`` for drilling down operation.


**Drilling down the hierarchy by embedding a subset of data points based on labels**

You can apply the same concept as above to embed data points based on labels. 

.. code:: python	

	embedding, y, indices = hUmap.transform(2, indices=np.array([4, 9]), class_based=True)


**C++ UMAP implementation**

You can also fit a one-level HUMAP hierarchy, which essentially corresponds to a UMAP projection.

.. code:: python

	umap_reducer = humap.UMAP()
	embedding = umap_reducer.fit_transform(X)

--------
Citation
--------

Please, use the following reference to cite HUMAP in your work:

.. code:: bibtex

    @misc{marciliojr_humap2021,
      title={HUMAP: Hierarchical Uniform Manifold Approximation and Projection}, 
      author={Wilson E. Marcílio-Jr and Danilo M. Eler and Fernando V. Paulovich and Rafael M. Martins},
      year={2021},
      eprint={2106.07718},
      archivePrefix={arXiv},
      primaryClass={cs.LG}
	}


-------
License
-------

HUMAP follows the 3-clause BSD license and it uses the open-source NNDescent implementation from `EFANNA <https://github.com/ZJULearning/efanna>`_. It also uses a C++ implementation of `UMAP <http://github.com/lmcinnes/umap>`_ for embedding hierarchy levels; this project would not be possible without UMAP's fantastic technique and package.

E-mail me (wilson_jr at outlook.com) if you like to contribute.


......
