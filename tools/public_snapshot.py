#!/usr/bin/env python3
"""Export an audited source-only snapshot without development Git history.

No remote mutation happens here. Map inputs, embedded output and map captures are
excluded until redistribution conditions are established. The output is a NEW,
empty directory, so this tool never deletes or rewrites an existing checkout.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT=Path(__file__).resolve().parents[1]
TOP=('AGENTS.md','README.md','README_KO.md','LICENSE','THIRD_PARTY_NOTICES.md','CMakeLists.txt','.gitattributes')
DOCS=('USER_GUIDE.md','DEVELOPMENT.md','ASSET_PROVENANCE.md','PUBLICATION_AUDIT.md',
      'ACCEPTANCE.md','HARDWARE_RETEST.md','MAPS_AUDIT.md','MEMORY.md','STORAGE.md',
      'POWER.md','STABILITY_KO.md','LAYOUT_AUDIT.md','ICON_AUDIT.md','ICON_AUDIT.json')
PUBLIC_IGNORE='''# Generated/local material is not licensed for bundled public redistribution.
.local/
build-cg/
build-host/
__pycache__/
*.pyc
.DS_Store
dist/
assets/maps/upstream/
assets/maps/statistics.json
src/maps/generated_maps.c
docs/captures/
docs/validation/
docs/SPECIFICATION_KO.txt
'''
BANNED_PREFIXES=('.git/','.local/','build-cg/','build-host/','dist/','assets/maps/upstream/',
                 'docs/captures/','docs/validation/')
BANNED_EXACT={'src/maps/generated_maps.c','assets/maps/statistics.json','docs/SPECIFICATION_KO.txt'}
PRIVATE_PATH=re.compile(rb'/(?:Users|home)/[A-Za-z0-9_.-]+/')
SECRET=re.compile(rb'gh[pousr]_[A-Za-z0-9_]{30,}|github_pat_[A-Za-z0-9_]{30,}|-----BEGIN (?:RSA |OPENSSH )?PRIVATE KEY-----')

def audit_files(root, paths):
    errors=[]
    for relative in paths:
        name=relative.as_posix();path=root/relative
        if name.startswith(BANNED_PREFIXES) or name in BANNED_EXACT or path.suffix.lower() in ('.pdf','.g3a','.exe','.o','.obj','.bin'):
            errors.append(f'Excluded publication path: {name}');continue
        if path.is_symlink() or not path.is_file():
            errors.append(f'Not a regular source file: {name}');continue
        data=path.read_bytes()
        if path.suffix.lower() not in ('.png',):
            if PRIVATE_PATH.search(data):errors.append(f'Private absolute path in {name}')
            if SECRET.search(data):errors.append(f'Credential/private key pattern in {name}')
    if errors:raise ValueError('\n'.join(errors))

def selected_files(root):
    files={Path(x) for x in TOP}
    for folder in ('include','src','tests','tools'):
        for p in (root/folder).rglob('*'):
            if p.is_file() and '__pycache__' not in p.parts and p.suffix not in ('.pyc',):
                rel=p.relative_to(root)
                if rel.as_posix() not in BANNED_EXACT:files.add(rel)
    files.update(Path('docs')/x for x in DOCS)
    for folder in ('docs/third_party','docs/public-captures','assets/font'):
        files.update(p.relative_to(root) for p in (root/folder).rglob('*') if p.is_file())
    files.update(p.relative_to(root) for p in (root/'assets').glob('icon-*.png'))
    files.add(Path('assets/maps/manifest.json'))
    return sorted(files)

def export(root,destination):
    if destination.exists():raise ValueError('Destination must not exist; choose a fresh candidate directory')
    paths=selected_files(root)
    audit_files(root,paths)
    destination.mkdir(parents=True)
    for relative in paths:
        target=destination/relative;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(root/relative,target)
    (destination/'.gitignore').write_text(PUBLIC_IGNORE)
    paths.append(Path('.gitignore'))
    audit_files(destination,paths)
    manifest={p.as_posix():hashlib.sha256((destination/p).read_bytes()).hexdigest() for p in sorted(paths)}
    print(f'Exported {len(paths)} public-safe source/notice/fixture files; no map pack or binary.')
    return manifest

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination',type=Path,nargs='?')
    parser.add_argument('--check-tracked',action='store_true',help='audit only files tracked in this snapshot Git repo')
    args=parser.parse_args()
    if args.check_tracked:
        import subprocess
        files=[Path(x.decode()) for x in subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).split(b'\0') if x]
        audit_files(ROOT,files);print(f'Public safety audit: {len(files)} tracked files passed.')
    elif args.destination:
        manifest=export(ROOT,args.destination.resolve())
        # Only an export inventory; Git records exact published source separately.
        print(hashlib.sha256(json.dumps(manifest,sort_keys=True).encode()).hexdigest())
    else:parser.error('provide a new destination or --check-tracked')
if __name__=='__main__':main()
