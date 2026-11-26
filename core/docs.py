import os

from pygemstones.util import log as l

from core import config as c
from core import run, tool, util


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

    l.i("Formatting Markdown files...")

    util.run_format(
        path_list=path_list,
        formatter=lambda file_item: run.run(
            [
                "remark",
                file_item,
                "-o",
                file_item,
            ],
            cwd=c.proj_path,
        ),
    )

    l.ok()
