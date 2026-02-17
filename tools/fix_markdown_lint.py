#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re
from typing import Iterable

HEADING_RE = re.compile(r"^(#{1,6})\s+(.*)$")
LIST_RE = re.compile(r"^(\s*)([-*+]\s+|\d+\.\s+).+")
FENCE_RE = re.compile(r"^\s*```")
TABLE_RULE_RE = re.compile(r"^\s*\|?\s*[:\-]+(?:\s*\|\s*[:\-]+)+\s*\|?\s*$")


def is_heading(line: str) -> bool:
    return bool(HEADING_RE.match(line))


def is_list_item(line: str) -> bool:
    return bool(LIST_RE.match(line))


def is_fence(line: str) -> bool:
    return bool(FENCE_RE.match(line))


def trim_heading_punctuation(line: str) -> str:
    m = HEADING_RE.match(line)
    if not m:
        return line
    hashes, text = m.groups()
    text = text.rstrip()
    if text.endswith(":"):
        text = text[:-1].rstrip()
    return f"{hashes} {text}"


def format_table_line(line: str) -> str:
    raw = line.strip()
    if "|" not in raw:
        return line
    if raw.startswith("|"):
        raw = raw[1:]
    if raw.endswith("|"):
        raw = raw[:-1]
    cells = [c.strip() for c in raw.split("|")]
    return "| " + " | ".join(cells) + " |"


def iter_markdown_files(paths: Iterable[pathlib.Path]) -> Iterable[pathlib.Path]:
    skip_dirs = {".git", "build", "build_vs", "3rdparty", "docs/generated"}
    for p in paths:
        if p.is_file() and p.suffix.lower() == ".md":
            yield p
            continue
        if not p.is_dir():
            continue
        for f in p.rglob("*.md"):
            rel = str(f).replace("\\", "/")
            if any(part in rel for part in skip_dirs):
                continue
            yield f


def normalize_file(path: pathlib.Path) -> bool:
    original = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    if not original:
        return False

    lines = [ln.rstrip() for ln in original]

    # First pass: heading punctuation + table formatting
    in_fence = False
    for i, line in enumerate(lines):
        if is_fence(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        if is_heading(line):
            lines[i] = trim_heading_punctuation(line)

    # Table block formatting outside fences
    i = 0
    in_fence = False
    while i < len(lines):
        if is_fence(lines[i]):
            in_fence = not in_fence
            i += 1
            continue
        if in_fence:
            i += 1
            continue
        if "|" in lines[i]:
            start = i
            while i < len(lines) and "|" in lines[i] and not is_fence(lines[i]):
                i += 1
            end = i
            if end - start >= 2:
                for j in range(start, end):
                    if lines[j].strip():
                        lines[j] = format_table_line(lines[j])
            continue
        i += 1

    # Second pass: blank lines around headings, fences, and list blocks
    out: list[str] = []
    in_fence = False

    def append_blank_if_needed() -> None:
        if out and out[-1] != "":
            out.append("")

    idx = 0
    while idx < len(lines):
        line = lines[idx]

        if is_fence(line):
            if not in_fence:
                append_blank_if_needed()
                out.append(line)
                in_fence = True
            else:
                out.append(line)
                in_fence = False
                out.append("")
            idx += 1
            continue

        if in_fence:
            out.append(line)
            idx += 1
            continue

        if is_heading(line):
            append_blank_if_needed()
            out.append(line)
            out.append("")
            idx += 1
            continue

        if is_list_item(line):
            append_blank_if_needed()
            while idx < len(lines) and is_list_item(lines[idx]):
                out.append(lines[idx])
                idx += 1
            if idx < len(lines) and lines[idx] != "":
                out.append("")
            continue

        out.append(line)
        idx += 1

    # compress multiple blanks
    final: list[str] = []
    for ln in out:
        if ln == "" and final and final[-1] == "":
            continue
        final.append(ln)

    if final and final[-1] != "":
        final.append("")

    new_text = "\n".join(final)
    old_text = "\n".join(original) + ("\n" if original else "")

    if new_text == old_text:
        return False

    path.write_text(new_text, encoding="utf-8", newline="\n")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Autofix common markdown contract/lint violations.")
    parser.add_argument("paths", nargs="*", default=["docs", "BUILD_INSTRUCTIONS.md"], help="Files/folders to process")
    args = parser.parse_args()

    targets = [pathlib.Path(p) for p in args.paths]
    changed = 0
    for md in iter_markdown_files(targets):
        try:
            if normalize_file(md):
                print(f"fixed: {md}")
                changed += 1
        except Exception as exc:
            print(f"error: {md}: {exc}")

    print(f"done. files changed: {changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
