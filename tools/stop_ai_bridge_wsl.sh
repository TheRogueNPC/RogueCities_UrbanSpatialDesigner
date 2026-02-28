#!/usr/bin/env bash
set -euo pipefail

PORT="7077"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      PORT="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
RUN_DIR="$REPO_ROOT/tools/.run"
PID_FILE="$RUN_DIR/toolserver_wsl.pid"

stopped=0
if [[ -f "$PID_FILE" ]]; then
  PID="$(cat "$PID_FILE" 2>/dev/null || true)"
  if [[ -n "$PID" ]] && kill -0 "$PID" 2>/dev/null; then
    kill "$PID" 2>/dev/null || true
    for _ in $(seq 1 20); do
      if ! kill -0 "$PID" 2>/dev/null; then
        break
      fi
      sleep 0.1
    done
    if kill -0 "$PID" 2>/dev/null; then
      kill -9 "$PID" 2>/dev/null || true
    fi
    stopped=1
    echo "Stopped WSL bridge pid=$PID"
  fi
  rm -f "$PID_FILE"
fi

if command -v ss >/dev/null 2>&1; then
  PIDS="$(ss -ltnp 2>/dev/null | awk -v p=":$PORT" '$4 ~ p { print $NF }' | sed -n 's/.*pid=\([0-9]\+\).*/\1/p' | sort -u)"
  if [[ -n "$PIDS" ]]; then
    for p in $PIDS; do
      kill "$p" 2>/dev/null || true
      sleep 0.1
      if kill -0 "$p" 2>/dev/null; then
        kill -9 "$p" 2>/dev/null || true
      fi
      stopped=1
      echo "Stopped listener pid=$p on port $PORT"
    done
  fi
fi

if [[ $stopped -eq 0 ]]; then
  echo "No WSL bridge process found for port $PORT"
fi
