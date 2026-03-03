import os
import glob
import re

files = glob.glob('rc_opendrive/src/*.cpp') + glob.glob('rc_opendrive/include/RogueOpenDRIVE/*.h*')
pattern = re.compile(r'#include\s+"([A-Za-z0-9_-]+\.h(pp)?)"')

for f in files:
    with open(f, 'r') as file:
        content = file.read()
    
    new_content = pattern.sub(r'#include "RogueOpenDRIVE/\1"', content)
    
    with open(f, 'w') as file:
        file.write(new_content)

print(f"Updated includes in {len(files)} files.")
