from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool


# -----------------------------------------------------------------------------
def run_task_setup():
    # check
    tool.check_tool_conan()

    # environment
    l.i("Creating default profile...")

    r.run(
        [
            "conan",
            "profile",
            "new",
            "default",
            "--detect",
            "--force",
        ],
        cwd=c.proj_path,
    )

    l.ok()
