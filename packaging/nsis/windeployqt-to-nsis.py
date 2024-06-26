#!/bin/env python3

# windeployqt-to-nsh
#
# Windeployqt-to-nsh - Convert the output of windeployqt to an equivalent set of
# NSIS "File" function calls.
#
# Rewritten in python from windeployqt-to-nsis.ps1, that has the following copyright:
#
# Copyright 2014 Gerald Combs <gerald@wireshark.org>
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# SPDX-License-Identifier: GPL-2.0-or-later

import sys
import os
import argparse
import subprocess

parser = argparse.ArgumentParser()
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument('--mapping')
group.add_argument('--executable')
parser.add_argument('--sysroot')
parser.add_argument('outfile')
args = parser.parse_args()

if args.mapping:
    if not args.sysroot:
        sys.exit('Option --sysroot is required with option --mapping')
    qt_version = None
    with open(args.mapping, 'r', encoding='utf-8') as f:
        out = f.read()
else:
    # Qt version
    qmake_command = [
        'qmake6.exe',
        '-query', 'QT_VERSION'
    ]
    qmake_out = subprocess.run(qmake_command, check=True, capture_output=True, encoding="utf-8")
    qt_version = qmake_out.stdout.strip()

    # XXX The powershell script asserts that the Qt version is greater than 5.3. We already require Qt6 to build the
    # installer using MSYS2 (currently not enforced).

    # Windeployqt output
    windeploy_command = [
        "windeployqt6.exe",
        "--no-compiler-runtime",
        "--no-translations",
        "--list", "mapping",
        args.executable
    ]
    out = subprocess.run(windeploy_command, check=True, capture_output=True, encoding="utf-8").stdout

with open(args.outfile, 'w') as f:
    header = """\
#
# Automatically generated by {}
#""".format(parser.prog)

    if qt_version:
        header += """\
# Qt version {}
#""".format(qt_version)

    print(header, file=f)

    current_dir = ""
    for line in out.splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if line.startswith('Adding in plugin'):
            # https://bugreports.qt.io/browse/QTBUG-122257
            # Affects 6.6.0 - 6.6.2
            continue
        path, relative = line.split(" ")
        rel_path = os.path.split(relative)
        if len(rel_path) > 1:
            base_dir = rel_path[0].strip('"')
            if base_dir != current_dir:
                set_out_path = r'SetOutPath "$INSTDIR\{}"'.format(base_dir)
                print(set_out_path, file=f)
                current_dir = base_dir

        path = path.strip('"')
        if args.sysroot:
            path = os.path.join(args.sysroot, path)
            if args.mapping and not os.path.isfile(path):
                # This hack is because Qt 6.7 renamed QWindowsVistaStylePlugin
                # to QModernWindowsStylePlugin. (This explicit mapping because
                # windeployqt6 doesn't work well with cross-compiling is
                # brittle.)
                continue
        file_path = 'File "{}"'.format(path)
        print(file_path, file=f)

