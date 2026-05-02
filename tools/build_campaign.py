#!/usr/bin/env python3
# Resolve the user-supplied campaign tree to source-file stems and emit
# `assets/campaign.json`, the data the C++ World panel reads.
#
# Each leaf carries:
#   {"display": "01 - Icy Waters", "name": "icy waters", "source": "20level"}
# When a name can't be matched in the original Baba is You data, `source`
# stays empty and the entry will render as disabled in the tree.
#
# Usage:
#   C:/Python314/python.exe tools/build_campaign.py
#       [--root original-baba-is-you/Data/Worlds/baba] [-o assets/campaign.json]

import argparse
import json
import re
from pathlib import Path


# Source-of-truth tree: copied verbatim from the user's spec.
CAMPAIGN = {
    "main_world": [
        {"id": "00", "name": "Baba Is You"},
        {"id": "01", "name": "Where Do I Go?"},
        {"id": "02", "name": "Now What Is This?"},
        {"id": "03", "name": "Out Of Reach"},
        {"id": "04", "name": "Still Out Of Reach"},
        {"id": "05", "name": "Volcano"},
        {"id": "06", "name": "Off Limits"},
        {"id": "07", "name": "Grass Yard"},
        {"id": "08", "name": "Slideshow"},
        {"id": "09", "name": "Fragile Existence"},
        {"id": "10", "name": "Hostile Environment"},
        {"id": "Finale", "name": "A Way Out?"},
        {"id": "Secret", "name": "?"},
    ],
    "subworlds": [
        {"id": 1, "name": "The Lake",
         "levels": ["01 - Icy Waters", "02 - Turns", "03 - Affection",
                    "04 - Pillar Yard", "05 - Brick Wall", "06 - Lock",
                    "07 - Novice Locksmith", "08 - Locked In", "09 - Changeless",
                    "10 - Two Doors", "11 - Jelly Throne", "12 - Crab Storage",
                    "13 - Burglary"],
         "extra_levels": ["Extra 1 - Submerged Ruins",
                          "Extra 2 - Sunken Temple"]},
        {"id": 2, "name": "Solitary Island",
         "levels": ["00 - Poem", "01 - Float", "02 - Warm River",
                    "03 - Bridge Building", "04 - Bridge Building?",
                    "05 - Victory Spring", "06 - Assembly Team",
                    "07 - Catch The Thief!", "08 - Tiny Pond",
                    "09 - Research Facility", "10 - Wireless Connection",
                    "11 - Prison"],
         "extra_levels": ["Extra 1 - Boiling River", "Extra 2 - ...Bridges?",
                          "Extra 3 - Tiny Isle", "Extra 4 - Dim Signal",
                          "Extra 5 - Dungeon", "Extra 6 - Evaporating River"]},
        {"id": 3, "name": "Temple Ruins",
         "levels": ["01 - Fragility", "02 - Tunnel Vision",
                    "03 - A Present for You", "04 - Unreachable Shores",
                    "05 - But Where's the Key", "06 - Love Is Out There",
                    "07 - Perilous Gang", "08 - Double Moat",
                    "09 - Walls of Gold"],
         "extra_levels": ["Extra 1 - Further Fields"]},
        {"id": 4, "name": "Forest of Fall",
         "levels": ["01 - Hop", "02 - Grand Stream", "03 - Rocky Road",
                    "04 - Telephone", "05 - Haunt", "06 - Crate Square",
                    "07 - Ghost Friend", "08 - Ghost Guard", "09 - Leaf Chamber",
                    "10 - Not There", "11 - Catch", "12 - Dead End",
                    "A - Literacy", "B - Broken Playground", "C - Fetching",
                    "D - Scenic Pond", "E - Skeletal Door"],
         "extra_levels": ["Extra 1 - Jump", "Extra 2 - Even Less There",
                          "Extra 3 - Deep Pool"]},
        {"id": 5, "name": "Deep Forest",
         "levels": ["01 - Renovating", "02 - Toolshed", "03 - Keep Out!",
                    "04 - Baba Doesn't Respond", "05 - Patrol", "06 - Canyon",
                    "07 - Concrete Goals", "08 - Victory in the Open",
                    "09 - Moving Floor", "10 - Lovely House", "11 - Supermarket",
                    "12 - Lock the Door", "13 - Factory", "14 - Tiny Pasture",
                    "A - Nearly", "B - Not Quite", "C - Passing Through",
                    "D - Salvage", "E - Insulation"],
         "extra_levels": ["Extra 1 - Crumbling Floor", "Extra 2 - Skull House"]},
        {"id": 6, "name": "Rocket Trip",
         "levels": ["01 - Empty", "02 - Lonely Flag", "03 - Babas Are You",
                    "04 - Please Hold My Key", "05 - Horror Story",
                    "06 - Aiming High", "07 - Trio", "08 - Bottleneck",
                    "09 - Platformer", "10 - The Pit", "11 - Heavy Words",
                    "12 - Guardians", "13 - Sky Hold"],
         "extra_levels": ["Extra 1 - Existential Crisis",
                          "Extra 2 - Heavy Cloud"]},
        {"id": 7, "name": "Flower Garden",
         "levels": ["01 - Condition", "02 - Thicket", "03 - Sorting Facility",
                    "04 - Relaxing Spot", "05 - Maritime Adventures",
                    "06 - Ruined Orchard", "07 - Blockade",
                    "08 - Jaywalkers United", "09 - Overgrowth",
                    "10 - Adventurers"],
         "extra_levels": ["Extra 1 - Secret Garden", "Extra 2 - Out at Sea"]},
        {"id": 8, "name": "Chasm",
         "levels": ["A - Rocky Prison", "B - Siege", "C - Elusive Condition",
                    "D - Treasury", "E - Looking for a Heart", "F - Lava Flood",
                    "G - Entropy", "H - Floodgates", "I - Lonely Sight"],
         "extra_levels": ["Extra-1 - Metacognition", "Extra-2 - Multitool",
                          "Extra-3 - Broken", "Extra-4 - Alley",
                          "Extra-5 - Keke and the Star",
                          "Extra-6 - Visiting Baba",
                          "Extra-7 - Automated Doors"]},
        {"id": 9, "name": "Volcanic Cavern",
         "levels": ["01 - Tour", "02 - Peril at Every Turn", "03 - Pillarwork",
                    "04 - Mouse Hole", "05 - Torn Apart",
                    "06 - Vital Ingredients", "07 - Backstage", "08 - The Heist",
                    "09 - Join the Crew", "10 - Automaton", "11 - Trick Door",
                    "12 - Trapped", "13 - Tunnel", "14 - Broken Expectations"],
         "extra_levels": ["Extra 1 - Coronation"]},
        {"id": 10, "name": "Mountaintop",
         "levels": ["01 - Shuffle", "02 - Love at First Sight", "03 - Solitude",
                    "04 - What is Baba?", "05 - Connector",
                    "06 - Floaty Platforms", "07 - Seeking Acceptance",
                    "08 - Tectonic Movements"],
         "extra_levels": ["Extra 1 - The Floatiest Platforms"]},
    ],
}


