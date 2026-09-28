125A Mechamorph GUI Asset Drop

ZIEL-REPO:
challanger2000/125A-Mechamorph
Branch: v0.1.0

INSTALLATION:
Dieses ZIP DIREKT in das Root des Repositories entpacken.
Die Ordnerstruktur im ZIP entspricht bereits der Repo-Struktur.

ENTHALTEN:
resource/mechamorph-faceplate.png
    - Produktions-Faceplate, 1440 x 960
    - statisch, ohne Beschriftungen/Knöpfe

resource/mechamorph-controls/knob-static.png
    - hochauflösender statischer 2D-Top-View-Knopf
    - ohne eingebrannten Zeiger
    - transparenter Hintergrund

resource/mechamorph-controls/knob-static-250.png
resource/mechamorph-controls/knob-static-220.png
resource/mechamorph-controls/knob-static-170.png
resource/mechamorph-controls/knob-static-104.png
    - vorbereitete Größen für MACHINE / SCALE / MAIN / UTILITY

design/Mechamorph_Faceplate_REFERENCE.png
    - unveränderte hochauflösende Designreferenz

VSTGUI-PLAN:
- Faceplate als statischer Hintergrund.
- Knopf als statisches Bild.
- Amber/gelber Unterglow wird in VSTGUI gezeichnet.
- Beweglicher gelb/oranger Index wird separat in VSTGUI gezeichnet.
- MACHINE bekommt 6 feste Rastpositionen.
- Keine Filmstrips mehr nötig.

WICHTIG:
Noch NICHT die vorhandenen Parameter-IDs, State-Logik oder DSP ändern.
