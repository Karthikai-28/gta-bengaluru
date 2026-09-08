#!/usr/bin/env python3
"""Generate the annotated concept SVG from the same layout used by Unreal."""
from html import escape
from pathlib import Path
from sandbox_layout import geometry, load_layout


def main():
    data = load_layout()
    palette = {k: '#' + ''.join(f'{round(c * 255):02x}' for c in rgb) for k, rgb in data['palette'].items()}
    parts = ['''<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="960" viewBox="0 0 1280 960">
<defs><pattern id="grid" width="58" height="58" patternUnits="userSpaceOnUse"><path d="M 58 0 L 0 0 0 58" fill="none" stroke="#ffffff" stroke-opacity=".05"/></pattern></defs>
<rect width="1280" height="960" fill="#101c22"/>
<g font-family="DejaVu Sans, sans-serif">
<text x="48" y="46" fill="#edb74e" font-size="13" letter-spacing="3">NAMMA CITY / PHASE 1A</text>
<text x="48" y="97" fill="#f6efda" font-size="38" font-weight="bold">Your first delivery.</text>
<text x="48" y="129" fill="#aebeb8" font-size="16">120 × 120 m • Bengaluru-inspired street sandbox • fixed daylight</text>
<rect x="40" y="157" width="756" height="756" rx="12" fill="#25372f"/>
''']
    def xy(x, y):
        return 418 + x * 5.8, 535 - y * 5.8
    # Ground first, street surfaces second, buildings/props third.
    for shape, color, collision, center, size, pitch in geometry(data):
        if color == 'ground' or color == 'gold':
            continue
        x, y = xy(center[0], center[1]); w, h = size[0] * 5.8, size[1] * 5.8
        if center[2] > 2 and (color in ('cream', 'dark') or (color == 'teal' and size[2] < 0.5)):
            continue  # Roof plan omits facade details.
        if shape == 'Sphere' and color == 'leaf':
            parts.append(f'<circle cx="{x}" cy="{y}" r="{w/2}" fill="{palette[color]}" stroke="#679660" stroke-width="2"/>')
        else:
            parts.append(f'<rect x="{x-w/2}" y="{y-h/2}" width="{w}" height="{h}" fill="{palette[color]}"/>')
    parts.append('<rect x="70" y="187" width="696" height="696" fill="url(#grid)"/>')
    points = ' '.join(f'{xy(x,y)[0]},{xy(x,y)[1]}' for x,y in data['route'])
    parts.append(f'<polyline points="{points}" fill="none" stroke="#ffda69" stroke-width="4" stroke-dasharray="9 7" stroke-linejoin="round"/>')
    for building in data['buildings']:
        x, y = xy(*building['center'])
        # Small fixed labels fit the simple massing footprints.
        name = building['name'].split(' ')
        for i, word in enumerate(name):
            parts.append(f'<text x="{x}" y="{y-4+i*13}" text-anchor="middle" fill="#fff5df" font-size="10" font-weight="bold">{escape(word)}</text>')
    for n, loc in enumerate([data['spawn'][:2], data['pickup'][:2], [10, -10], data['delivery'][:2]], 1):
        x,y = xy(*loc)
        parts.append(f'<circle cx="{x}" cy="{y}" r="15" fill="#ffda69" stroke="#172229" stroke-width="3"/><text x="{x}" y="{y+5}" text-anchor="middle" fill="#172229" font-size="14" font-weight="bold">{n}</text>')
    sx, sy = xy(*data['spawn'][:2])
    parts.append(f'<path d="M {sx+18} {sy-16} L {sx+65} {sy-28} L {sx+65} {sy+28} L {sx+18} {sy+16}" fill="#ffda69" opacity=".17"/>')
    parts += ['''<text x="441" y="540" fill="#efead7" font-size="12" text-anchor="middle">MARKET COURT</text>
<text x="747" y="208" fill="#efead7" font-size="14">N ↑</text>
<path d="M 93 865 H 209 M 93 860 V 870 M 209 860 V 870" fill="none" stroke="#efead7" stroke-width="2"/>
<text x="151" y="851" text-anchor="middle" fill="#efead7" font-size="11">20 metres</text>
<text x="836" y="191" fill="#edb74e" font-size="12" letter-spacing="2">THE FIRST FIVE MINUTES</text>
''']
    steps = [
        ('01', 'Step outside', ['Start behind your character in', 'Safehouse Lane. Look toward the', 'tea stall; follow the gold trail.']),
        ('02', 'Collect the parcel', ['Walk or sprint to Namma Tea.', 'Look at the counter and press E.', 'The parcel appears on your back.']),
        ('03', 'Cross Market Court', ['Turn through the courtyard.', 'Try the ramp, jump, and explore', 'the shade of the street trees.']),
        ('04', 'Make your delivery', ['Reach Corner Stores and press E.', 'See completion, then press R', 'to reset and walk the route again.'])]
    for i, (num, title, lines) in enumerate(steps):
        y = 238 + i * 133
        parts.append(f'<text x="836" y="{y}" fill="#edb74e" font-size="16">{num}</text><text x="880" y="{y}" fill="#f6efda" font-size="20" font-weight="bold">{title}</text>')
        for j,line in enumerate(lines):
            parts.append(f'<text x="880" y="{y+28+j*22}" fill="#aebeb8" font-size="15">{line}</text>')
    parts += ['''<rect x="828" y="796" width="400" height="115" rx="10" fill="#203139"/>
<text x="850" y="825" fill="#f6efda" font-size="15" font-weight="bold">720p / 30 fps target</text>
<text x="850" y="851" fill="#aebeb8" font-size="14">Simple shapes. Shared materials. No traffic.</text>
<text x="850" y="879" fill="#edb74e" font-size="13">CONCEPT PLAN — NOT AN ENGINE CAPTURE</text>
<text x="48" y="942" fill="#8fa39b" font-size="12">Generated from Scripts/data/player_sandbox.json and sandbox_layout.py. GPU playability remains unverified.</text>
</g></svg>''']
    output = Path(__file__).resolve().parents[1] / 'docs/phase-1/starting-point.svg'
    output.write_text('\n'.join(parts))
    print(output)


if __name__ == '__main__':
    main()
