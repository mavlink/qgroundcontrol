from remotesystem import RemoteSystem
import squish
import test
import fnmatch
import os

SQUISHRUNNER_HOST = os.environ["SQUISHRUNNER_HOST"]
SQUISHRUNNER_PORT = os.environ["SQUISHRUNNER_PORT"]


def remove_file(file_path):
    remote_sys = RemoteSystem(SQUISHRUNNER_HOST, SQUISHRUNNER_PORT)
    if remote_sys.exists(file_path):
        test.log(f'[Test Setup] Remove existing file "{file_path}"')
        remote_sys.deleteFile(file_path)
    else:
        test.log(f'[Test Setup] File "{file_path}" doesn\'t exist')


def expand_remote_envs(path):
    remote_sys = RemoteSystem(SQUISHRUNNER_HOST, SQUISHRUNNER_PORT)
    return "/".join(
        [
            remote_sys.getEnvironmentVariable(lvl[1:]) if lvl.startswith("$") else lvl
            for lvl in path.split("/")
        ]
    )


def delete_logs_from_dir(dir_path):
    test.startSection(f'Remove log files from "{dir_path}"')
    try:
        dir_path = expand_remote_envs(dir_path)
        remote_sys = RemoteSystem(SQUISHRUNNER_HOST, SQUISHRUNNER_PORT)
        test.log(f'Expanded path is "{dir_path}"')
        for file in remote_sys.listFiles(dir_path):
            if file.startswith("log_") and file.endswith(".ulg"):
                full_path = dir_path + "/" + file
                test.log(f'Remove "{full_path}"')
                remote_sys.deleteFile(full_path)
    finally:
        if file_exists(dir_path + "/log_*.ulg"):
            test.fatal(f'There should be no log_*.ulg log files in "{dir_path}"')
        test.endSection()


def file_exists(file_path):
    remote_sys = RemoteSystem(SQUISHRUNNER_HOST, SQUISHRUNNER_PORT)
    file_path = expand_remote_envs(file_path)
    dir_name = os.path.dirname(file_path)
    for file_on_remote in remote_sys.listFiles(dir_name):
        if fnmatch.fnmatch(dir_name + "/" + file_on_remote, file_path):
            return True
    return False


def upload_file(local_path, remote_path, force=True):
    try:
        rs = RemoteSystem(SQUISHRUNNER_HOST, SQUISHRUNNER_PORT)
    except:
        raise
    if force and rs.exists(remote_path):
        test.log(f'[Test Setup] Remove existing remote "{remote_path}"')
        rs.deleteFile(remote_path)
    test.log(f'[Test Setup] Upload local "{local_path}" to remote "{remote_path}"')
    rs.upload(local_path, remote_path)


def is_sufficiently_sized(obj):
    return obj.width >= obj.implicitWidth and obj.height >= obj.implicitHeight


def start_headless_simulator():
    test.startSection("[Test Setup] Start headless simulator")
    try:
        rs = RemoteSystem(SQUISHRUNNER_HOST, SQUISHRUNNER_PORT)
    except:
        raise
    local_path = squish.findFile("scripts", "scripts/run_simulator_headless.sh")
    remote_path = rs.getEnvironmentVariable("HOME") + "/run_simulator_headless.sh"

    if rs.exists(remote_path):
        test.log(f'[Test Setup] Remove existing remote "{remote_path}"')
        rs.deleteFile(remote_path)
    test.log(f'[Test Setup] Upload local "{local_path}" to remote "{remote_path}"')
    rs.upload(local_path, remote_path)
    test.log(f'[Test Setup] Set execution rights to remote "{remote_path}"')
    rs.execute(["chmod", "a+x", remote_path])
    test.log(f'[Test Setup] Launch simulator "{remote_path}"')

    cmd = f"{remote_path}"
    exitcode, stdout, stderr = rs.execute([remote_path])
    squish.waitForObjectExists(ToolbarPO.vehicle_1_Text, 45000)
    test.log("[Test Setup] Simulator started")
    test.endSection()


def compare_position(pos1, pos2, epsilon=1):
    return (abs(pos1[0] - pos2[0]) <= epsilon) and (abs(pos1[1] - pos2[1]) <= epsilon)
