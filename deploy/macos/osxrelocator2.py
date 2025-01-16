#!/usr/bin/env python3
# cerbero - a multi-platform build system for Open Source software
# Copyright (C) 2012 Andoni Morales Alastruey <ylatuya@gmail.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

import os
import subprocess

INT_CMD = 'install_name_tool'
OTOOL_CMD = 'otool'


class CerberoException(Exception):
    header = ''
    msg = ''

    def __init__(self, msg=''):
        self.msg = msg
        Exception.__init__(self, self.header + msg)


class FatalError(CerberoException):
    header = 'Fatal Error: '

    def __init__(self, msg='', arch=''):
        self.arch = arch
        CerberoException.__init__(self, msg)


class CommandError(FatalError):
    header = 'Command Error: '

    def __init__(self, msg, cmd, returncode):
        msg = 'Running {!r} returned {}\n{}'.format(cmd, returncode, msg or '')
        FatalError.__init__(self, msg)


def _resolve_cmd(cmd, env):
    """
    On Windows, we can't pass the PATH variable through the env= kwarg to
    subprocess.* and expect it to use that value to search for the command,
    because Python uses CreateProcess directly. Unlike execvpe, CreateProcess
    does not use the PATH env var in the env supplied to search for the
    executable. Hence, we need to search for it manually.
    """
    if PLATFORM != Platform.WINDOWS or env is None or 'PATH' not in env:
        return cmd
    if not os.path.isabs(cmd[0]):
        resolved_cmd = shutil.which(cmd[0], path=env['PATH'])
        if not resolved_cmd:
            raise FatalError('Could not find {!r} in PATH {!r}'.format(cmd[0], env['PATH']))
        cmd[0] = resolved_cmd
    return cmd


def _cmd_string_to_array(cmd, env):
    if isinstance(cmd, list):
        return _resolve_cmd(cmd, env)
    assert isinstance(cmd, str)
    # If we've been given a string, run it through sh to get scripts working on
    # Windows and shell syntax such as && and env var setting working on all
    # platforms.
    return ['sh', '-c', cmd]


def shell_check_output(cmd, cmd_dir=None, fail=True, logfile=None, env=None, quiet=False):
    cmd = _cmd_string_to_array(cmd, env)
    stderr = logfile
    if quiet and not logfile:
        stderr = subprocess.DEVNULL
    if logfile:
        logfile.write(f'Running command {cmd!r} in {cmd_dir}\n')
        logfile.flush()

    try:
        o = subprocess.check_output(cmd, cwd=cmd_dir, env=env, stderr=stderr)
    except SUBPROCESS_EXCEPTIONS as e:
        msg = getattr(e, 'output', '')
        if isinstance(msg, bytes):
            msg = msg.decode(sys.stdout.encoding, errors='replace')
        if not fail:
            return msg
        if logfile:
            msg += '\nstderr in logfile {}'.format(logfile.name)
        raise CommandError(msg, cmd, getattr(e, 'returncode', -1))

    if sys.stdout.encoding:
        o = o.decode(sys.stdout.encoding, errors='replace')
    return o


def shell_new_call(
    cmd, cmd_dir=None, fail=True, logfile=None, env=None, verbose=False, interactive=False, shell=False, input=None
):
    cmd = _cmd_string_to_array(cmd, env)
    if logfile:
        if input:
            logfile.write(f'Running command {cmd!r} with stdin {input} in {cmd_dir}\n')
        else:
            logfile.write(f'Running command {cmd!r} in {cmd_dir}\n')
        logfile.flush()
    if verbose:
        m.message('Running {!r}\n'.format(cmd))
    if input:
        stdin = None
    elif not interactive:
        stdin = subprocess.DEVNULL
    else:
        stdin = None
    try:
        subprocess.run(
            cmd,
            cwd=cmd_dir,
            env=env,
            stdout=logfile,
            stderr=subprocess.STDOUT,
            stdin=stdin,
            input=input,
            shell=shell,
            check=True,
        )
    except SUBPROCESS_EXCEPTIONS as e:
        returncode = getattr(e, 'returncode', -1)
        if not fail:
            stream = logfile or sys.stderr
            if isinstance(e, FileNotFoundError):
                stream.write('{}: file not found\n'.format(cmd[0]))
            if isinstance(e, PermissionError):
                stream.write('{!r}: permission error\n'.format(cmd))
            return returncode
        msg = ''
        if logfile:
            msg = 'Output in logfile {}'.format(logfile.name)
        raise CommandError(msg, cmd, returncode)
    return 0


