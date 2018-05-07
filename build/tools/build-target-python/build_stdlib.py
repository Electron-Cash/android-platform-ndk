import argparse
import os.path
import zipfile


# Chaquopy: We omit things which are either non-importable, or both large and unnecessary.
# Don't omit importable things just because they're useless: user code may import things
# unconditionally and then use them conditionally.
IGNORE_PATHS = [
    # Replaced by our own copies below.
    'site.py',
    'sysconfig.py',

    # Not used in our setup.
    'site-packages/',

    # Depends on Tk, which we don't build.
    'idlelib/',
    'lib-tk/',      # 2.x only
    'tkinter/',     # 3.x only
    'turtle.py',    # 3.x only
    'turtledemo/',  # 3.x only

    # Depends on other native modules which we don't build.
    'anydbm.py',    # 2.x only
    'bsddb/',       # 2.x only
    'curses/',
    'dbm/',         # 3.x only
    'msilib/',
    'whichdb.py',   # 2.x only

    # Large and unnecessary. (All `test` and `tests` directories are automatically ignored
    # below, so no need to list them here.)
    'ensurepip/',
    'pydoc_data/',
]


def in_interest(fs_path, arch_path, is_dir, pathbits):
    name = pathbits[-1]
    if is_dir:
        if (name == '__pycache__' or name == 'test' or name == 'tests'):
            return False
        if arch_path.startswith('plat-'):
            return False
    else:
        if not arch_path.endswith('.py'):
            return False
    return not any(arch_path.startswith(exclusion) for exclusion in IGNORE_PATHS)


def enum_content(seed, catalog, pathbits = None):
    if pathbits is None:
        fs_path = seed
        is_dir = True
    else:
        fs_path = os.path.join(seed, *pathbits)
        is_dir = os.path.isdir(fs_path)
    if pathbits is not None:
        arc_path = '/'.join(pathbits)
        if not in_interest(fs_path, arc_path, is_dir, pathbits):
            return
        if not is_dir:
            catalog.append((fs_path, arc_path))
    else:
        pathbits = []
    if is_dir:
        files = []
        dirs = []
        for name in os.listdir(fs_path):
            p = os.path.join(fs_path, name)
            if os.path.isdir(p):
                dirs.append(name)
            else:
                files.append(name)
        for name in sorted(dirs):
            pathbits.append(name)
            enum_content(seed, catalog, pathbits)
            del pathbits[-1]
        for name in sorted(files):
            pathbits.append(name)
            enum_content(seed, catalog, pathbits)
            del pathbits[-1]


def build_stdlib():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pysrc-root', required=True)
    parser.add_argument('--output-zip', required=True)
    parser.add_argument('--py2', action='store_true')
    args = parser.parse_args()

    dirhere = os.path.normpath(os.path.abspath(os.path.dirname(__file__)))
    stdlib_srcdir = os.path.normpath(os.path.abspath(os.path.join(args.pysrc_root, 'Lib')))
    zipfilename = os.path.normpath(os.path.abspath(args.output_zip))
    display_zipname = os.path.basename(zipfilename)

    catalog = []
    enum_content(stdlib_srcdir, catalog)
    catalog += [
        (os.path.join(dirhere, 'site.py'), 'site.py'),
        (os.path.join(dirhere, 'sysconfig.py'), 'sysconfig.py'),
        (os.path.join(dirhere, '_sysconfigdata.py'), '_sysconfigdata.py'),
    ]
    if args.py2:
        catalog += [(os.path.join(dirhere, '_sitebuiltins.py'), '_sitebuiltins.py')]

    print("::: compiling python-stdlib zip package '{0}' ...".format(zipfilename))
    with zipfile.ZipFile(zipfilename, "w", zipfile.ZIP_DEFLATED) as fzip:
        for entry in catalog:
            fname, arcname = entry[0], entry[1]
            fzip.write(fname, arcname)
            print("::: {0} >>> {1}/{2}".format(fname, display_zipname, arcname))


if __name__ == '__main__':
    build_stdlib()
