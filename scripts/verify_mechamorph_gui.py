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

bitmap_specs={
    "mech-machine":("machine",250,6,[("1.252",313),("1.5",375),("2",500)]),
    "mech-main":("main",170,96,[("1.2529411764705882",213),("1.5",255),("2",340)]),
    "mech-scale":("scale",220,72,[("1.25",275),("1.5",330),("2",440)]),
    "mech-utility":("utility",104,128,[("1.25",130),("1.5",156),("2",208)]),
}
bitmaps_node=root.find("bitmaps")
assert bitmaps_node is not None,"UIDESC bitmaps section missing"
bitmap_nodes={b.attrib["name"]:b for b in bitmaps_node.findall("bitmap")}
expected_names=set()
asset_root=ROOT/"resource/mechamorph-controls"

def png_size(path):
    data=path.read_bytes()[:24]
    assert data[:8]==b"\x89PNG\r\n\x1a\n",f"bad PNG signature: {path}"
    return struct.unpack(">II",data[16:24])

for base_name,(stem,size,frames,scaled) in bitmap_specs.items():
    expected_names.add(base_name)
    b=bitmap_nodes.get(base_name)
    assert b is not None,f"missing base bitmap {base_name}"
    assert b.attrib.get("path")==f"mechamorph-controls/{stem}.png"
    assert int(b.attrib.get("multiframe-num-frames","0"))==frames
    assert b.attrib.get("multiframe-size")==f"{size},{size}"
    assert b.attrib.get("mulitframe-frames-per-row")=="1"
    path=asset_root/f"{stem}.png"
    assert path.exists(),f"missing asset {path}"
    assert png_size(path)==(size,size*frames),(path,png_size(path),(size,size*frames))
    for factor,width in scaled:
        name=f"{base_name}#{factor}x"
        expected_names.add(name)
        sb=bitmap_nodes.get(name)
        assert sb is not None,f"missing scaled bitmap {name}"
        expected_path=f"mechamorph-controls/{stem}#{factor}x.png"
        assert sb.attrib.get("path")==expected_path,(name,sb.attrib.get("path"),expected_path)
        path=asset_root/f"{stem}#{factor}x.png"
        assert path.exists(),f"missing scaled asset {path}"
        assert png_size(path)==(width,width*frames),(path,png_size(path),(width,width*frames))
        sf=float(factor)
        assert width/sf==size,(name,"scaled bitmap logical width mismatch",width,sf,size)
        assert (width*frames)/sf==size*frames,(name,"scaled strip logical height mismatch")

assert set(bitmap_nodes)==expected_names,("unexpected bitmap nodes",set(bitmap_nodes)-expected_names)
assert len(list(asset_root.glob("*.png")))==16,"expected exactly 16 control PNGs"

tpl=root.find("template")
assert tpl is not None
assert tpl.attrib.get("size")=="1440,900"
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
    assert x>=0 and y>=0 and x+w<=1440 and y+h<=900,(v.attrib["custom-view-name"],x,y,w,h)

def box(n):
    v=next(v for v in views if v.attrib["custom-view-name"]==n)
    x,y=map(int,v.attrib["origin"].split(",")); w,h=map(int,v.attrib["size"].split(","))
    return x,y,w,h

# Permanent symmetry/alignment contracts.
assert box("BrandLogo")==tuple(layout["top"]["brandLogo"]),"BrandLogo geometry not sourced from layout"
assert box("BrandTitle")==tuple(layout["top"]["brandTitle"]),"BrandTitle geometry not sourced from layout"
assert box("BrandSubtitle")==tuple(layout["top"]["brandSubtitle"]),"BrandSubtitle geometry not sourced from layout"
for item in layout["top"]["machinePositions"]:
    assert box(item["name"])==tuple(item["rect"]),(item["name"],"machine position legend geometry mismatch")

for n,cx in [("Speed",143),("Load",385),("Action",627),("Wear",869),("Scale",1154),
             ("Body",1368),("Space",1368),("Output",1368),("Machine",720)]:
    x,y,w,h=box(n)
    assert x+w/2==cx,(n,"axis mismatch")

for knob,label in [("Speed","SpeedLabel"),("Load","LoadLabel"),("Action","ActionLabel"),
                   ("Wear","WearLabel"),("Scale","ScaleLabel"),("Body","BodyLabel"),
                   ("Space","SpaceLabel"),("Output","OutputLabel")]:
    kx,ky,kw,kh=box(knob); lx,ly,lw,lh=box(label)
    assert abs((kx+kw/2)-(lx+lw/2))<=1,(knob,label,"label not centered")

print("Mechamorph GUI geometry + multires asset contract PASS")
