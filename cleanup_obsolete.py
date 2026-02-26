import os
files = [
    'visualizer/src/ui/tools/rc_tool_geometry_policy.cpp',
    'visualizer/src/ui/tools/rc_tool_geometry_policy.h'
]
for f in files:
    if os.path.exists(f):
        print(f"Removing {f}")
        os.remove(f)
    else:
        print(f"{f} not found")
