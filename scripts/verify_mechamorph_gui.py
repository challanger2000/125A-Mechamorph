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
    "mech-faceplate":"mechamorph-faceplate-runtime.png",
    "mech-knob-machine":"mechamorph-controls/knob-machine-64.png",
    "mech-knob-main":"mechamorph-controls/knob-main-64.png",
    "mech-knob-scale":"mechamorph-controls/knob-scale-64.png",
    "mech-knob-utility":"mechamorph-controls/knob-utility-64.png",
}
assert set(bitmap_nodes)==set(expected),("unexpected bitmap nodes",set(bitmap_nodes)^set(expected))
for name,path in expected.items():
    assert bitmap_nodes[name].attrib.get("path")==path,(name,bitmap_nodes[name].attrib.get("path"),path)

render=layout["renderSizes"]
film_specs={
    "mech-knob-machine":("64",f'{render["machine"]},{render["machine"]}'),
    "mech-knob-main":("64",f'{render["main"]},{render["main"]}'),
    "mech-knob-scale":("64",f'{render["scale"]},{render["scale"]}'),
    "mech-knob-utility":("64",f'{render["utility"]},{render["utility"]}'),
}
for film_name,(frames,frame_size) in film_specs.items():
    film=bitmap_nodes[film_name]
    assert film.attrib.get("multiframe-num-frames")==frames
    assert film.attrib.get("multiframe-size")==frame_size
    assert film.attrib.get("mulitframe-frames-per-row")=="1"

def png_size(path):
    data=path.read_bytes()[:24]
    assert data[:8]==b"\x89PNG\r\n\x1a\n",f"bad PNG signature: {path}"
    return struct.unpack(">II",data[16:24])

asset_sizes={
    ROOT/"resource/mechamorph-faceplate-runtime.png":(1440,960),
    ROOT/"resource/mechamorph-controls/mechamorph-knob-grip-64.png":(125,8000),
    ROOT/"resource/mechamorph-controls/knob-machine-64.png":(render["machine"],render["machine"]*64),
    ROOT/"resource/mechamorph-controls/knob-main-64.png":(render["main"],render["main"]*64),
    ROOT/"resource/mechamorph-controls/knob-scale-64.png":(render["scale"],render["scale"]*64),
    ROOT/"resource/mechamorph-controls/knob-utility-64.png":(render["utility"],render["utility"]*64),
}
for path,size in asset_sizes.items():
    assert path.exists(),f"missing asset {path}"
    assert png_size(path)==size,(path,png_size(path),size)

source_faceplate=ROOT/"resource/mechamorph-faceplate-v2.png"
assert source_faceplate.exists(),f"missing source faceplate {source_faceplate}"
fw,fh=png_size(source_faceplate)
assert fw*2==fh*3,("faceplate source must be 3:2",fw,fh)
assert fw>=1440 and fh>=960,("faceplate source too small",fw,fh)

assert abs(float(layout["knobFillRatio"])-0.94)<1e-9

tpl=root.find("template")
assert tpl is not None
assert tpl.attrib.get("size")==f"{W},{H}"
views=[v for v in tpl if v.attrib.get("custom-view-name")]
names=[v.attrib["custom-view-name"] for v in views]
required=[
 "Faceplate","UIScale","Machine","MachinePosRow1","MachinePosRow2",
 "PressureLamp","FrictionLamp","StallLamp",
 "Speed","Load","Action","Wear","Scale","Body","Space","Output"
]
missing=[n for n in required if n not in names]
assert not missing,missing
for forbidden in [
 "BrandLogo","BrandTitle","BrandSubtitle","MachineLabel",
 "PressureLabel","FrictionLabel","StallLabel",
 "SpeedLabel","LoadLabel","ActionLabel","WearLabel","ScaleLabel","BodyLabel","SpaceLabel","OutputLabel"
]:
    assert forbidden not in names,("baked faceplate text must not be redrawn",forbidden)
assert len(names)==len(set(names)),"duplicate custom view names"

for v in views:
    x,y=map(int,v.attrib["origin"].split(","))
    w,h=map(int,v.attrib["size"].split(","))
    assert x>=0 and y>=0 and x+w<=W and y+h<=H,(v.attrib["custom-view-name"],x,y,w,h)

def box(n):
    v=next(v for v in views if v.attrib["custom-view-name"]==n)
    x,y=map(int,v.attrib["origin"].split(",")); w,h=map(int,v.attrib["size"].split(","))
    return x,y,w,h

assert box("UIScale")==tuple(layout["top"]["uiScale"])
for item in layout["top"]["machinePositions"]:
    assert box(item["name"])==tuple(item["rect"])

axes={
 "Speed":layout["controls"]["speed"],
 "Load":layout["controls"]["load"],
 "Action":layout["controls"]["action"],
 "Wear":layout["controls"]["wear"],
 "Scale":layout["controls"]["scale"],
 "Body":layout["controls"]["body"],
 "Space":layout["controls"]["space"],
 "Output":layout["controls"]["output"],
 "Machine":layout["top"]["machine"],
}
for n,(cx,cy) in axes.items():
    x,y,w,h=box(n)
    assert x+w/2==cx,(n,"x axis mismatch",x,w,cx)
    assert y+h/2==cy,(n,"y axis mismatch",y,h,cy)

for key,name in [("pressure","PressureLamp"),("friction","FrictionLamp"),("stall","StallLamp")]:
    cx,cy=layout["top"]["status"][key]
    x,y,w,h=box(name)
    assert x+w/2==cx and y+h/2==cy,(name,"status axis mismatch")

print("Mechamorph baked-faceplate GUI geometry + unified control sizing PASS")
