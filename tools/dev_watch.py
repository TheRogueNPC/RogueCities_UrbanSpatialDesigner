import os
import sys
import time
import argparse
import subprocess
from pathlib import Path

def get_latest_mtime(watch_dirs):
    latest = 0
    for d in watch_dirs:
        path = Path(d)
        if not path.exists():
            continue
        for p in path.rglob('*'):
            if p.is_file() and p.suffix in ['.cpp', '.hpp', '.h', '.c']:
                st = p.stat()
                if st.st_mtime > latest:
                    latest = st.st_mtime
    return latest

def main():
    parser = argparse.ArgumentParser(description="Watch source files and rebuild target on change.")
    parser.add_argument("target", nargs='?', default="RogueCityVisualizerGui", help="CMake target to rebuild")
    args = parser.parse_args()

    watch_dirs = [
        "core/src", "core/include", 
        "generators/src", "generators/include", 
        "app/src", "app/include", 
        "visualizer/src"
    ]

    print(f"\n[rc-watch] Watching for C++ file changes to compile target: {args.target}")
    print("[rc-watch] Press Ctrl+C to stop.")

    last_mtime = get_latest_mtime(watch_dirs)
    
    try:
        while True:
            time.sleep(1.0)
            current = get_latest_mtime(watch_dirs)
            if current > last_mtime:
                print(f"\n[rc-watch] Change detected at timestamp {current}! Rebuilding {args.target}...")
                subprocess.run(["cmake", "--build", "--preset", "gui-release", "--target", args.target])
                # Reset mtime after build to avoid duplicate triggers if the build tool modifies paths
                last_mtime = get_latest_mtime(watch_dirs)
                print(f"\n[rc-watch] Resuming watch... (Press Ctrl+C to stop)")
    except KeyboardInterrupt:
        print("\n[rc-watch] Stopped watching.")

if __name__ == "__main__":
    main()
