#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
layout_path=ROOT/"resource/mechamorph.layout.json"
ui_path=ROOT/"resource/mechamorph.uidesc"
layout=json.loads(layout_path.read_text(encoding="utf-8"))

with tempfile.TemporaryDirectory() as td:
    generated_path=Path(td)/"mechamorph.uidesc"
    subprocess.run([
        sys.executable,str(ROOT/"scripts/generate_mechamorph_gui.py"),
        "--layout",str(layout_path),"--output",str(generated_path)
    ],check=True)
    generated=generated_path.read_text(encoding="utf-8")
actual=ui_path.read_text(encoding="utf-8")
assert actual==generated,"mechamorph.uidesc is stale; regenerate from mechamorph.layout.json"

root=ET.fromstring(generated)
tpl=root.find("template")
assert tpl is not None
assert tpl.attrib.get("size")=="1440,900"
views=[v for v in tpl if v.attrib.get("custom-view-name")]
names=[v.attrib["custom-view-name"] for v in views]
required=[
 "Faceplate","BrandLogo","BrandTitle","BrandSubtitle","UIScale","MachineLabel","Machine",
 "PressureLamp","FrictionLamp","StallLamp","PressureLabel","FrictionLabel","StallLabel",
 "Speed","Load","Action","Wear","Scale","Body","Space","Output",
 "SpeedLabel","LoadLabel","ActionLabel","WearLabel","ScaleLabel","BodyLabel","SpaceLabel","OutputLabel"
]
missing=[n for n in required if n not in names]
assert not missing,missing
assert len(names)==len(set(names)),"duplicate custom view names"
for v in views:
    x,y=map(int,v.attrib["origin"].split(","))
    w,h=map(int,v.attrib["size"].split(","))
    assert x>=0 and y>=0 and x+w<=1440 and y+h<=900,(v.attrib["custom-view-name"],x,y,w,h)

def box(n):
    v=next(v for v in views if v.attrib["custom-view-name"]==n)
    x,y=map(int,v.attrib["origin"].split(",")); w,h=map(int,v.attrib["size"].split(","))
    return x,y,w,h

# Permanent symmetry/alignment contracts.
assert box("BrandLogo")==tuple(layout["top"]["brandLogo"]),"BrandLogo geometry not sourced from layout"
assert box("BrandTitle")==tuple(layout["top"]["brandTitle"]),"BrandTitle geometry not sourced from layout"
assert box("BrandSubtitle")==tuple(layout["top"]["brandSubtitle"]),"BrandSubtitle geometry not sourced from layout"

for n,cx in [("Speed",143),("Load",385),("Action",627),("Wear",869),("Scale",1154),
             ("Body",1368),("Space",1368),("Output",1368),("Machine",720)]:
    x,y,w,h=box(n)
    assert x+w/2==cx,(n,"axis mismatch")

for knob,label in [("Speed","SpeedLabel"),("Load","LoadLabel"),("Action","ActionLabel"),
                   ("Wear","WearLabel"),("Scale","ScaleLabel"),("Body","BodyLabel"),
                   ("Space","SpaceLabel"),("Output","OutputLabel")]:
    kx,ky,kw,kh=box(knob); lx,ly,lw,lh=box(label)
    assert abs((kx+kw/2)-(lx+lw/2))<=1,(knob,label,"label not centered")

print("Mechamorph GUI geometry contract PASS")
