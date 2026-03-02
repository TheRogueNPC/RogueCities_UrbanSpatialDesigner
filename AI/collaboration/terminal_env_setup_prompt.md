# Terminal Environment Supercharge Prompt for Agents

**Context for Fellow Agents:**
If you are integrating new extensions or CLI tooling that require environment variables to hook properly into the IDE's VS Code Integrated Terminal, you must inject them into the `settings.json` file under `terminal.integrated.env.windows`. This allows the unified `rc-console` and Dev Shells to automatically inherit your proxy ports, tokens, and debug hooks seamlessly.

## Instructions
Please adapt the following prompt syntax to document your environment contributions and inject them into `C:\Users\teamc\AppData\Roaming\Antigravity\User\settings.json`:

---

**Example Configuration Prompt:**

```markdown
# Terminal Environment Changes

Please add the following environment variables to `terminal.integrated.env.windows` in my settings.json:

## Extension: vadimcn.vscode-lldb
No-config debugging
- `PATH=c:\Users\teamc\.antigravity\extensions\vadimcn.vscode-lldb-1.12.1\bin;${env:PATH}`
- `CODELLDB_LAUNCH_CONNECT_FILE=c:\Users\teamc\AppData\Roaming\Antigravity\User\workspaceStorage\0eeab2f8f9b1993d251d2f79a7a77f75\vadimcn.vscode-lldb\rpcaddress.txt`

## Extension: google.gemini-cli-vscode-ide-companion
- `GEMINI_CLI_IDE_SERVER_PORT=61181`
- `GEMINI_CLI_IDE_WORKSPACE_PATH=c:\Users\teamc\Documents\Rogue Cities\RogueCities_UrbanSpatialDesigner`
- `GEMINI_CLI_IDE_AUTH_TOKEN=d4ec633d-7bde-4e76-a237-9d2b4cce3087`

## Extension: vscode.git
Enables the following features: git auth provider
- `GIT_ASKPASS=c:\Users\teamc\AppData\Local\Programs\Antigravity\resources\app\extensions\git\dist\askpass.sh`
- `VSCODE_GIT_ASKPASS_NODE=C:\Users\teamc\AppData\Local\Programs\Antigravity\Antigravity.exe`
- `VSCODE_GIT_ASKPASS_EXTRA_ARGS=`
- `VSCODE_GIT_ASKPASS_MAIN=c:\Users\teamc\AppData\Local\Programs\Antigravity\resources\app\extensions\git\dist\askpass-main.js`
- `VSCODE_GIT_IPC_HANDLE=\\.\pipe\vscode-git-de8377d597-sock`

## Extension: eamodio.gitlens
Enables GK CLI integration
- `GK_GL_ADDR=http://127.0.0.1:61180`
- `GK_GL_PATH=C:\Users\teamc\AppData\Local\Temp\gitkraken\gitlens\gitlens-ipc-server-202872-61180.json`

## Extension: ms-python.python
Enables `python.terminal.shellIntegration.enabled` by modifying `PYTHONSTARTUP` and `PYTHON_BASIC_REPL`
- `PYTHONSTARTUP=c:\Users\teamc\AppData\Roaming\Antigravity\User\workspaceStorage\0eeab2f8f9b1993d251d2f79a7a77f75\ms-python.python\pythonrc.py`
- `PYTHON_BASIC_REPL=1`
```
