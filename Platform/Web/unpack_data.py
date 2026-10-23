"""Unpack an Emscripten file-packager bundle (a web build's .data, listed in its .js) into a folder.

    python3 unpack_data.py SpaceCadetPinball.js SpaceCadetPinball.data game_resources

The GitHub Pages build uses it to take the game's files from the gh-pages branch (alula's web build).
"""
import json
import os
import re
import sys

js, data, out = sys.argv[1:4]
meta = json.loads(re.search(r'loadPackage\((\{"files":.*?\})\)', open(js, encoding="utf-8").read(), re.S).group(1))
blob = open(data, "rb").read()
os.makedirs(out, exist_ok=True)
count = 0
for f in meta["files"]:
    name = os.path.basename(f["filename"])
    if name.lower() == "readme.txt":
        continue
    with open(os.path.join(out, name), "wb") as fh:
        fh.write(blob[f["start"]:f["end"]])
    count += 1
print(count, "files into", out)
