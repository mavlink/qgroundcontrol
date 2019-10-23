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


def upload_file(local_path, remote_path, force=True):
    try:
        rs = RemoteSystem()
    except:
        raise
    if force and rs.exists(remote_path):
        test.log(f'[Test Setup] Remove existing remote "{remote_path}"')
        rs.deleteFile(remote_path)
    test.log(f'[Test Setup] Upload local "{local_path}" to remote "{remote_path}"')
    rs.upload(local_path, remote_path)


def start_qgc(clear_settings=True):
    test.startSection("[Test Setup] Starting the QGroundControl Ground Control Station")
    aut = "run_qgc.sh --clear-settings" if clear_settings else "run_qgc.sh"
    test.log(f"[Test Setup] Launch {aut}")
    squish.startApplication(aut)
    squish.waitForObject(names.o_icon_Image)
    test.log("[Test Setup] Application started")
    test.endSection()


def start_headless_simulator():
    test.startSection("[Test Setup] Start headless simulator")
    try:
        rs = RemoteSystem()
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
    squish.waitForObjectExists(names.vehicle_1_Text, 45000)
    test.log("[Test Setup] Simulator started")
    test.endSection()


def confirm_with_slider():
    test.log("Confirm with a slider")
    squish.mouseDrag(
        squish.waitForObject(names.sliderDragArea_QGCMouseArea),
        150,
        23,
        300,
        23,
        squish.Qt.NoModifier,
        squish.Qt.LeftButton,
    )


def compare_position(pos1, pos2, epsilon=1):
    return (abs(pos1[0] - pos2[0]) <= epsilon) and (abs(pos1[1] - pos2[1]) <= epsilon)
