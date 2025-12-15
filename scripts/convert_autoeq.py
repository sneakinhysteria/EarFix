#!/usr/bin/env python3
"""
AutoEq to EarFix Converter

Converts AutoEq ParametricEQ.txt files to EarFix JSON format.
Downloads and processes the AutoEq database for use with EarFix headphone correction.

Usage:
    python convert_autoeq.py                    # Download and convert all
    python convert_autoeq.py --top 100          # Only top 100 popular headphones
    python convert_autoeq.py --update           # Update existing database
    python convert_autoeq.py --local /path      # Convert from local AutoEq clone

Output: ~/Library/Application Support/EarFix/headphones/
"""

import os
import sys
import json
import re
import argparse
import urllib.request
import zipfile
import tempfile
import shutil
from pathlib import Path
from datetime import datetime

# Popular headphones to prioritize (curated list)
POPULAR_HEADPHONES = [
    # Over-ear
    "Sennheiser HD 650", "Sennheiser HD 600", "Sennheiser HD 660S",
    "Sennheiser HD 800 S", "Sennheiser HD 800", "Sennheiser HD 560S",
    "Beyerdynamic DT 770 Pro", "Beyerdynamic DT 880", "Beyerdynamic DT 990 Pro",
    "Audio-Technica ATH-M50x", "Audio-Technica ATH-R70x",
    "AKG K702", "AKG K712 Pro", "AKG K371",
    "Sony WH-1000XM4", "Sony WH-1000XM5", "Sony WH-1000XM3",
    "Sony MDR-7506", "Sony MDR-V6",
    "Apple AirPods Max",
    "Focal Clear", "Focal Utopia", "Focal Elegia",
    "HIFIMAN Sundara", "HIFIMAN Ananda", "HIFIMAN Edition XS", "HIFIMAN HE400se",
    "Audeze LCD-2", "Audeze LCD-X",
    "Philips SHP9500", "Philips Fidelio X2HR",
    "Bose QuietComfort 45", "Bose 700",
    "Meze 99 Classics", "Meze Audio Liric",
    "Grado SR80e", "Grado SR325e",
    "Dan Clark Audio Aeon 2",

    # In-ear
    "Apple AirPods Pro", "Apple AirPods Pro 2",
    "Sony WF-1000XM4", "Sony WF-1000XM5", "Sony IER-M9",
    "Sennheiser IE 300", "Sennheiser IE 600", "Sennheiser Momentum True Wireless 3",
    "Samsung Galaxy Buds Pro", "Samsung Galaxy Buds2 Pro",
    "Moondrop Blessing 2", "Moondrop Starfield", "Moondrop Aria", "Moondrop Kato",
    "Shure SE215", "Shure SE535", "Shure SE846",
    "Etymotic ER2XR", "Etymotic ER4XR",
    "7Hz Timeless", "7Hz Salnotes Zero",
    "Truthear Zero", "Truthear Hexa",
    "KZ ZS10 Pro", "KZ ZSN Pro X",
    "Tin HiFi T2", "Tin HiFi T3 Plus",
    "FiiO FH3", "FiiO FD5",
    "Jabra Elite 85t", "Jabra Elite 7 Pro",
    "Google Pixel Buds Pro",
]

# Preferred measurement sources (in order of preference)
PREFERRED_SOURCES = [
    "oratory1990",
    "crinacle/GRAS 43AG-7",
    "crinacle/711 in-ear",
    "Rtings",
    "Innerfidelity",
]

def get_output_dir():
    """Get the EarFix headphones directory."""
    if sys.platform == "darwin":
        base = Path.home() / "Library" / "Application Support" / "EarFix"
    elif sys.platform == "win32":
        base = Path(os.environ.get("APPDATA", "")) / "EarFix"
    else:
        base = Path.home() / ".config" / "EarFix"

    output_dir = base / "headphones"
    output_dir.mkdir(parents=True, exist_ok=True)
    return output_dir


