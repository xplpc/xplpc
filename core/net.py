from core import config as c
from core import run


# -----------------------------------------------------------------------------
def serve(root_path):
    run_args = [
        "python3",
        "-m",
        "http.server",
        f"{c.http_server_port}",
        "--bind",
        f"{c.http_server_host}",
    ]

    run.run(run_args, cwd=root_path)
