#!/usr/bin/env bash
set -euo pipefail

PORT="7077"
HOST="127.0.0.1"
MODE="live"
OLLAMA_BASE_URL_ARG=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      PORT="$2"
      shift 2
      ;;
    --host)
      HOST="$2"
      shift 2
      ;;
    --mock)
      MODE="mock"
      shift
      ;;
    --live)
      MODE="live"
      shift
      ;;
    --ollama-base-url)
      OLLAMA_BASE_URL_ARG="$2"
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
LOG_FILE="$RUN_DIR/toolserver_wsl.log"

mkdir -p "$RUN_DIR"

if [[ -f "$PID_FILE" ]]; then
  EXISTING_PID="$(cat "$PID_FILE" 2>/dev/null || true)"
  if [[ -n "$EXISTING_PID" ]] && kill -0 "$EXISTING_PID" 2>/dev/null; then
    echo "WSL bridge already running (pid=$EXISTING_PID)" >&2
    exit 0
  fi
  rm -f "$PID_FILE"
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 not found in PATH" >&2
  exit 1
fi

python3 - <<'PY'
import importlib.util
mods = ["fastapi", "uvicorn", "httpx", "pydantic"]
missing = [m for m in mods if importlib.util.find_spec(m) is None]
if missing:
    raise SystemExit("Missing Python modules: " + ", ".join(missing))
print("python_modules_ok")
PY

export RC_AI_PIPELINE_V2="${RC_AI_PIPELINE_V2:-on}"
export RC_AI_AUDIT_STRICT="${RC_AI_AUDIT_STRICT:-off}"
export RC_AI_CONTROLLER_MODEL="${RC_AI_CONTROLLER_MODEL:-functiongemma}"
export RC_AI_TRIAGE_MODEL="${RC_AI_TRIAGE_MODEL:-codegemma:2b}"
export RC_AI_SYNTH_FAST_MODEL="${RC_AI_SYNTH_FAST_MODEL:-gemma3:4b}"
export RC_AI_SYNTH_ESCALATION_MODEL="${RC_AI_SYNTH_ESCALATION_MODEL:-gemma3:12b}"
export RC_AI_EMBEDDING_MODEL="${RC_AI_EMBEDDING_MODEL:-embeddinggemma}"
export RC_AI_VISION_MODEL="${RC_AI_VISION_MODEL:-granite3.2-vision}"
export RC_AI_OCR_MODEL="${RC_AI_OCR_MODEL:-glm-ocr}"
export RC_AI_BRIDGE_BASE_URL="http://127.0.0.1:${PORT}"

# Ollama runtime tuning defaults (can be overridden by caller).
export OLLAMA_FLASH_ATTENTION="${OLLAMA_FLASH_ATTENTION:-1}"
export OLLAMA_KV_CACHE_TYPE="${OLLAMA_KV_CACHE_TYPE:-f16}"

probe_ollama_base_url() {
  local url="$1"
  if [[ -z "$url" ]]; then
    return 1
  fi
  curl -fsS --max-time 2 "${url%/}/api/tags" >/dev/null 2>&1
}

choose_ollama_base_url() {
  local candidates=()
  candidates+=("http://127.0.0.1:11434")
  candidates+=("http://host.docker.internal:11434")
  if [[ -r /etc/resolv.conf ]]; then
    local ns
    ns="$(awk '/^nameserver[ \t]+/ {print $2; exit}' /etc/resolv.conf || true)"
    if [[ -n "$ns" ]]; then
      candidates+=("http://${ns}:11434")
    fi
  fi
  for c in "${candidates[@]}"; do
    if probe_ollama_base_url "$c"; then
      echo "$c"
      return 0
    fi
  done
  echo "http://127.0.0.1:11434"
  return 0
}

if [[ -n "$OLLAMA_BASE_URL_ARG" ]]; then
  export OLLAMA_BASE_URL="${OLLAMA_BASE_URL_ARG%/}"
elif [[ -z "${OLLAMA_BASE_URL:-}" ]]; then
  export OLLAMA_BASE_URL="$(choose_ollama_base_url)"
fi

if [[ "$MODE" == "mock" ]]; then
  export ROGUECITY_TOOLSERVER_MOCK="1"
else
  unset ROGUECITY_TOOLSERVER_MOCK || true
fi

cd "$REPO_ROOT"
if command -v setsid >/dev/null 2>&1; then
  nohup setsid python3 -m uvicorn tools.toolserver:app --host "$HOST" --port "$PORT" </dev/null >"$LOG_FILE" 2>&1 &
else
  nohup python3 -m uvicorn tools.toolserver:app --host "$HOST" --port "$PORT" </dev/null >"$LOG_FILE" 2>&1 &
fi
PID="$!"
echo "$PID" > "$PID_FILE"
disown "$PID" 2>/dev/null || true

HEALTH_URL="http://127.0.0.1:${PORT}/health"
for _ in $(seq 1 60); do
  if curl -fsS "$HEALTH_URL" >/dev/null 2>&1; then
    echo "WSL bridge started (pid=$PID mode=$MODE host=$HOST port=$PORT)"
    echo "Ollama base: ${OLLAMA_BASE_URL:-http://127.0.0.1:11434}"
    echo "Health: $HEALTH_URL"
    exit 0
  fi
  sleep 0.25
  if ! kill -0 "$PID" 2>/dev/null; then
    echo "WSL bridge process exited early. See: $LOG_FILE" >&2
    exit 1
  fi
done

echo "WSL bridge health check timed out. See: $LOG_FILE" >&2
exit 1