def parse_parametric_eq(filepath):
    """Parse AutoEq ParametricEQ.txt file into structured data."""
    filters = []
    preamp = 0.0

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"  Error reading {filepath}: {e}")
        return None

    # Extract preamp
    preamp_match = re.search(r'Preamp:\s*(-?\d+\.?\d*)\s*dB', content)
    if preamp_match:
        preamp = float(preamp_match.group(1))

    # Extract filters
    filter_pattern = r'Filter\s+\d+:\s+ON\s+(\w+)\s+Fc\s+(\d+)\s+Hz\s+Gain\s+(-?\d+\.?\d*)\s+dB\s+Q\s+(\d+\.?\d*)'
    for match in re.finditer(filter_pattern, content):
        filter_type = match.group(1)
        freq = int(match.group(2))
        gain = float(match.group(3))
        q = float(match.group(4))

        filters.append({
            "type": filter_type,
            "freq": freq,
            "gain": gain,
            "q": q
        })

    if not filters:
        return None

    return {
        "preamp": preamp,
        "filters": filters
    }


def find_headphone_files(autoeq_path):
    """Find all ParametricEQ.txt files in AutoEq directory."""
    headphones = []
    results_path = Path(autoeq_path) / "results"

    if not results_path.exists():
        print(f"Error: Results directory not found at {results_path}")
        return headphones

    for eq_file in results_path.rglob("*ParametricEQ.txt"):
        # Extract info from path
        rel_path = eq_file.relative_to(results_path)
        parts = list(rel_path.parts)

        if len(parts) < 3:
            continue

        source = parts[0]
        if len(parts) >= 4:
            source = f"{parts[0]}/{parts[1]}"
            hp_type = parts[2] if parts[2] in ["over-ear", "in-ear", "earbud"] else "unknown"
            name = eq_file.stem.replace(" ParametricEQ", "")
        else:
            hp_type = parts[1] if parts[1] in ["over-ear", "in-ear", "earbud"] else "unknown"
            name = eq_file.stem.replace(" ParametricEQ", "")

        headphones.append({
            "name": name,
            "source": source,
            "type": hp_type,
            "file": str(eq_file)
        })

    return headphones


def select_best_source(headphones_by_name):
    """Select the best measurement source for each headphone."""
    selected = {}

    for name, variants in headphones_by_name.items():
        best = None
        best_priority = len(PREFERRED_SOURCES) + 1

        for hp in variants:
            for i, pref_source in enumerate(PREFERRED_SOURCES):
                if pref_source in hp["source"]:
                    if i < best_priority:
                        best = hp
                        best_priority = i
                    break
            else:
                if best is None:
                    best = hp

        if best:
            selected[name] = best

    return selected


def convert_headphone(hp_info, output_dir):
    """Convert a single headphone EQ file to JSON."""
    eq_data = parse_parametric_eq(hp_info["file"])
    if not eq_data:
        return None

    # Create output JSON
    output = {
        "name": hp_info["name"],
        "source": hp_info["source"],
        "type": hp_info["type"],
        "preamp": eq_data["preamp"],
        "filters": eq_data["filters"]
    }

    # Sanitize filename
    safe_name = re.sub(r'[<>:"/\\|?*]', '_', hp_info["name"])
    output_file = output_dir / f"{safe_name}.json"

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(output, f, indent=2)

    return safe_name


def create_index(output_dir, converted_headphones):
    """Create index.json with list of all headphones."""
    index = {
        "version": datetime.now().strftime("%Y-%m-%d"),
        "source": "https://github.com/jaakkopasanen/AutoEq",
        "count": len(converted_headphones),
        "headphones": []
    }

    # Sort by type, then name
    sorted_hp = sorted(converted_headphones, key=lambda x: (x.get("type", ""), x["name"]))

    for hp in sorted_hp:
        safe_name = re.sub(r'[<>:"/\\|?*]', '_', hp["name"])
        index["headphones"].append({
            "name": hp["name"],
            "file": f"{safe_name}.json",
            "type": hp.get("type", "unknown"),
            "source": hp.get("source", "unknown")
        })

    index_file = output_dir / "index.json"
    with open(index_file, 'w', encoding='utf-8') as f:
        json.dump(index, f, indent=2)

    print(f"\nCreated index with {len(converted_headphones)} headphones")
    return index_file


