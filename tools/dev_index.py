import os
import json
import re
from pathlib import Path

def extract_symbols(content):
    symbols = []
    
    # Simple regex to catch top-level struct/class definitions
    class_pattern = re.compile(r'\b(?:class|struct)\s+([A-Z][a-zA-Z0-9_]+)')
    enum_pattern = re.compile(r'\benum\s+(?:class\s+)?([A-Z][a-zA-Z0-9_]+)')
    
    for match in class_pattern.finditer(content):
        symbols.append({"type": "class/struct", "name": match.group(1)})
    
    for match in enum_pattern.finditer(content):
        symbols.append({"type": "enum", "name": match.group(1)})
        
    return symbols

def main():
    print("[rc-index] Scanning codebase to generate symbolic map...")
    
    scan_dirs = ["core", "generators", "app", "visualizer"]
    index = {}

    for d in scan_dirs:
        path = Path(d)
        if not path.exists():
            continue
        for p in path.rglob('*'):
            if p.is_file() and p.suffix in ['.hpp', '.h']:
                try:
                    with open(p, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        
                    symbols = extract_symbols(content)
                    if symbols:
                        # Normalize path
                        rel_path = p.as_posix()
                        index[rel_path] = symbols
                except Exception as e:
                    print(f"Failed to scan {p}: {e}")

    out_file = Path("AI/docs/symbol_index.json")
    out_file.parent.mkdir(parents=True, exist_ok=True)
    
    with open(out_file, 'w', encoding='utf-8') as f:
        json.dump(index, f, indent=2)

    print(f"[rc-index] Successfully indexed {len(index)} headers into {out_file.as_posix()}")

if __name__ == "__main__":
    main()
