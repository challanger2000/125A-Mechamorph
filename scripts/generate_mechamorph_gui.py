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
    sz=L["sizes"]; render=L["renderSizes"]; top=L["top"]; controls=L["controls"]
    out=[
      '<?xml version="1.0" encoding="UTF-8"?>',
      '<vstgui-ui-description version="1">',
      '  <colors><color name="Background" rgba="#090A0BFF"/></colors>',
      '  <bitmaps>',
      '    <bitmap name="mech-faceplate" path="mechamorph-faceplate-runtime.png"/>',
      '    <bitmap name="mech-danger-sign" path="mechamorph-controls/mechamorph-dangerous-sign-v2.png"/>',
      f'    <bitmap name="mech-knob-machine" path="mechamorph-controls/knob-machine-64.png" multiframe-num-frames="64" multiframe-size="{render["machine"]},{render["machine"]}" mulitframe-frames-per-row="1"/>',
      f'    <bitmap name="mech-knob-main" path="mechamorph-controls/knob-main-64.png" multiframe-num-frames="64" multiframe-size="{render["main"]},{render["main"]}" mulitframe-frames-per-row="1"/>',
      f'    <bitmap name="mech-knob-scale" path="mechamorph-controls/knob-scale-64.png" multiframe-num-frames="64" multiframe-size="{render["scale"]},{render["scale"]}" mulitframe-frames-per-row="1"/>',
      f'    <bitmap name="mech-knob-utility" path="mechamorph-controls/knob-utility-64.png" multiframe-num-frames="64" multiframe-size="{render["utility"]},{render["utility"]}" mulitframe-frames-per-row="1"/>',
      '  </bitmaps>',
      f'  <template name="view" class="CViewContainer" origin="0,0" size="{W},{H}" transparent="false" background-color="Background">',
      view("Faceplate",[0,0,W,H],False),
      '',
      view("DangerSign",L["decorations"]["dangerSign"],False),
      '',
      view("UIScale",top["uiScale"]),
      '',
      view("Machine",rc(*top["machine"],sz["machine"],sz["machine"]))
    ]
    for item in top["machinePositions"]:
        out.append(view(item["name"],item["rect"],False))
    out.append('')
    for key,name in [("pressure","Pressure"),("friction","Friction"),("stall","Stall")]:
        out.append(view(name+"Lamp",rc(*top["status"][key],sz["status"],sz["status"]),False))
    out.append('')
    for key,name in [("speed","Speed"),("load","Load"),("action","Action"),("wear","Wear")]:
        out.append(view(name,rc(*controls[key],sz["main"],sz["main"])))
    out.append(view("Scale",rc(*controls["scale"],sz["scale"],sz["scale"])))
    out.append('')
    for key,name in [("body","Body"),("space","Space"),("output","Output")]:
        out.append(view(name,rc(*controls[key],sz["utility"],sz["utility"])))
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
