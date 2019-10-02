from remotesystem import RemoteSystem
import global_object_map as names
import squish
import test


def remove_file(file_path):
    remote_sys = RemoteSystem()
    if remote_sys.exists(file_path):
        test.log(f'[Test Setup] Remove existing file "{file_path}"')
        remote_sys.deleteFile(file_path)
    else:
        test.log(f'[Test Setup] File "{file_path}" doesn\'t exist')


def file_exists(file_path):
    remote_sys = RemoteSystem()
    return True if remote_sys.exists(file_path) else False


def start_qgc(remove_settings=True):
    test.startSection("[Test Setup] Starting the QGroundControl Ground Control Station")
    if remove_settings:
        remote_path = (
            RemoteSystem().getEnvironmentVariable("HOME")
            + "/.config/qgroundcontrol.org/CustomQGC.ini"
        )
        remove_file(remote_path)
    test.log("[Test Setup] Launch run_qgc.sh")
    squish.startApplication("run_qgc.sh")
    squish.waitForObject(names.o_icon_Image)
    test.log("[Test Setup] Application started")
    test.endSection()


def start_headless_emulator():
    test.startSection("Start headless simulator")
    try:
        remote_sys = RemoteSystem()
    except:
        raise
    local_path = squish.findFile("scripts", "scripts/run_simulator_headless.sh")
    remote_path = (
        remote_sys.getEnvironmentVariable("HOME") + "/run_simulator_headless.sh"
    )

    if remote_sys.exists(remote_path):
        test.log(f'Remove existing remote "{remote_path}"')
        remote_sys.deleteFile(remote_path)
    test.log(f'Upload local "{local_path}" to remote "{remote_path}"')
    remote_sys.upload(local_path, remote_path)
    test.log(f'Set execution rights to remote "{remote_path}"')
    remote_sys.execute(["chmod", "a+x", remote_path])
    test.endSection()
