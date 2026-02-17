const fs = require("fs");
const path = require("path");
const vscode = require("vscode");

let outputChannel;
let pendingTimer = null;

function severityToString(severity) {
  switch (severity) {
    case vscode.DiagnosticSeverity.Error:
      return "Error";
    case vscode.DiagnosticSeverity.Warning:
      return "Warning";
    case vscode.DiagnosticSeverity.Information:
      return "Information";
    case vscode.DiagnosticSeverity.Hint:
      return "Hint";
    default:
      return "Unknown";
  }
}

function diagnosticCodeToString(code) {
  if (typeof code === "string") {
    return code;
  }
  if (code && typeof code.value !== "undefined") {
    return String(code.value);
  }
  return "";
}

function toOneBasedPosition(position) {
  return {
    line: position.line + 1,
    character: position.character + 1
  };
}

function toJsonDiagnostic(diagnostic) {
  const relatedInformation = (diagnostic.relatedInformation || []).map((info) => ({
    message: info.message,
    filePath: info.location.uri.fsPath || info.location.uri.path,
    range: {
      start: toOneBasedPosition(info.location.range.start),
      end: toOneBasedPosition(info.location.range.end)
    }
  }));

  return {
    severity: severityToString(diagnostic.severity),
    message: diagnostic.message,
    source: diagnostic.source || "",
    code: diagnosticCodeToString(diagnostic.code),
    range: {
      start: toOneBasedPosition(diagnostic.range.start),
      end: toOneBasedPosition(diagnostic.range.end)
    },
    tags: diagnostic.tags || [],
    relatedInformation
  };
}

function getConfig() {
  const cfg = vscode.workspace.getConfiguration("problemsBridge");
  return {
    autoExport: cfg.get("autoExport", true),
    outputPath: cfg.get("outputPath", ".vscode/problems.export.json"),
    debounceMs: Math.max(0, cfg.get("debounceMs", 600)),
    showNotifications: cfg.get("showNotifications", false)
  };
}

function resolveOutputPath(config) {
  const folders = vscode.workspace.workspaceFolders || [];
  if (folders.length === 0) {
    return null;
  }
  if (path.isAbsolute(config.outputPath)) {
    return config.outputPath;
  }
  return path.join(folders[0].uri.fsPath, config.outputPath);
}

function collectDiagnosticsSnapshot(reason) {
  const folders = vscode.workspace.workspaceFolders || [];
  const diagnostics = vscode.languages.getDiagnostics();
  const files = [];
  let totalCount = 0;

  for (const [uri, fileDiagnostics] of diagnostics) {
    if (!fileDiagnostics || fileDiagnostics.length === 0) {
      continue;
    }

    const mapped = fileDiagnostics.map(toJsonDiagnostic);
    totalCount += mapped.length;
    files.push({
      uri: uri.toString(),
      filePath: uri.fsPath || uri.path,
      count: mapped.length,
      diagnostics: mapped
    });
  }

  files.sort((a, b) => a.filePath.localeCompare(b.filePath));

  return {
    generatedAt: new Date().toISOString(),
    reason,
    workspaceFolders: folders.map((f) => f.uri.fsPath),
    fileCount: files.length,
    totalCount,
    files
  };
}

async function exportDiagnostics(reason) {
  const config = getConfig();
  const outputPath = resolveOutputPath(config);
  if (!outputPath) {
    outputChannel.appendLine("[Problems Bridge] No workspace folder. Export skipped.");
    return;
  }

  const payload = collectDiagnosticsSnapshot(reason);
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, JSON.stringify(payload, null, 2), "utf8");

  outputChannel.appendLine(
    `[Problems Bridge] Exported ${payload.totalCount} diagnostics to ${outputPath}`
  );
  if (config.showNotifications) {
    vscode.window.setStatusBarMessage(
      `Problems Bridge: ${payload.totalCount} diagnostics exported`,
      3000
    );
  }
}

function scheduleExport(reason) {
  const config = getConfig();
  if (pendingTimer !== null) {
    clearTimeout(pendingTimer);
  }
  pendingTimer = setTimeout(() => {
    exportDiagnostics(reason).catch((error) => {
      outputChannel.appendLine(
        `[Problems Bridge] Export failed: ${error && error.message ? error.message : String(error)}`
      );
    });
  }, config.debounceMs);
}

function activate(context) {
  outputChannel = vscode.window.createOutputChannel("Problems Bridge");
  context.subscriptions.push(outputChannel);

  const exportCommand = vscode.commands.registerCommand(
    "problemsBridge.exportNow",
    async () => {
      await exportDiagnostics("manual-command");
    }
  );
  context.subscriptions.push(exportCommand);

  const diagnosticsListener = vscode.languages.onDidChangeDiagnostics(() => {
    if (getConfig().autoExport) {
      scheduleExport("diagnostics-changed");
    }
  });
  context.subscriptions.push(diagnosticsListener);

  const saveListener = vscode.workspace.onDidSaveTextDocument(() => {
    if (getConfig().autoExport) {
      scheduleExport("document-saved");
    }
  });
  context.subscriptions.push(saveListener);

  const settingsListener = vscode.workspace.onDidChangeConfiguration((event) => {
    if (event.affectsConfiguration("problemsBridge") && getConfig().autoExport) {
      scheduleExport("configuration-changed");
    }
  });
  context.subscriptions.push(settingsListener);

  if (getConfig().autoExport) {
    scheduleExport("startup");
  }
}

function deactivate() {
  if (pendingTimer !== null) {
    clearTimeout(pendingTimer);
    pendingTimer = null;
  }
}

module.exports = {
  activate,
  deactivate
};