class OSXRelocator(object):
    """
    Wrapper for OS X's install_name_tool and otool commands to help
    relocating shared libraries.

    It parses lib/ /libexec and bin/ directories, changes the prefix path of
    the shared libraries that an object file uses and changes it's library
    ID if the file is a shared library.
    """

    def __init__(self, root, install_prefix, recursive, logfile=None):
        self.root = root
        self.install_prefix = self._fix_path(install_prefix)
        self.recursive = recursive
        self.use_relative_paths = True
        self.logfile = None

    def relocate(self):
        self.parse_dir(self.root)

    def relocate_dir(self, dirname):
        self.parse_dir(os.path.join(self.root, dirname))

    def relocate_file(self, object_file, original_file=None):
        self.change_libs_path(object_file, original_file)

    def change_id(self, object_file, id=None):
        """
        Changes the `LC_ID_DYLIB` of the given object file.
        @object_file: Path to the object file
        @id: New ID; if None, it'll be `@rpath/<basename>`
        """
        id = id or object_file.replace(self.install_prefix, '@rpath')
        if not self._is_mach_o_file(object_file):
            return
        cmd = [INT_CMD, '-id', id, object_file]
        shell_new_call(cmd, fail=False, logfile=self.logfile)

    def change_libs_path(self, object_file, original_file=None):
        """
        Sanitizes the `LC_LOAD_DYLIB` and `LC_RPATH` load commands,
        setting the former to be of the form `@rpath/libyadda.dylib`,
        and the latter to point to the /lib folder within the GStreamer prefix.
        @object_file: the actual file location
        @original_file: where the file will end up in the output directory
        structure and the basis of how to calculate rpath entries.  This may
        be different from where the file is currently located e.g. when
        creating a fat binary from copy of the original file in a temporary
        location.
        """
        if not self._is_mach_o_file(object_file):
            return
        if original_file is None:
            original_file = object_file
        # First things first: ensure the load command of future consumers
        # points to the real ID of this library
        # This used to be done only at Universal lipo time, but by then
        # it's too late -- unless one wants to run through all load commands
        # If the library isn't a dylib, it's a framework, in which case
        # assert that it's already rpath'd
        dylib_id = self.get_dylib_id(object_file)
        is_dylib = dylib_id is not None
        is_framework = is_dylib and not object_file.endswith('.dylib')
        if not is_framework:
            self.change_id(object_file, id='@rpath/{}'.format(os.path.basename(original_file)))
        elif '@rpath' not in dylib_id:
            raise FatalError(f'Cannot relocate a fixed location framework: {dylib_id}')
        # With that out of the way, we need to sort out how many parents
        # need to be navigated to reach the root of the GStreamer prefix
        depth = len(os.path.dirname(original_file).split('/')) - len(self.install_prefix.split('/'))
        p_depth = '/..' * depth
        # These paths assume that the file being relocated resides within
        # <GStreamer root>/lib
        rpaths = [
            # From a deeply nested library
            f'@loader_path{p_depth}/lib',
            # From a deeply nested framework or binary
            f'@executable_path{p_depth}/lib',
            # From a library within the prefix
            '@loader_path/../lib',
            # From a binary within the prefix
            '@executable_path/../lib',
        ]
        if depth > 1:
            rpaths += [
                # Allow loading from the parent (e.g. GIO plugin)
                '@loader_path/..',
                '@executable_path/..',
            ]
        if is_framework:
            # Start with framework's libraries
            rpaths = [
                '@loader_path/lib',
            ] + rpaths
        # Make them unique
        rpaths = list(set(rpaths))
        # Remove absolute RPATHs, we don't want or need these
        existing_rpaths = list(set(self.list_rpaths(object_file)))
        for p in filter(lambda p: p.startswith('/'), self.list_rpaths(object_file)):
            cmd = [INT_CMD, '-delete_rpath', p, object_file]
            shell_new_call(cmd, fail=False)
        # Add relative RPATHs
        for p in filter(lambda p: p not in existing_rpaths, rpaths):
            cmd = [INT_CMD, '-add_rpath', p, object_file]
            shell_new_call(cmd, fail=False)
        # Change dependencies' paths from absolute to @rpath/
        for lib in self.list_shared_libraries(object_file):
            new_lib = lib.replace(self.install_prefix, '@rpath').replace('@rpath/lib/', '@rpath/')
            # These are leftovers from meson thinking RPATH == prefix
            if new_lib == lib:
                continue
            cmd = [INT_CMD, '-change', lib, new_lib, object_file]
            shell_new_call(cmd, fail=False, logfile=self.logfile)

    def change_lib_path(self, object_file, old_path, new_path):
        for lib in self.list_shared_libraries(object_file):
            if old_path in lib:
                new_path = lib.replace(old_path, new_path)
                cmd = [INT_CMD, '-change', lib, new_path, object_file]
                shell_new_call(cmd, fail=True, logfile=self.logfile)

    def parse_dir(self, dir_path, filters=None):
        for dirpath, dirnames, filenames in os.walk(dir_path):
            for f in filenames:
                if filters is not None and os.path.splitext(f)[1] not in filters:
                    continue
                self.change_libs_path(os.path.join(dirpath, f))
            if not self.recursive:
                break

    @staticmethod
    def get_dylib_id(object_file):
        res = shell_check_output([OTOOL_CMD, '-D', object_file]).splitlines()
        return res[-1] if len(res) > 1 else None

    @staticmethod
    def list_shared_libraries(object_file):
        res = shell_check_output([OTOOL_CMD, '-L', object_file]).splitlines()
        # We don't use the first line
        libs = res[1:]
        # Remove the first character tabulation
        libs = [x[1:] for x in libs]
        # Remove the version info
        libs = [x.split(' ', 1)[0] for x in libs]
        return libs

    @staticmethod
    def list_rpaths(object_file):
        res = shell_check_output([OTOOL_CMD, '-l', object_file]).splitlines()
        i = iter(res)
        paths = []
        for line in i:
            if 'LC_RPATH' not in line:
                continue
            next(i)
            path_line = next(i)
            # Extract the path from a line that looks like this:
            #          path @loader_path/.. (offset 12)
            path = path_line.split('path ', 1)[1].split(' (offset', 1)[0]
            paths.append(path)
        return paths

    def _fix_path(self, path):
        if path.endswith('/'):
            return path[:-1]
        return path

    def _is_mach_o_file(self, filename):
        fileext = os.path.splitext(filename)[1]

        if '.dylib' in fileext:
            return True

        filedesc = shell_check_output(['file', '-bh', filename])

        if fileext == '.a' and 'ar archive' in filedesc:
            return False

        return filedesc.startswith('Mach-O')


class Main(object):
    def run(self):
        # We use OptionParser instead of ArgumentsParse because this script
        # might be run in OS X 10.6 or older, which do not provide the argparse
        # module
        import optparse

        usage = 'usage: %prog [options] library_path old_prefix new_prefix'
        description = (
            'Rellocates object files changing the dependant ' ' dynamic libraries location path with a new one'
        )
        parser = optparse.OptionParser(usage=usage, description=description)
        parser.add_option(
            '-r',
            '--recursive',
            action='store_true',
            default=False,
            dest='recursive',
            help='Scan directories recursively',
        )

        options, args = parser.parse_args()
        if len(args) != 3:
            parser.print_usage()
            exit(1)
        relocator = OSXRelocator(args[1], args[2], options.recursive)
        relocator.relocate_file(args[0])
        exit(0)


if __name__ == '__main__':
    main = Main()
    main.run()
