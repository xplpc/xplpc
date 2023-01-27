import os

from pygemstones.system import runner as r
from pygemstones.util import log as l

from core import config as c
from core import tool, util


# -----------------------------------------------------------------------------
def run_task_format():
    # check
    tool.check_tool_remark()

    # format
    path_list = [
        {
            "path": os.path.join(c.proj_path, "README.md"),
        },
        {
            "path": os.path.join(c.proj_path, "docs"),
            "patterns": ["*.md"],
        },
    ]

    if path_list:
        l.i("Formatting Markdown files...")

        util.run_format(
            path_list=path_list,
            formatter=lambda file_item: r.run(
                [
                    "remark",
                    file_item,
                    "-o",
                    file_item,
                ],
                cwd=c.proj_path,
            ),
            ignore_path_list=[],
        )

        l.ok()
    else:
        l.i("No Markdown files found to format")
