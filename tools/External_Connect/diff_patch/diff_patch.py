# File: diff_patch.py
# Diff & Patch utilities for RogueCities Senior Coder Agent
#
# Provides:
# - Git diff generation (unified format)
# - Patch application with conflict detection
# - File snapshots for rollback
# - Change validation against Rogue Protocol

import os
import re
import json
import hashlib
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, Dict, Any, List, Tuple
from dataclasses import dataclass, field

SNAPSHOT_DIR = os.getenv("SNAPSHOT_DIR", "")
MAX_SNAPSHOTS = int(os.getenv("MAX_SNAPSHOTS", "50"))
MAX_DIFF_SIZE = int(os.getenv("MAX_DIFF_SIZE", "1048576"))

DENYLIST = [
    r"\.git[/\\]",
    r"\.env$",
    r"\.env\.",
    r"credentials",
    r"\.secret",
    r"\.key$",
    r"\.pem$",
    r"\.pfx$",
]


@dataclass
class DiffHunk:
    old_start: int
    old_count: int
    new_start: int
    new_count: int
    lines: List[str] = field(default_factory=list)


@dataclass
class DiffResult:
    path: str
    old_path: str
    new_path: str
    hunks: List[DiffHunk] = field(default_factory=list)
    binary: bool = False
    added: bool = False
    deleted: bool = False
    raw: str = ""


@dataclass
class PatchResult:
    success: bool
    path: str
    hunks_applied: int
    hunks_total: int
    conflicts: List[Dict[str, Any]] = field(default_factory=list)
    error: Optional[str] = None


@dataclass
class Snapshot:
    id: str
    timestamp: str
    files: Dict[str, str]
    description: str = ""


def _is_denied(path: str) -> bool:
    normalized = path.replace("\\", "/")
    for pattern in DENYLIST:
        if re.search(pattern, normalized, re.IGNORECASE):
            return True
    return False


def _run_git(args: List[str], cwd: str) -> Tuple[int, str, str]:
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=30,
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Git command timed out"
    except FileNotFoundError:
        return -1, "", "Git not found"
    except Exception as e:
        return -1, "", str(e)


def _file_hash(path: Path) -> str:
    try:
        with open(path, "rb") as f:
            return hashlib.sha256(f.read()).hexdigest()[:16]
    except Exception:
        return "error"


def _ensure_snapshot_dir(root: Path) -> Path:
    if SNAPSHOT_DIR:
        snap_dir = Path(SNAPSHOT_DIR)
    else:
        snap_dir = root / ".rogue-snapshots"

    snap_dir.mkdir(parents=True, exist_ok=True)
    return snap_dir


def parse_unified_diff(diff_text: str) -> List[DiffResult]:
    results = []
    current: Optional[DiffResult] = None
    current_hunk: Optional[DiffHunk] = None

    lines = diff_text.split("\n")
    i = 0

    while i < len(lines):
        line = lines[i]

        if line.startswith("diff --git "):
            if current:
                if current_hunk:
                    current.hunks.append(current_hunk)
                results.append(current)

            parts = line.split(" ")
            if len(parts) >= 4:
                old_path = parts[2].lstrip("a/")
                new_path = parts[3].lstrip("b/")
                current = DiffResult(
                    path=new_path,
                    old_path=old_path,
                    new_path=new_path,
                    raw=line + "\n",
                )
                current_hunk = None
            i += 1
            continue

        if current:
            current.raw += line + "\n"

            if line.startswith("new file mode"):
                current.added = True
            elif line.startswith("deleted file mode"):
                current.deleted = True
            elif line.startswith("Binary files"):
                current.binary = True

            if line.startswith("@@"):
                if current_hunk:
                    current.hunks.append(current_hunk)

                match = re.match(
                    r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@",
                    line,
                )
                if match:
                    current_hunk = DiffHunk(
                        old_start=int(match.group(1)),
                        old_count=int(match.group(2) or "1"),
                        new_start=int(match.group(3)),
                        new_count=int(match.group(4) or "1"),
                        lines=[line],
                    )
            elif current_hunk:
                current_hunk.lines.append(line)

        i += 1

    if current:
        if current_hunk:
            current.hunks.append(current_hunk)
        results.append(current)

    return results


def generate_diff(
    root: Path,
    path: str,
    base: str = "HEAD",
    staged: bool = False,
    context_lines: int = 3,
) -> Optional[DiffResult]:
    if _is_denied(path):
        return None

    file_path = root / path
    if not file_path.exists():
        return None

    args = ["diff"]

    if staged:
        args.append("--staged")

    args.extend(
        [
            f"-U{context_lines}",
            "--no-color",
            base,
            "--",
            path,
        ]
    )

    code, stdout, stderr = _run_git(args, str(root))

    if code != 0 or not stdout.strip():
        if not file_path.exists():
            return None

        code2, stdout2, stderr2 = _run_git(
            ["diff", "--no-index", "/dev/null", path],
            str(root),
        )
        if code2 == 0 or stdout2.strip():
            stdout = stdout2

    if not stdout.strip():
        return None

    diffs = parse_unified_diff(stdout)
    return diffs[0] if diffs else None


