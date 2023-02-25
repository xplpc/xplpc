import os

from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool


# -----------------------------------------------------------------------------
def run_task_test():
    # check
    tool.check_tool_flutter()

    # build
    l.i("Testing...")
    project_dir = os.path.join(c.proj_path, "flutter", "plugin")

    # dependencies
    r.run(["flutter", "pub", "get"], cwd=project_dir)

    # build runner
    r.run(
        [
            "flutter",
            "pub",
            "run",
            "build_runner",
            "build",
            "--delete-conflicting-outputs",
        ],
        cwd=project_dir,
    )

    # test
    r.run(["flutter", "test"], cwd=project_dir)

    l.ok()