def build_name_to_stem(root):
    """Scan every *.ld and map lowercase name -> stem.
    Some names occur multiple times in the catalogue; we keep the first match
    (sorted by stem) so the result is deterministic. Callers can override with
    `--alias` if they need a specific pick."""
    out = {}
    for ld in sorted(root.glob('*.ld')):
        text = ld.read_text(encoding='utf-8', errors='replace')
        m = re.search(r'^name=(.*)', text, re.M)
        if not m:
            continue
        name = m.group(1).strip().lower()
        out.setdefault(name, ld.stem)
    return out


def split_display(label):
    """'01 - Icy Waters' -> ('Icy Waters', 'icy waters')
       'Extra 2 - Sunken Temple' -> ('Sunken Temple', 'sunken temple')
       'A - Literacy' -> ('Literacy', 'literacy')"""
    if ' - ' in label:
        _, rest = label.split(' - ', 1)
        return rest, rest.lower()
    return label, label.lower()


def resolve_main(entry, name_map):
    """Match a main-world level by its `name` field (case-insensitive)."""
    needle = entry['name'].strip().lower()
    return name_map.get(needle, '')


def resolve_subworld_level(label, name_map):
    title, key = split_display(label)
    return name_map.get(key, '')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', default='original-baba-is-you/Data/Worlds/baba')
    ap.add_argument('-o', '--output', default='assets/campaign.json')
    args = ap.parse_args()

    root = Path(args.root)
    name_map = build_name_to_stem(root)
    print(f'indexed {len(name_map)} names from {root}')

    out = {'main_world': [], 'subworlds': []}
    misses = []

    for entry in CAMPAIGN['main_world']:
        source = resolve_main(entry, name_map)
        if not source:
            misses.append(f'main: {entry["id"]} - {entry["name"]}')
        out['main_world'].append({
            'id': entry['id'], 'name': entry['name'], 'source': source,
        })

    for sw in CAMPAIGN['subworlds']:
        sw_out = {'id': sw['id'], 'name': sw['name'],
                  'levels': [], 'extra_levels': []}
        for label in sw['levels']:
            source = resolve_subworld_level(label, name_map)
            title, _ = split_display(label)
            if not source:
                misses.append(f'  sw{sw["id"]}: {label}')
            sw_out['levels'].append({'display': label, 'name': title, 'source': source})
        for label in sw.get('extra_levels', []):
            source = resolve_subworld_level(label, name_map)
            title, _ = split_display(label)
            if not source:
                misses.append(f'  sw{sw["id"]} (extra): {label}')
            sw_out['extra_levels'].append({'display': label, 'name': title, 'source': source})
        out['subworlds'].append(sw_out)

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(out, indent=2, ensure_ascii=False), encoding='utf-8')
    print(f'wrote {out_path}')
    if misses:
        print(f'\nUnresolved ({len(misses)}):')
        for m in misses:
            print(' -', m)


if __name__ == '__main__':
    main()
