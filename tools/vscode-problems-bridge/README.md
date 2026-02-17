# VSCode Problems Bridge

This local extension exports VS Code diagnostics (Problems panel) to JSON so terminal-based agents can read them.

## What it writes

- Default output file: `.vscode/problems.export.json`
- Includes file path, severity, message, source/code, and line/column ranges.

## Install (one-time)

Option A (quickest, from VS Code UI):

1. Open Command Palette.
2. Run `Developer: Install Extension from Location...`
3. Select: `tools/vscode-problems-bridge`
4. Reload VS Code when prompted.

Option B (CLI via VSIX):

```bash
cd tools/vscode-problems-bridge
npx @vscode/vsce package
code --install-extension vscode-problems-bridge-0.0.1.vsix
```

## Usage

- Auto-export is enabled by default.
- You can force an export with command:
  - `Problems Bridge: Export Diagnostics Now`

## Settings

- `problemsBridge.autoExport` (default: `true`)
- `problemsBridge.outputPath` (default: `.vscode/problems.export.json`)
- `problemsBridge.debounceMs` (default: `600`)
- `problemsBridge.showNotifications` (default: `false`)

