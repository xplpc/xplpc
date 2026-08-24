from pygemstones.system import runner as r
from pygemstones.util import log as l


def run(args, shell=False, cwd=None, env=None, silent=False):
    # A process killed by a signal answers a negative code, which reads as success to anything that only checks for a positive one.

    code = r.run(args, shell=shell, cwd=cwd, env=env, silent=silent)

    if code < 0:
        if not isinstance(args, str):
            args = " ".join(args)

        l.e(f"Command was killed by signal {-code}: {args}")

    return code