def generate_diff_uncommitted(
    root: Path,
    path: Optional[str] = None,
    include_staged: bool = True,
    context_lines: int = 3,
) -> List[DiffResult]:
    results = []

    args = ["diff"]
    if include_staged:
        args.append("HEAD")
    args.extend([f"-U{context_lines}", "--no-color"])

    if path:
        if _is_denied(path):
            return results
        args.extend(["--", path])

    code, stdout, stderr = _run_git(args, str(root))

    if stdout.strip():
        results.extend(parse_unified_diff(stdout))

    if include_staged and not path:
        code2, stdout2, _ = _run_git(
            ["diff", "--staged", f"-U{context_lines}", "--no-color"],
            str(root),
        )
        if stdout2.strip():
            for d in parse_unified_diff(stdout2):
                if not any(r.path == d.path for r in results):
                    results.append(d)

    return results


def apply_patch(
    root: Path,
    patch_text: str,
    path: Optional[str] = None,
    dry_run: bool = False,
) -> PatchResult:
    diffs = parse_unified_diff(patch_text)

    if not diffs:
        return PatchResult(
            success=False,
            path=path or "",
            hunks_applied=0,
            hunks_total=0,
            error="No valid diff found in patch",
        )

    target_diff = None
    if path:
        for d in diffs:
            if d.new_path == path or d.old_path == path:
                target_diff = d
                break
        if not target_diff:
            return PatchResult(
                success=False,
                path=path,
                hunks_applied=0,
                hunks_total=0,
                error=f"Patch does not contain changes for {path}",
            )
    else:
        target_diff = diffs[0]

    if _is_denied(target_diff.new_path):
        return PatchResult(
            success=False,
            path=target_diff.new_path,
            hunks_applied=0,
            hunks_total=len(target_diff.hunks),
            error="Path matches denylist",
        )

    file_path = root / target_diff.new_path

    if target_diff.binary:
        return PatchResult(
            success=False,
            path=target_diff.new_path,
            hunks_applied=0,
            hunks_total=0,
            error="Cannot apply binary patch",
        )

    if target_diff.added and not file_path.exists():
        file_path.parent.mkdir(parents=True, exist_ok=True)
        content = ""
        for hunk in target_diff.hunks:
            for line in hunk.lines[1:]:
                if line.startswith("+") and not line.startswith("+++"):
                    content += line[1:] + "\n"
        if not dry_run:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(content)
        return PatchResult(
            success=True,
            path=target_diff.new_path,
            hunks_applied=len(target_diff.hunks),
            hunks_total=len(target_diff.hunks),
        )

    if target_diff.deleted:
        if file_path.exists() and not dry_run:
            file_path.unlink()
        return PatchResult(
            success=True,
            path=target_diff.new_path,
            hunks_applied=1,
            hunks_total=1,
        )

    if not file_path.exists():
        return PatchResult(
            success=False,
            path=target_diff.new_path,
            hunks_applied=0,
            hunks_total=len(target_diff.hunks),
            error=f"File not found: {target_diff.new_path}",
        )

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            original_lines = f.read().split("\n")
    except Exception as e:
        return PatchResult(
            success=False,
            path=target_diff.new_path,
            hunks_applied=0,
            hunks_total=len(target_diff.hunks),
            error=f"Failed to read file: {e}",
        )

    result_lines = original_lines.copy()
    conflicts = []
    offset = 0
    applied = 0

    for hunk in target_diff.hunks:
        start_line = hunk.new_start - 1 + offset
        expected_old = []
        new_content = []

        for line in hunk.lines[1:]:
            if line.startswith("-"):
                expected_old.append(line[1:])
            elif line.startswith("+"):
                new_content.append(line[1:])
            elif line.startswith(" "):
                expected_old.append(line[1:])
                new_content.append(line[1:])

        actual_slice = result_lines[start_line : start_line + len(expected_old)]

        matches = True
        mismatch_pos = -1
        for i, (exp, act) in enumerate(zip(expected_old, actual_slice)):
            if exp != act:
                matches = False
                mismatch_pos = i
                break

        if not matches:
            fuzz_matched = False
            for fuzz in range(-5, 6):
                if fuzz == 0:
                    continue
                test_start = start_line + fuzz
                if test_start < 0 or test_start + len(expected_old) > len(result_lines):
                    continue
                actual_slice = result_lines[test_start : test_start + len(expected_old)]
                if actual_slice == expected_old:
                    start_line = test_start
                    fuzz_matched = True
                    break

            if not fuzz_matched:
                conflicts.append(
                    {
                        "hunk": applied + 1,
                        "line": start_line + 1,
                        "expected": expected_old[:5],
                        "actual": actual_slice[:5] if actual_slice else [],
                    }
                )
                continue

        if not dry_run:
            result_lines[start_line : start_line + len(expected_old)] = new_content
            offset += len(new_content) - len(expected_old)

        applied += 1

    if not dry_run and applied > 0:
        try:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write("\n".join(result_lines))
        except Exception as e:
            return PatchResult(
                success=False,
                path=target_diff.new_path,
                hunks_applied=applied,
                hunks_total=len(target_diff.hunks),
                conflicts=conflicts,
                error=f"Failed to write file: {e}",
            )

    return PatchResult(
        success=len(conflicts) == 0 and applied > 0,
        path=target_diff.new_path,
        hunks_applied=applied,
        hunks_total=len(target_diff.hunks),
        conflicts=conflicts,
    )


