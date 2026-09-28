#!/usr/bin/env python3
import json
import subprocess
import sys
import tempfile
import struct
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
layout_path=ROOT/"resource/mechamorph.layout.json"
ui_path=ROOT/"resource/mechamorph.uidesc"
layout=json.loads(layout_path.read_text(encoding="utf-8"))
W,H=layout["canvas"]["w"],layout["canvas"]["h"]

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
bitmaps_node=root.find("bitmaps")
assert bitmaps_node is not None,"UIDESC bitmaps section missing"
bitmap_nodes={b.attrib["name"]:b for b in bitmaps_node.findall("bitmap")}
expected={
    "mech-faceplate":"mechamorph-faceplate.png",
    "mech-static-machine":"mechamorph-controls/knob-static-250.png",
    "mech-static-main":"mechamorph-controls/knob-static-170.png",
    "mech-static-scale":"mechamorph-controls/knob-static-220.png",
    "mech-static-utility":"mechamorph-controls/knob-static-104.png",
}
assert set(bitmap_nodes)==set(expected),("unexpected bitmap nodes",set(bitmap_nodes)^set(expected))
for name,path in expected.items():
    assert bitmap_nodes[name].attrib.get("path")==path,(name,bitmap_nodes[name].attrib.get("path"),path)

def png_size(path):
    data=path.read_bytes()[:24]
    assert data[:8]==b"\x89PNG\r\n\x1a\n",f"bad PNG signature: {path}"
    return struct.unpack(">II",data[16:24])

asset_sizes={
    ROOT/"resource/mechamorph-faceplate.png":(1440,960),
    ROOT/"resource/mechamorph-controls/knob-static-250.png":(250,250),
    ROOT/"resource/mechamorph-controls/knob-static-220.png":(220,220),
    ROOT/"resource/mechamorph-controls/knob-static-170.png":(170,170),
    ROOT/"resource/mechamorph-controls/knob-static-104.png":(104,104),
}
for path,size in asset_sizes.items():
    assert path.exists(),f"missing asset {path}"
    assert png_size(path)==size,(path,png_size(path),size)

tpl=root.find("template")
assert tpl is not None
assert tpl.attrib.get("size")==f"{W},{H}"
views=[v for v in tpl if v.attrib.get("custom-view-name")]
names=[v.attrib["custom-view-name"] for v in views]
required=[
 "Faceplate","BrandLogo","BrandTitle","BrandSubtitle","UIScale","MachineLabel","Machine","MachinePosRow1","MachinePosRow2",
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
    assert x>=0 and y>=0 and x+w<=W and y+h<=H,(v.attrib["custom-view-name"],x,y,w,h)

def box(n):
    v=next(v for v in views if v.attrib["custom-view-name"]==n)
    x,y=map(int,v.attrib["origin"].split(",")); w,h=map(int,v.attrib["size"].split(","))
    return x,y,w,h

assert box("BrandLogo")==tuple(layout["top"]["brandLogo"])
assert box("BrandTitle")==tuple(layout["top"]["brandTitle"])
assert box("BrandSubtitle")==tuple(layout["top"]["brandSubtitle"])
for item in layout["top"]["machinePositions"]:
    assert box(item["name"])==tuple(item["rect"])

axes={
 "Speed":layout["controls"]["speed"][0],
 "Load":layout["controls"]["load"][0],
 "Action":layout["controls"]["action"][0],
 "Wear":layout["controls"]["wear"][0],
 "Scale":layout["controls"]["scale"][0],
 "Body":layout["controls"]["body"][0],
 "Space":layout["controls"]["space"][0],
 "Output":layout["controls"]["output"][0],
 "Machine":layout["top"]["machine"][0],
}
for n,cx in axes.items():
    x,y,w,h=box(n)
    assert x+w/2==cx,(n,"axis mismatch",x,w,cx)

for knob,label in [("Speed","SpeedLabel"),("Load","LoadLabel"),("Action","ActionLabel"),
                   ("Wear","WearLabel"),("Scale","ScaleLabel"),("Body","BodyLabel"),
                   ("Space","SpaceLabel"),("Output","OutputLabel")]:
    kx,ky,kw,kh=box(knob); lx,ly,lw,lh=box(label)
    assert abs((kx+kw/2)-(lx+lw/2))<=1,(knob,label,"label not centered")

print("Mechamorph static-faceplate GUI geometry + asset contract PASS")
