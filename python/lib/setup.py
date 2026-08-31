from setuptools import setup
from setuptools.command.bdist_wheel import bdist_wheel


class BinaryDistribution(bdist_wheel):
    # The package carries a native library for one platform, so the wheel has to say which one
    # rather than claim it runs anywhere and fail when a call reaches the library. Nothing here is
    # compiled against an interpreter, so the python tag stays the one every version 3 accepts.
    def finalize_options(self):
        super().finalize_options()

        self.root_is_pure = False

    def get_tag(self):
        _, _, platform = super().get_tag()

        return "py3", "none", platform


setup(cmdclass={"bdist_wheel": BinaryDistribution})