def create_snapshot(
    root: Path,
    paths: List[str],
    description: str = "",
) -> Snapshot:
    snap_dir = _ensure_snapshot_dir(root)

    files = {}
    for path in paths:
        if _is_denied(path):
            continue
        file_path = root / path
        if file_path.exists() and file_path.is_file():
            files[path] = _file_hash(file_path)

    snap_id = hashlib.sha256(
        (
            datetime.now(timezone.utc).isoformat() + json.dumps(files, sort_keys=True)
        ).encode()
    ).hexdigest()[:12]

    timestamp = datetime.now(timezone.utc).isoformat()

    for path, content_hash in files.items():
        file_path = root / path
        snap_path = snap_dir / snap_id / path
        snap_path.parent.mkdir(parents=True, exist_ok=True)
        try:
            shutil.copy2(file_path, snap_path)
        except Exception:
            pass

    meta = {
        "id": snap_id,
        "timestamp": timestamp,
        "description": description,
        "files": files,
    }

    meta_path = snap_dir / snap_id / "snapshot.json"
    with open(meta_path, "w", encoding="utf-8") as f:
        json.dump(meta, f, indent=2)

    _cleanup_old_snapshots(snap_dir)

    return Snapshot(
        id=snap_id,
        timestamp=timestamp,
        files=files,
        description=description,
    )


def restore_snapshot(root: Path, snapshot_id: str) -> Tuple[bool, str]:
    snap_dir = _ensure_snapshot_dir(root)
    snap_path = snap_dir / snapshot_id

    if not snap_path.exists():
        return False, f"Snapshot not found: {snapshot_id}"

    meta_path = snap_path / "snapshot.json"
    if not meta_path.exists():
        return False, f"Snapshot metadata missing: {snapshot_id}"

    try:
        with open(meta_path, "r", encoding="utf-8") as f:
            meta = json.load(f)
    except Exception as e:
        return False, f"Failed to read snapshot metadata: {e}"

    restored = []
    failed = []

    for path in meta.get("files", {}):
        if _is_denied(path):
            continue

        src = snap_path / path
        dst = root / path

        if src.exists():
            try:
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
                restored.append(path)
            except Exception as e:
                failed.append(f"{path}: {e}")

    if failed:
        return True, f"Restored {len(restored)} files, failed: {'; '.join(failed)}"
    return True, f"Restored {len(restored)} files from snapshot {snapshot_id}"


def list_snapshots(root: Path) -> List[Dict[str, Any]]:
    snap_dir = _ensure_snapshot_dir(root)

    snapshots = []
    for item in snap_dir.iterdir():
        if item.is_dir():
            meta_path = item / "snapshot.json"
            if meta_path.exists():
                try:
                    with open(meta_path, "r", encoding="utf-8") as f:
                        meta = json.load(f)
                    snapshots.append(meta)
                except Exception:
                    pass

    snapshots.sort(key=lambda x: x.get("timestamp", ""), reverse=True)
    return snapshots


def delete_snapshot(root: Path, snapshot_id: str) -> bool:
    snap_dir = _ensure_snapshot_dir(root)
    snap_path = snap_dir / snapshot_id

    if not snap_path.exists():
        return False

    try:
        shutil.rmtree(snap_path)
        return True
    except Exception:
        return False


def _cleanup_old_snapshots(snap_dir: Path):
    snapshots = []
    for item in snap_dir.iterdir():
        if item.is_dir():
            meta_path = item / "snapshot.json"
            if meta_path.exists():
                try:
                    with open(meta_path, "r", encoding="utf-8") as f:
                        meta = json.load(f)
                    snapshots.append((meta.get("timestamp", ""), item))
                except Exception:
                    pass

    snapshots.sort(key=lambda x: x[0], reverse=True)

    for _, snap_path in snapshots[MAX_SNAPSHOTS:]:
        try:
            shutil.rmtree(snap_path)
        except Exception:
            pass
