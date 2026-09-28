#!/usr/bin/env python3
import argparse
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def rc(cx,cy,w,h):
    return [int(round(cx-w/2)),int(round(cy-h/2)),int(w),int(h)]

def view(name,r,mouse=True):
    x,y,w,h=r
    mouse_attr='' if mouse else ' mouse-enabled="false"'
    return f'    <view class="CView" custom-view-name="{name}" origin="{x},{y}" size="{w},{h}"{mouse_attr}/>'

def generate(L):
    W,H=L["canvas"]["w"],L["canvas"]["h"]
    sz=L["sizes"]; top=L["top"]; controls=L["controls"]; labels=L["labels"]
    out=[
      '<?xml version="1.0" encoding="UTF-8"?>',
      '<vstgui-ui-description version="1">',
      '  <colors><color name="Background" rgba="#090A0BFF"/></colors>',
      f'  <template name="view" class="CViewContainer" origin="0,0" size="{W},{H}" transparent="false" background-color="Background">',
      view("Faceplate",[0,0,W,H],False),
      '',
      view("BrandTitle",[48,64,430,62],False),
      view("BrandSubtitle",[48,132,430,28],False),
      view("UIScale",top["uiScale"]),
      '',
      view("MachineLabel",labels["machine"],False),
      view("Machine",rc(*top["machine"],sz["machine"],sz["machine"])),
      ''
    ]
    for key,name in [("pressure","Pressure"),("friction","Friction"),("stall","Stall")]:
        out.append(view(name+"Lamp",rc(*top["status"][key],sz["status"],sz["status"]),False))
    for key,name in [("pressure","Pressure"),("friction","Friction"),("stall","Stall")]:
        out.append(view(name+"Label",labels[key],False))
    out.append('')
    for key,name in [("speed","Speed"),("load","Load"),("action","Action"),("wear","Wear")]:
        out.append(view(name,rc(*controls[key],sz["main"],sz["main"])))
    out.append(view("Scale",rc(*controls["scale"],sz["scale"],sz["scale"])))
    out.append('')
    for key,name in [("body","Body"),("space","Space"),("output","Output")]:
        out.append(view(name,rc(*controls[key],sz["utility"],sz["utility"])))
    out.append('')
    for key,name in [("speed","Speed"),("load","Load"),("action","Action"),("wear","Wear"),("scale","Scale"),
                     ("body","Body"),("space","Space"),("output","Output")]:
        out.append(view(name+"Label",labels[key],False))
    out += ['  </template>','</vstgui-ui-description>']
    return "\n".join(out)+"\n"

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--layout",default=ROOT/"resource/mechamorph.layout.json")
    ap.add_argument("--output",default=ROOT/"resource/mechamorph.uidesc")
    a=ap.parse_args()
    L=json.loads(Path(a.layout).read_text(encoding="utf-8"))
    Path(a.output).write_text(generate(L),encoding="utf-8",newline="\n")
    print(f"Generated {a.output}")

if __name__=="__main__":
    main()
