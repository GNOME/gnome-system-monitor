#!/bin/env python3

# Based on https://gitlab.gnome.org/GNOME/mutter/-/blob/main/check-style.py

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

# Path relative to this script
uncrustify_cfg = 'tools/gtk.cfg'


def check_progs():
    git = shutil.which('git')
    patch = shutil.which('patch')
    uncrustify = shutil.which('uncrustify')

    return git is not None and uncrustify is not None and patch is not None


def find_files(sha, all):
    if all:
        files = []
        for dirpath, dirnames, filenames in os.walk('src'):
            for filename in filenames:
                files.append(os.path.join(dirpath, filename))
    else:
        proc = subprocess.Popen(
            ["git", "diff", "--name-only", sha, "HEAD"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
        files = proc.stdout.read().strip().decode('utf-8')
        files = files.split('\n')

    files_filtered = []
    for file in files:
        if file.endswith('.cpp') or file.endswith('.c') or file.endswith('.h'):
            files_filtered.append(file)

    return files_filtered


def reformat_files(files, rewrite):
    changed = None

    for file in files:
        # uncrustify chunk
        proc = subprocess.Popen(
            ["uncrustify", "-c", uncrustify_cfg, "-f", file],
            stdout=subprocess.PIPE)
        reformatted_raw = proc.stdout.readlines()
        proc.wait()
        if proc.returncode != 0:
            continue

        reformatted_tmp = tempfile.NamedTemporaryFile()
        for line in reformatted_raw:
            reformatted_tmp.write(line)
        reformatted_tmp.seek(0)

        if dry_run is True:
            # Show changes
            proc = subprocess.Popen(
                ["diff", "-up", "--color=always", file, reformatted_tmp.name],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT)
            diff = proc.stdout.read().decode('utf-8')
            if diff != '':
                output = re.sub('\t', '↦\t', diff)
                print(output)
                changed = True
        else:
            # Apply changes
            diff = subprocess.Popen(
                ["diff", "-up", file, reformatted_tmp.name],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT)
            patch = subprocess.Popen(["patch", file], stdin=diff.stdout)
            diff.stdout.close()
            patch.communicate()

        reformatted_tmp.close()

    return changed


parser = argparse.ArgumentParser(description='Check code style.')
parser.add_argument('--all', type=bool,
                    action=argparse.BooleanOptionalAction,
                    help='Format all source files')
parser.add_argument('--sha', metavar='SHA', type=str,
                    help='SHA for the commit to compare HEAD with')
parser.add_argument('--dry-run', '-d', type=bool,
                    action=argparse.BooleanOptionalAction,
                    help='Only print changes to stdout, do not change code')
parser.add_argument('--rewrite', '-r', type=bool,
                    action=argparse.BooleanOptionalAction,
                    help='Whether to amend the result to the last commit (e.g. \'git rebase --exec "%(prog)s -r"\')')

# Change CWD to script location, necessary for always locating the configuration file
os.chdir(os.path.dirname(os.path.abspath(sys.argv[0])))

args = parser.parse_args()
all = args.all
sha = args.sha or 'HEAD^'
rewrite = args.rewrite
dry_run = args.dry_run

if all and args.sha is not None:
    print("Flags --all and --sha are mutualy exclusive")
    sys.exit(1)

if check_progs() is False:
    print("Make sure git, patch and uncrustify are installed")
    sys.exit(1)

files = find_files(sha, all)
changed = reformat_files(files, rewrite)

if dry_run is not True and rewrite is True:
    proc = subprocess.Popen(
        ["git", "commit", "--all", "--amend", "-C", "HEAD"],
        stdout=subprocess.DEVNULL)
    sys.exit(0)
elif dry_run is True and changed is True:
    print("\nIssue the following command in your local tree to apply the suggested changes:\n\n $ git rebase origin/main --exec \"./check-style.py -r\" \n")
    sys.exit(2)

sys.exit(0)
