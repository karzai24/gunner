"""Enable a generated local profile after closing Unreal; restore portable BP.

Standard Python, not an editor script. Verify every recovery source and keep a
backup before changing the local character or ignored Saved config. Refuse to
replace any authoring changed since the profile report was written.
"""
from pathlib import Path
import datetime
import configparser
import hashlib
import json
import shutil
import subprocess

P = Path(__file__).resolve().parent.parent

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def need(condition, message):
    if not condition:
        raise RuntimeError(message)

def main():
    report_file = P / 'Saved/local_movement_authoring_report.json'
    report = json.loads(report_file.read_text())
    need(report['status'] == 'local_profile_created_pending_enable_and_validation', 'Profile is already enabled or not ready')
    profile = report['settings']
    profile_file = P / 'Content' / (profile.removeprefix('/Game/') + '.uasset')
    need(sha(profile_file) == report['profile_package_sha256'], 'Generated profile was changed')
    character = P / report['character_package']
    need(sha(character) == report['character_sha256_before_enable'], 'Character changed after profile creation')
    initial = json.loads((P / 'Saved/installed_traversal_authoring_report.json').read_text())
    baseline = next(item for item in initial['backup_files'] if item['original'] == report['character_package'])
    baseline_file = Path(baseline['backup'])
    need(sha(baseline_file) == baseline['sha256'], 'Portable character recovery copy changed')
    git_blob = subprocess.check_output(['git', 'rev-parse', 'HEAD:' + report['character_package']], cwd=P, text=True).strip()
    backup_blob = subprocess.check_output(['git', 'hash-object', '--', str(baseline_file)], cwd=P, text=True).strip()
    need(git_blob == backup_blob, 'Recovery copy differs from current committed character; review before restoring')
    configs = []
    for platform in ('MacEditor', 'Mac'):
        file = P / 'Saved/Config' / platform / 'Game.ini'
        text = file.read_text() if file.exists() else ''
        if '[Gunner.LocalMotion]' in text:
            parsed = configparser.ConfigParser(strict=False, interpolation=None)
            parsed.read_string(text)
            need(dict(parsed.items('Gunner.LocalMotion')) == {'profile': profile},
                 'Existing different local motion config requires explicit review: ' + str(file))
        configs.append((file, text))
    backup = P / 'Saved/AuthoringDrafts' / ('enable-local-movement-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    backup.mkdir(parents=True, exist_ok=False)
    shutil.copy2(character, backup / character.name)
    for file, text in configs:
        target = backup / file.relative_to(P)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)
    shutil.copy2(baseline_file, character)
    need(sha(character) == baseline['sha256'], 'Portable character recovery failed')
    for file, text in configs:
        file.parent.mkdir(parents=True, exist_ok=True)
        if '[Gunner.LocalMotion]' not in text:
            file.write_text(text.rstrip() + '\n\n[Gunner.LocalMotion]\nProfile=' + profile + '\n')
    report['status'] = 'local_profile_enabled_pending_rendered_validation'
    report['enable_backup'] = str(backup)
    report['local_config_files'] = [str(file.relative_to(P)) for file, _ in configs]
    report['portable_character_sha256'] = sha(character)
    report_file.write_text(json.dumps(report, indent=2) + '\n')
    print('Local movement profile enabled; tracked character restored to the portable baseline.')

if __name__ == '__main__':
    main()
