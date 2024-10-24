import subprocess
import sys


if __name__ == '__main__':
    subprocess.check_call(
        [
            sys.executable,
            '-m',
            'pip',
            'install', 
            '-r',
            'builder_requirements.txt'
        ]
    )
