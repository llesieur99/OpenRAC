#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 the OpenRAC contributors
"""Build the native iPhone/iPad application; keep discs and build output private."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--moltenvk", type=Path, required=True,
                        help="MoltenVK.framework from the iOS ARM64 XCFramework slice")
    parser.add_argument("--disc", type=Path, required=True, help="Your Ratchet & Clank NTSC-U ISO")
    parser.add_argument("--python-xcframework", type=Path, required=True, help="Python 3.14 iOS XCFramework")
    parser.add_argument("--launcher-core", type=Path, required=True, help="Shared launcher core iOS static library")
    parser.add_argument("--team", default="")
    parser.add_argument("--bundle-id", default="ca.lesieur.openrac")
    parser.add_argument("--jobs", type=int, default=6)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    if not shutil.which("cmake") or not shutil.which("ninja"):
        parser.error("Put CMake and Ninja on PATH before building.")
    if not (args.moltenvk / "MoltenVK").is_file() or not args.disc.is_file():
        parser.error("The iOS framework and your disc image must exist.")
    subprocess.run([
        "cmake", "-S", str(root / "port"), "-B", str(args.build_dir.resolve()), "-G", "Ninja",
        "-DCMAKE_SYSTEM_NAME=iOS", "-DCMAKE_OSX_SYSROOT=iphoneos", "-DCMAKE_OSX_ARCHITECTURES=arm64",
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=17.0", "-DCMAKE_BUILD_TYPE=Release",
        "-DOPENRAC_STATIC_SDL=ON", "-DOPENRAC_ENHANCED_RENDERING=OFF",
        f"-DPython3_EXECUTABLE={sys.executable}",
        f"-DOPENRAC_IOS_MOLTENVK_FRAMEWORK={args.moltenvk.resolve()}",
        f"-DOPENRAC_IOS_PYTHON_XCFRAMEWORK={args.python_xcframework.resolve()}",
        f"-DOPENRAC_IOS_LAUNCHER_CORE={args.launcher_core.resolve()}",
        f"-DOPENRAC_RAC1_NTSC_RETAIL_IMAGE={args.disc.resolve()}",
        f"-DOPENRAC_IOS_TEAM={args.team}", f"-DOPENRAC_IOS_BUNDLE_ID={args.bundle_id}",
    ], check=True)
    subprocess.run(["cmake", "--build", str(args.build_dir.resolve()), "--target", "openrac-rac1-ntsc",
                    "--parallel", str(args.jobs)], check=True)
    print(f"Built {args.build_dir.resolve() / 'OpenRAC.app'} (sign before installing).")


if __name__ == "__main__":
    main()
