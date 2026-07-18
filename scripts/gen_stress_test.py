# Generates testdata/stress-test-10k.fcd: a 40x25 grid of cells, each with
# a mix of every primitive type (lines with FCJ arrows/dashes, rectangles,
# ellipses, polygons, complex curves, Beziers, texts, connections, PCB
# tracks and pads, across all 16 layers), plus three 800-vertex polygons
# and a 400-point curve - ~12k primitives for performance testing.
# Run from the repo root: python -X utf8 scripts/gen_stress_test.py
import math

lines = ["[FIDOCAD]"]
count = 0

COLS, ROWS, CELL = 40, 25, 120
X0, Y0 = 100, 100

for row in range(ROWS):
    for col in range(COLS):
        x = X0 + col * CELL
        y = Y0 + row * CELL
        layer = (row * COLS + col) % 16
        idx = row * COLS + col

        # two lines, one dashed with an end arrow
        lines.append(f"LI {x} {y} {x+50} {y+20} {layer}")
        count += 1
        lines.append(f"LI {x} {y+10} {x+50} {y+30} {layer}")
        lines.append(f"FCJ 2 0 3 1 {idx % 5} 0")
        count += 1
        # rectangle (alternating filled) and ellipse
        code = "RP" if idx % 2 else "RV"
        lines.append(f"{code} {x+55} {y} {x+85} {y+25} {layer}")
        count += 1
        code = "EP" if idx % 3 == 0 else "EV"
        lines.append(f"{code} {x+55} {y+30} {x+85} {y+55} {layer}")
        count += 1
        # small polygon (5 vertices)
        cx, cy = x + 25, y + 55
        pts = []
        for k in range(5):
            a = 2 * math.pi * k / 5 - math.pi / 2
            pts.append(f"{cx + round(15*math.cos(a))} {cy + round(15*math.sin(a))}")
        code = "PP" if idx % 4 == 0 else "PV"
        lines.append(f"{code} {' '.join(pts)} {layer}")
        count += 1
        # open complex curve (4 points)
        lines.append(f"CV 0 {x} {y+80} {x+20} {y+70} {x+40} {y+90} {x+60} {y+75} {layer}")
        count += 1
        # bezier
        lines.append(f"BE {x+60} {y+90} {x+70} {y+60} {x+85} {y+100} {x+95} {y+70} {layer}")
        count += 1
        # text
        lines.append(f"TY {x} {y+95} 4 3 0 0 {layer} * C{idx}")
        count += 1
        # two connection dots and a PCB track + pad
        lines.append(f"SA {x+90} {y+10} {layer}")
        lines.append(f"SA {x+90} {y+40} {layer}")
        count += 2
        lines.append(f"PL {x+90} {y+10} {x+90} {y+40} 3 {layer}")
        count += 1
        lines.append(f"PA {x+100} {y+25} 12 12 5 {idx % 3} {layer}")
        count += 1

# Three monster polygons (800 vertices each): sine-modulated rings
for m in range(3):
    cx, cy = 1200 + m * 1400, 3400
    pts = []
    for k in range(800):
        a = 2 * math.pi * k / 800
        r = 300 + 60 * math.sin(12 * a + m)
        pts.append(f"{round(cx + r*math.cos(a), 2)} {round(cy + r*math.sin(a), 2)}")
    lines.append(f"PV {' '.join(pts)} {m + 1}")
    count += 1

# One long open complex curve (400 points): a damped wave
pts = []
for k in range(400):
    t = k / 399
    pts.append(f"{round(100 + t*4700, 2)} {round(3900 + 150*math.sin(40*t)*math.exp(-2*t), 2)}")
lines.append(f"CV 0 {' '.join(pts)} 2")
count += 1

lines.append(f"TY 100 4100 8 6 0 1 0 * Stress test - {count} primitives")
count += 1

path = "testdata/stress-test-10k.fcd"
with open(path, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines) + "\n")
print(f"{path}: {count} primitives, {len(lines)} lines")
