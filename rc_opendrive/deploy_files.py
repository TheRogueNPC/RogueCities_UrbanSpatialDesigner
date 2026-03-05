import shutil
import os

def check_and_copy(src, dst):
    if not os.path.exists(src):
        print(f"Error: {src} does not exist.")
        return
    print(f"Copying from {src} to {dst}")
    shutil.copytree(src, dst, dirs_exist_ok=True)
    print("Done.")

check_and_copy('.Temp/RogueOpenDRIVE-main/src', 'rc_opendrive/src')
check_and_copy('.Temp/RogueOpenDRIVE-main/include', 'rc_opendrive/include/RogueOpenDRIVE')
