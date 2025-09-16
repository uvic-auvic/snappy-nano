from setuptools import find_packages
from setuptools import setup

setup(
    name='rviz_resource_interfaces',
    version='15.1.8',
    packages=find_packages(
        include=('rviz_resource_interfaces', 'rviz_resource_interfaces.*')),
)
