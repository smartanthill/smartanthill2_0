# Copyright (C) 2015 OLogN Technologies AG
#
# This source file is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

from setuptools import find_packages, setup

from smartanthill import (__author__, __description__, __email__, __license__,
                          __title__, __url__, __version__)

setup(
    name=__title__,
    version=__version__,
    description=__description__,
    long_description=open("README.rst").read(),
    author=__author__,
    author_email=__email__,
    url=__url__,
    license=__license__,
    install_requires=[
        # "smartanthill_zc",
        "platformio>=2.3.1",
        "pyserial",
        "twisted>=14.1",
        "txws",
        "six",  # required by txws but not listed in it's requirements
    ],
    packages=find_packages() + ["twisted.plugins"],
    include_package_data=True,
    entry_points={
        "console_scripts": [
            "smartanthill = smartanthill.__main__:main"
        ]
    },
    classifiers=[
        "Development Status :: 2 - Pre-Alpha",
        "Environment :: Console",
        "Environment :: Web Environment",
        "Framework :: Twisted",
        "Intended Audience :: Customer Service",
        "Intended Audience :: Developers",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: Information Technology",
        "Intended Audience :: Manufacturing",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",
        "Operating System :: OS Independent",
        "Programming Language :: C",
        "Programming Language :: JavaScript",
        "Programming Language :: Python",
        "Topic :: Adaptive Technologies",
        "Topic :: Communications",
        "Topic :: Home Automation",
        "Topic :: Internet",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Human Machine Interfaces",
        "Topic :: Scientific/Engineering :: Interface Engine/Protocol"
        "Translator",
        "Topic :: Software Development :: Compilers",
        "Topic :: Software Development :: Embedded Systems",
        "Topic :: System :: Distributed Computing",
        "Topic :: System :: Networking",
        "Topic :: Terminals :: Serial"
    ]
)
