import sys
from setuptools import setup

setup(name="heimdalite",
    version="0.1",
    description="Control of heimdallr simulation and prototype of NOTT control",
    url="--",
    author="Romain Laugier",
    author_email="romain.laugier@kuleuven.be",
    license="BSD-3-Clause",
    classifiers=[
      'Development Status :: 2 - Pre-alpha',
      'Intended Audience :: Professional Astronomers',
      'Topic :: High Angular Resolution Astronomy :: Interferometry :: High-contrast',
      'Programming Language :: Python :: 3.10'
    ],
    packages=["heimdalite"],
    install_requires=[
            "numpy", "scipy", "matplotlib", "configparser", "tqdm", "jax", "dnull",
            "pyserial", "threading"
    ],
    zip_safe=False)
