from pygemstones.system import runner as r

from core import config as c


# -----------------------------------------------------------------------------
def serve(root_path):
    run_args = [
        "python3",
        "-m",
        "http.server",
        "{0}".format(c.http_server_port),
        "--bind",
        "{0}".format(c.http_server_host),
    ]

    r.run(run_args, cwd=root_path)
