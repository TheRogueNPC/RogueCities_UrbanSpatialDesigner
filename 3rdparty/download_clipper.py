import os
import urllib.request
import zipfile
import shutil

url = 'https://github.com/AngusJohnson/Clipper2/archive/refs/heads/master.zip'
zip_path = 'clipper.zip'

print("Downloading Clipper2...")
urllib.request.urlretrieve(url, zip_path)

print("Extracting...")
with zipfile.ZipFile(zip_path, 'r') as zip_ref:
    zip_ref.extractall('extracted')

print("Moving to Clipper2 directory...")
shutil.move('extracted/Clipper2-master', 'Clipper2')

print("Cleaning up noise...")
shutil.rmtree('extracted')
os.remove(zip_path)

for noisy in ['.git', '.github', 'CPP/Tests']:
    path = os.path.join('Clipper2', noisy)
    if os.path.exists(path):
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)

print("Done.")
