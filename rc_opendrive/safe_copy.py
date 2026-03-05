import shutil
import os

base = r"c:\Users\teamc\Documents\Rogue Cities\RogueCities_UrbanSpatialDesigner"
src = os.path.join(base, ".Temp", "RogueOpenDRIVE-main", "src")
inc = os.path.join(base, ".Temp", "RogueOpenDRIVE-main", "include")

dest_src = os.path.join(base, "rc_opendrive", "src")
dest_inc = os.path.join(base, "rc_opendrive", "include", "RogueOpenDRIVE")

print("Starting copy...")
try:
    if not os.path.exists(src): print("Warning: src not found at", src)
    if not os.path.exists(inc): print("Warning: inc not found at", inc)
    
    shutil.copytree(src, dest_src, dirs_exist_ok=True)
    print("Copied src")
    shutil.copytree(inc, dest_inc, dirs_exist_ok=True)
    print("Copied include")
except Exception as e:
    print("Error:", e)
