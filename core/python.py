import os

from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool, util


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_python_formatter()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "xplpc.py"),
        },
        {
            "path": os.path.join(c.proj_path, "core"),
            "patterns": ["*.py"],
        },
    ]

    if path_list:
        l.i("Formatting Python files...")

        util.run_format(
            path_list=path_list,
            formatter=lambda file_item: r.run(
                [
                    "black",
                    "-q",
                    file_item,
                ],
                cwd=c.proj_path,
            ),
            ignore_path_list=[],
        )

        l.ok()
    else:
        l.i("No Python files found to format")
