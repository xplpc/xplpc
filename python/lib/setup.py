from setuptools import find_packages, setup

setup(
    name="xplpc",
    version="1.0.0",
    description="Cross Platform Lite Procedure Call",
    long_description="Cross Platform Lite Procedure Call - The XPLPC project connects languages and platforms, allowing for the transfer of serialized data between them",
    long_description_content_type="text/markdown",
    author="Paulo Coutinho",
    author_email="paulocoutinhox@gmail.com",
    url="https://github.com/xplpc/xplpc",
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    include_package_data=True,
    package_data={"": ["lib/**/*"]},
)
