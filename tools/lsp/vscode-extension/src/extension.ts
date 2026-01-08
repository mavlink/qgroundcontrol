import * as path from 'path';
import * as fs from 'fs';
import { workspace, ExtensionContext, commands, window } from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind,
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

export function activate(context: ExtensionContext) {
    const config = workspace.getConfiguration('qgc-lsp');

    if (!config.get<boolean>('enable', true)) {
        return;
    }

    // Find the LSP server
    const serverPath = findServerPath(config.get<string>('serverPath', ''));
    if (!serverPath) {
        window.showWarningMessage(
            'QGC LSP: Could not find language server. ' +
            'Make sure you have pygls installed (pip install pygls lsprotocol)'
        );
        return;
    }

    const pythonPath = config.get<string>('pythonPath', 'python3');

    // Server options - run Python server via STDIO
    const serverOptions: ServerOptions = {
        command: pythonPath,
        args: ['-m', 'tools.lsp'],
        options: {
            cwd: findWorkspaceRoot(),
        },
        transport: TransportKind.stdio,
    };

    // Client options
    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'cpp' },
            { scheme: 'file', language: 'c' },
        ],
        synchronize: {
            fileEvents: workspace.createFileSystemWatcher('**/*.{cpp,cc,cxx,h,hpp,hxx}'),
        },
        outputChannelName: 'QGC LSP',
    };

    // Create and start the client
    client = new LanguageClient(
        'qgc-lsp',
        'QGC Language Server',
        serverOptions,
        clientOptions
    );

    // Register restart command
    context.subscriptions.push(
        commands.registerCommand('qgc-lsp.restart', async () => {
            if (client) {
                await client.stop();
                await client.start();
                window.showInformationMessage('QGC LSP: Language server restarted');
            }
        })
    );

    // Start the client
    client.start().then(() => {
        console.log('QGC LSP client started');
    }).catch((error) => {
        console.error('Failed to start QGC LSP client:', error);
        window.showErrorMessage(`QGC LSP: Failed to start language server: ${error.message}`);
    });

    context.subscriptions.push(client);
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}

function findServerPath(configPath: string): string | undefined {
    // If explicit path is configured, use it
    if (configPath && fs.existsSync(configPath)) {
        return configPath;
    }

    // Look for server in workspace
    const workspaceRoot = findWorkspaceRoot();
    if (workspaceRoot) {
        const serverInWorkspace = path.join(workspaceRoot, 'tools', 'lsp', 'server.py');
        if (fs.existsSync(serverInWorkspace)) {
            return serverInWorkspace;
        }
    }

    // Fallback to module invocation (requires tools.lsp in Python path)
    return 'tools.lsp';
}

function findWorkspaceRoot(): string | undefined {
    const folders = workspace.workspaceFolders;
    if (!folders || folders.length === 0) {
        return undefined;
    }

    // Find the folder containing CMakeLists.txt (QGC root)
    for (const folder of folders) {
        const cmakePath = path.join(folder.uri.fsPath, 'CMakeLists.txt');
        if (fs.existsSync(cmakePath)) {
            return folder.uri.fsPath;
        }
    }

    return folders[0].uri.fsPath;
}
