# Claude Code: Agent Onboarding & Environment Brief

**To:** All active agents (Gemini, Codex, Copilot, local Gemma)
**From:** Claude Code
**Date:** 2026-03-01
**Topic:** Shell environment hardening, secret hygiene, and new Claude Code agent commands

---

## 1. What Changed This Session

### Secret hygiene — Gemini auth token
`GEMINI_CLI_IDE_AUTH_TOKEN` has been **removed from plain-text storage** in `settings.json` and `dev-shell.ps1`.

- It now lives in the Windows user environment (`HKCU\Environment`) — set once, inherited by every terminal automatically.
- `settings.json` references it as `${env:GEMINI_CLI_IDE_AUTH_TOKEN}`.
- `dev-shell.ps1 $ideEnvMap` reads it as `$env:GEMINI_CLI_IDE_AUTH_TOKEN`.

**If you are an agent that needs to reference this token:** read it from the environment. Do not hardcode it. Do not write it back into any file.

### Active agent marker
A new environment variable is now set in every terminal and dev-shell session:

```
RC_ACTIVE_AGENT = claude-code
```

This is registered in both `settings.json → terminal.integrated.env.windows` and `dev-shell.ps1 $ideEnvMap`. Scripts and pipelines can branch on this to know which agent is driving.

### AI bridge URL standardized
`RC_AI_BRIDGE_BASE_URL = http://127.0.0.1:7077` is now injected at the VS Code terminal level via `settings.json`, so bare terminals (cmd, Git Bash, VS Dev Prompt) that skip `dev-shell.ps1` still inherit it. The dev-shell conditional (`if (-not $env:RC_AI_BRIDGE_BASE_URL)`) respects whatever is already set, so this is safe and non-breaking.

---

## 2. New Dev-Shell Commands

Three new `rc-claude*` commands are registered at the bottom of `tools/dev-shell.ps1`, in the **Claude Code Agent** section.

| Command | Description |
|---|---|
| `rc-claude [-Prompt '...']` | Opens Claude Code CLI in `RC_ROOT`. Pass `-Prompt` for one-shot non-interactive use. |
| `rc-claude-status` | Prints `RC_ACTIVE_AGENT`, `RC_AI_BRIDGE_BASE_URL`, `RC_AI_PIPELINE_V2`, and the first 40 lines of Claude's persistent memory file. |
| `rc-claude-handoff [-Summary '...'] [-NextAgent gemini]` | Writes a dated `claude_YYYY-MM-DD_handoff.md` to `AI/collaboration/` capturing env snapshot, all available `rc-*` commands, and next-agent guidance. |

These appear in `rc-help` under the **Claude Code Agent** section at the bottom.

---

## 3. New rc-console TUI Commands

Three commands are now available inside the headless interactive console (`rc-console` / `--interactive`):

| Command | Behaviour |
|---|---|
| `agent` | Inline print of `RC_ACTIVE_AGENT` and `RC_AI_BRIDGE_BASE_URL` — no subprocess, instant. |
| `claude_status` | Shells to `rc-claude-status`. Shows memory + agent config. |
| `claude_handoff` | Shells to `rc-claude-handoff`. Drops a brief in `AI/collaboration/`. |

These appear in the `help` / `?` output under the **DEV SHELL SUBPROCESS** section.

---

## 4. How to Add Your Own Agent Commands

Follow the pattern established in `AI/collaboration/terminal_env_setup_prompt.md`:

1. **settings.json** — add your IDE companion vars to `terminal.integrated.env.windows`. Reference secrets via `${env:YOUR_SECRET}`, never hardcode.
2. **dev-shell.ps1 `$ideEnvMap`** — add your vars so dev-shell terminals also inherit them. Use `$env:YOUR_SECRET` for anything sensitive.
3. **dev-shell functions** — add an `rc-<agent>*` function block with `.SYNOPSIS`, and register it in `rc-help`.
4. **main.cpp interactive handler** — add `else if (line == "your_command")` in the stdin reader, and add a line to the `help` output.

All four points together = full "agent at home" coverage across VS Code terminals, dev-shell, and the TUI console.

---

## 5. Validation

Changes are in:
- `C:\Users\teamc\AppData\Roaming\Antigravity\User\settings.json` — `terminal.integrated.env.windows`
- `tools/dev-shell.ps1` — `$ideEnvMap`, `rc-claude*` functions, `rc-help`
- `visualizer/src/main.cpp` — `agent`, `claude_status`, `claude_handoff` commands + help text

No generator, core, or app layer files were touched. Layer boundary: **meta/tooling only**.

`CHANGELOG updated: no` — these are dev-environment-only changes (no compiled output changed; main.cpp change takes effect after next `rc-bld-vis`).

---

## 6. Open Items for Next Agent

- `main.cpp` TUI changes require a `rc-bld-vis` or `rc-bld-headless` to take effect.
- `RC_ACTIVE_AGENT` is currently hardcoded to `claude-code`. If multi-agent handoffs need to flip this dynamically, consider a small helper that sets it at session start.
- The Gemini token in `dev-shell.ps1` will resolve to empty string if the Windows user env var was not yet propagated to the current shell. A full terminal restart is needed after the first `SetEnvironmentVariable` call.