def download_autoeq(temp_dir):
    """Download AutoEq repository as ZIP."""
    url = "https://github.com/jaakkopasanen/AutoEq/archive/refs/heads/master.zip"
    zip_path = temp_dir / "autoeq.zip"

    print(f"Downloading AutoEq database...")
    print(f"  URL: {url}")

    try:
        urllib.request.urlretrieve(url, zip_path)
    except Exception as e:
        print(f"Error downloading: {e}")
        return None

    print(f"  Extracting...")
    extract_dir = temp_dir / "autoeq"
    with zipfile.ZipFile(zip_path, 'r') as zf:
        zf.extractall(extract_dir)

    # Find the extracted directory (usually AutoEq-master)
    for item in extract_dir.iterdir():
        if item.is_dir() and "AutoEq" in item.name:
            return item

    return extract_dir


def main():
    parser = argparse.ArgumentParser(description="Convert AutoEq database to EarFix format")
    parser.add_argument("--local", type=str, help="Path to local AutoEq repository")
    parser.add_argument("--top", type=int, help="Only convert top N popular headphones")
    parser.add_argument("--popular-only", action="store_true", help="Only convert popular headphones")
    parser.add_argument("--output", type=str, help="Output directory (default: Application Support)")
    parser.add_argument("--list", action="store_true", help="List available headphones without converting")
    args = parser.parse_args()

    output_dir = Path(args.output) if args.output else get_output_dir()
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"EarFix AutoEq Converter")
    print(f"=======================")
    print(f"Output directory: {output_dir}")

    # Get AutoEq data
    temp_dir = None
    if args.local:
        autoeq_path = Path(args.local)
        if not autoeq_path.exists():
            print(f"Error: Local path not found: {autoeq_path}")
            return 1
    else:
        temp_dir = Path(tempfile.mkdtemp())
        autoeq_path = download_autoeq(temp_dir)
        if not autoeq_path:
            print("Failed to download AutoEq")
            return 1

    print(f"\nScanning for headphone measurements...")
    all_headphones = find_headphone_files(autoeq_path)
    print(f"  Found {len(all_headphones)} measurement files")

    # Group by name
    by_name = {}
    for hp in all_headphones:
        name = hp["name"]
        if name not in by_name:
            by_name[name] = []
        by_name[name].append(hp)

    print(f"  {len(by_name)} unique headphone models")

    # Select best source for each
    selected = select_best_source(by_name)

    # Filter to popular/top if requested
    if args.popular_only:
        selected = {k: v for k, v in selected.items() if k in POPULAR_HEADPHONES}
        print(f"  Filtered to {len(selected)} popular headphones")
    elif args.top:
        # Prioritize popular headphones, then alphabetical
        def sort_key(name):
            try:
                return (0, POPULAR_HEADPHONES.index(name))
            except ValueError:
                return (1, name)

        sorted_names = sorted(selected.keys(), key=sort_key)[:args.top]
        selected = {k: selected[k] for k in sorted_names}
        print(f"  Limited to top {len(selected)} headphones")

    if args.list:
        print(f"\nAvailable headphones:")
        for name in sorted(selected.keys()):
            hp = selected[name]
            print(f"  - {name} ({hp['type']}, {hp['source']})")
        return 0

    # Convert
    print(f"\nConverting {len(selected)} headphones...")
    converted = []
    for name, hp in selected.items():
        result = convert_headphone(hp, output_dir)
        if result:
            converted.append(hp)
            print(f"  + {name}")
        else:
            print(f"  - {name} (failed)")

    # Create index
    create_index(output_dir, converted)

    # Cleanup
    if temp_dir:
        shutil.rmtree(temp_dir, ignore_errors=True)

    print(f"\nDone! Converted {len(converted)} headphones to {output_dir}")
    print(f"EarFix will automatically detect these files on next load.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
