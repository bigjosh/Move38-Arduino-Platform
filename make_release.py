#!/usr/bin/env python3
"""
Build a Boards Manager release of this platform and update the package index.

    python make_release.py 1.4.0

What it does:
  1. Checks the git working tree is clean and that platform.txt `version=` matches.
  2. Builds Move38-Arduino-Platform-<version>.zip with `git archive`, so the zip
     contains exactly the committed files (no .git folder, no local build output),
     with LF line endings and normal Unix permissions, under a single top folder.
  3. Computes the exact size and SHA-256 and adds a new entry to the top of
     package_move38.com-blinks_index.json, keeping the older entries so people
     can still pick earlier versions in Boards Manager.

Afterwards:
  - Commit the updated index JSON and tag the commit with the version.
  - Create a GitHub release with that tag and attach the zip.
  - Copy the index JSON to https://boardsmanager.com/package_move38.com-blinks_index.json
"""
import hashlib
import io
import json
import os
import re
import subprocess
import sys
import tarfile
import time
import zipfile

REPO = "bigjosh/Move38-Arduino-Platform"
INDEX = "package_move38.com-blinks_index.json"
PREFIX = "Move38-Arduino-Platform"


def git(*args):
    return subprocess.check_output(["git", *args], text=True).strip()


def build_zip(zip_name):
    """Zip the last commit (honoring export-ignore in .gitattributes) with Unix
    permissions set explicitly, so the files are readable on macOS and Linux."""
    tar_bytes = subprocess.check_output(["git", "archive", "--format=tar", f"--prefix={PREFIX}/", "HEAD"])
    with tarfile.open(fileobj=io.BytesIO(tar_bytes)) as tar, \
            zipfile.ZipFile(zip_name, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for m in tar.getmembers():
            if m.isdir():
                info = zipfile.ZipInfo(m.name.rstrip("/") + "/", time.localtime(m.mtime)[:6])
                info.external_attr = (0o40755 << 16) | 0x10
                info.create_system = 3  # Unix, so extractors honor the mode
                z.writestr(info, b"")
            elif m.isfile():
                info = zipfile.ZipInfo(m.name, time.localtime(m.mtime)[:6])
                info.external_attr = (0o100755 if m.mode & 0o111 else 0o100644) << 16
                info.create_system = 3
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, tar.extractfile(m).read())


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"\d+\.\d+\.\d+", sys.argv[1]):
        sys.exit("usage: python make_release.py X.Y.Z")
    version = sys.argv[1]
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    if git("status", "--porcelain"):
        sys.exit("Commit or stash your changes first; the zip is built from the last commit.")

    m = re.search(r"^version=(.+)$", open("platform.txt").read(), re.M)
    if not m or m.group(1).strip() != version:
        sys.exit(f"Set version={version} in platform.txt and commit it first.")

    index = json.load(open(INDEX))
    platforms = index["packages"][0]["platforms"]
    if any(p["version"] == version for p in platforms):
        sys.exit(f"{version} is already in {INDEX}.")

    zip_name = f"{PREFIX}-{version}.zip"
    build_zip(zip_name)
    data = open(zip_name, "rb").read()

    entry = json.loads(json.dumps(platforms[0]))  # copy the latest entry as a template
    entry.update({
        "version": version,
        "url": f"https://github.com/{REPO}/releases/download/{version}/{zip_name}",
        "archiveFileName": zip_name,
        "checksum": "SHA-256:" + hashlib.sha256(data).hexdigest(),
        "size": str(len(data)),
    })
    platforms.insert(0, entry)

    with open(INDEX, "w", newline="\n") as f:
        json.dump(index, f, indent=2)
        f.write("\n")

    print(f"Built {zip_name} ({len(data)} bytes)")
    print(f"Added {version} to {INDEX}")


if __name__ == "__main__":
    main()
