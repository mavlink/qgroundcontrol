-- QGC Tools - Qt Creator Lua Extension
-- Integrates QGC development tools with the IDE

local a = require("action")
local process = require("process")
local gui = require("gui")
local utils = require("utils")
local fetch = require("fetch")

-- Configuration
local Config = {
    -- Path to QGC tools directory (relative to project root)
    toolsPath = "tools",
    -- Python executable
    python = "python3",
}

-- Helper: Get project root directory
local function getProjectRoot()
    local projectDir = a.currentProjectDirectory()
    if projectDir and projectDir ~= "" then
        return projectDir
    end
    -- Fallback to current file's directory
    local doc = a.currentDocument()
    if doc then
        local path = doc:filePath()
        -- Walk up to find .git or CMakeLists.txt
        local dir = path:match("(.*/)")
        while dir and dir ~= "/" do
            if utils.fileExists(dir .. ".git") or utils.fileExists(dir .. "CMakeLists.txt") then
                return dir
            end
            dir = dir:match("(.*/)[^/]+/")
        end
    end
    return nil
end

-- Helper: Run a tool and return output
local function runTool(toolPath, args, callback)
    local projectRoot = getProjectRoot()
    if not projectRoot then
        gui.showError("QGC Tools", "Could not determine project root directory")
        return
    end

    local fullToolPath = projectRoot .. "/" .. Config.toolsPath .. "/" .. toolPath
    local cmd = { Config.python, fullToolPath }
    for _, arg in ipairs(args) do
        table.insert(cmd, arg)
    end

    local proc = process.Process()
    proc:setWorkingDirectory(projectRoot)
    proc:setCommand(cmd)

    local stdout = ""
    local stderr = ""

    proc:onReadyReadStandardOutput(function()
        stdout = stdout .. proc:readAllStandardOutput()
    end)

    proc:onReadyReadStandardError(function()
        stderr = stderr .. proc:readAllStandardError()
    end)

    proc:onFinished(function(exitCode, exitStatus)
        if callback then
            callback(exitCode, stdout, stderr)
        end
    end)

    proc:start()
end

-- ============================================================================
-- QGC Locator - Search for Facts, FactGroups, MAVLink
-- ============================================================================

local function showLocatorDialog(searchType)
    local titles = {
        fact = "Search Facts",
        factgroup = "Search FactGroups",
        mavlink = "Search MAVLink Usage",
    }

    local input = gui.inputDialog(titles[searchType] or "QGC Search", "Enter search query:")
    if not input or input == "" then
        return
    end

    runTool("locators/qgc_locator.py", { searchType, input, "--json" },
        function(exitCode, stdout, stderr)
            if exitCode ~= 0 then
                gui.showError("QGC Locator", "Search failed:\n" .. stderr)
                return
            end

            -- Parse JSON results
            local ok, results = pcall(function()
                return fetch.json(stdout)
            end)

            if not ok or not results then
                gui.showError("QGC Locator", "Failed to parse results")
                return
            end

            if #results == 0 then
                gui.showInfo("QGC Locator", "No results found for: " .. input)
                return
            end

            -- Build display items
            local items = {}
            for _, r in ipairs(results) do
                local display = string.format("%s  (%s:%d)", r.name, r.file, r.line)
                table.insert(items, {
                    text = display,
                    file = r.file,
                    line = r.line,
                })
            end

            -- Show results in a list dialog
            local selected = gui.selectFromList("QGC Locator - " .. #results .. " results", items,
                function(item) return item.text end)

            if selected then
                -- Open the selected file at line
                local projectRoot = getProjectRoot()
                local fullPath = projectRoot .. "/" .. selected.file
                a.openDocument(fullPath)
                a.gotoLine(selected.line)
            end
        end)
end

-- ============================================================================
-- Vehicle Null-Check Analyzer
-- ============================================================================

local function runAnalyzerOnCurrentFile()
    local doc = a.currentDocument()
    if not doc then
        gui.showError("QGC Analyzer", "No file open")
        return
    end

    local filePath = doc:filePath()
    if not filePath:match("%.[ch]pp?$") and not filePath:match("%.cc?$") then
        gui.showInfo("QGC Analyzer", "Not a C++ file")
        return
    end

    runTool("analyzers/vehicle_null_check.py", { filePath, "--json" },
        function(exitCode, stdout, stderr)
            local ok, violations = pcall(function()
                return fetch.json(stdout)
            end)

            if not ok then
                violations = {}
            end

            if #violations == 0 then
                gui.showInfo("QGC Analyzer", "No null-safety issues found!")
                return
            end

            -- Build display items
            local items = {}
            for _, v in ipairs(violations) do
                local display = string.format("Line %d: %s\n  → %s",
                    v.line, v.code:gsub("^%s+", ""), v.suggestion)
                table.insert(items, {
                    text = display,
                    line = v.line,
                    column = v.column,
                })
            end

            local selected = gui.selectFromList(
                "Null-Safety Issues - " .. #violations .. " found", items,
                function(item) return item.text end)

            if selected then
                a.gotoLine(selected.line, selected.column)
            end
        end)
end

local function runAnalyzerOnProject()
    local projectRoot = getProjectRoot()
    if not projectRoot then
        gui.showError("QGC Analyzer", "Could not determine project root")
        return
    end

    gui.showInfo("QGC Analyzer", "Running analyzer on src/ directory...\nThis may take a moment.")

    runTool("analyzers/vehicle_null_check.py", { "src/", "--json" },
        function(exitCode, stdout, stderr)
            local ok, violations = pcall(function()
                return fetch.json(stdout)
            end)

            if not ok then
                violations = {}
            end

            if #violations == 0 then
                gui.showInfo("QGC Analyzer", "No null-safety issues found in project!")
                return
            end

            -- Group by file
            local byFile = {}
            for _, v in ipairs(violations) do
                local file = v.file
                if not byFile[file] then
                    byFile[file] = {}
                end
                table.insert(byFile[file], v)
            end

            -- Build display items
            local items = {}
            for file, fileViolations in pairs(byFile) do
                for _, v in ipairs(fileViolations) do
                    local display = string.format("%s:%d - %s",
                        file:match("[^/]+$"), v.line, v.code:gsub("^%s+", ""))
                    table.insert(items, {
                        text = display,
                        file = file,
                        line = v.line,
                        column = v.column,
                    })
                end
            end

            local selected = gui.selectFromList(
                "Project Null-Safety Issues - " .. #violations .. " found", items,
                function(item) return item.text end)

            if selected then
                local fullPath = projectRoot .. "/" .. selected.file
                a.openDocument(fullPath)
                a.gotoLine(selected.line, selected.column)
            end
        end)
end

-- ============================================================================
-- FactGroup Generator
-- ============================================================================

local function runGeneratorWizard()
    -- Step 1: Get FactGroup name
    local name = gui.inputDialog("FactGroup Generator", "Enter FactGroup domain name (e.g., 'Wind', 'Battery'):")
    if not name or name == "" then
        return
    end

    -- Step 2: Get facts definition
    local factsHelp = [[Enter facts as: name:type:units (comma-separated)
Example: speed:double:m/s,altitude:double:m,count:uint32]]
    local facts = gui.inputDialog("FactGroup Generator - Facts", factsHelp)
    if not facts or facts == "" then
        return
    end

    -- Step 3: Optional MAVLink messages
    local mavlink = gui.inputDialog("FactGroup Generator - MAVLink (optional)",
        "Enter MAVLink message IDs (comma-separated, e.g., GPS_RAW_INT,HEARTBEAT):")

    -- Build arguments
    local args = {
        "--name", name,
        "--facts", facts,
        "--dry-run",
    }
    if mavlink and mavlink ~= "" then
        table.insert(args, "--mavlink")
        table.insert(args, mavlink)
    end

    -- Run generator in dry-run mode first
    runTool("generators/factgroup/cli.py", args,
        function(exitCode, stdout, stderr)
            if exitCode ~= 0 then
                gui.showError("FactGroup Generator", "Generation failed:\n" .. stderr)
                return
            end

            -- Show preview
            local preview = "=== Generated Files Preview ===\n\n" .. stdout

            local confirmed = gui.confirm("FactGroup Generator",
                "Preview generated code?\n\nFiles will be created in src/Vehicle/FactGroups/")

            if confirmed then
                -- Show the generated content
                gui.showText("Generated Code Preview", stdout)

                -- Ask to actually generate
                local generate = gui.confirm("FactGroup Generator",
                    "Generate files to src/Vehicle/FactGroups/?")

                if generate then
                    -- Remove --dry-run and add output path
                    local genArgs = {
                        "--name", name,
                        "--facts", facts,
                        "--output", "src/Vehicle/FactGroups/",
                    }
                    if mavlink and mavlink ~= "" then
                        table.insert(genArgs, "--mavlink")
                        table.insert(genArgs, mavlink)
                    end

                    runTool("generators/factgroup/cli.py", genArgs,
                        function(genExitCode, genStdout, genStderr)
                            if genExitCode ~= 0 then
                                gui.showError("FactGroup Generator", "Failed:\n" .. genStderr)
                            else
                                gui.showInfo("FactGroup Generator",
                                    "Successfully generated FactGroup files!\n\n" .. genStdout)
                            end
                        end)
                end
            end
        end)
end

-- ============================================================================
-- Register Menu Actions
-- ============================================================================

-- Create QGC submenu under Tools
local qgcMenu = a.menu("Tools.QGC", "QGC Tools")

-- Locator actions
a.create({
    id = "QGC.SearchFacts",
    text = "Search Facts...",
    menu = qgcMenu,
    shortcut = "Ctrl+Shift+F",
    onTriggered = function() showLocatorDialog("fact") end,
})

a.create({
    id = "QGC.SearchFactGroups",
    text = "Search FactGroups...",
    menu = qgcMenu,
    shortcut = "Ctrl+Shift+G",
    onTriggered = function() showLocatorDialog("factgroup") end,
})

a.create({
    id = "QGC.SearchMAVLink",
    text = "Search MAVLink Usage...",
    menu = qgcMenu,
    shortcut = "Ctrl+Shift+M",
    onTriggered = function() showLocatorDialog("mavlink") end,
})

-- Separator
a.create({
    id = "QGC.Sep1",
    menu = qgcMenu,
    isSeparator = true,
})

-- Analyzer actions
a.create({
    id = "QGC.AnalyzeFile",
    text = "Check Null Safety (Current File)",
    menu = qgcMenu,
    shortcut = "Ctrl+Shift+N",
    onTriggered = runAnalyzerOnCurrentFile,
})

a.create({
    id = "QGC.AnalyzeProject",
    text = "Check Null Safety (Project)",
    menu = qgcMenu,
    onTriggered = runAnalyzerOnProject,
})

-- Separator
a.create({
    id = "QGC.Sep2",
    menu = qgcMenu,
    isSeparator = true,
})

-- Generator action
a.create({
    id = "QGC.GenerateFactGroup",
    text = "Generate FactGroup...",
    menu = qgcMenu,
    shortcut = "Ctrl+Shift+Alt+G",
    onTriggered = runGeneratorWizard,
})

-- ============================================================================
-- LSP Client Integration
-- ============================================================================

-- Try to register QGC LSP server with Qt Creator's built-in LSP client
local function setupLspClient()
    local ok, lsp = pcall(require, "lsp")
    if not ok then
        print("[QGC Tools] LSP module not available (Qt Creator < 14.0)")
        return false
    end

    local projectRoot = getProjectRoot()
    if not projectRoot then
        print("[QGC Tools] LSP: No project root found, will retry when project opens")
        return false
    end

    -- Check if LSP server exists
    local serverPath = projectRoot .. "/tools/lsp/server.py"
    if not utils.fileExists(serverPath) then
        print("[QGC Tools] LSP: Server not found at " .. serverPath)
        return false
    end

    -- Create LSP client for QGC C++ and Fact JSON files
    local client = lsp.Client.create({
        name = "QGC LSP",
        cmd = function()
            return { Config.python, "-m", "tools.lsp" }
        end,
        transport = "stdio",
        languageFilter = {
            patterns = {
                -- C++ files
                "*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hxx",
                -- Fact JSON files
                "*Fact.json", "*Facts.json", "*FactMetaData.json",
            },
            mimeTypes = {
                "text/x-c++src", "text/x-c++hdr", "text/x-csrc", "text/x-chdr",
                "application/json",
            },
        },
        startBehavior = "RequiresFile",
        settings = {},
        initializationOptions = {
            projectRoot = projectRoot,
        },
    })

    if client then
        print("[QGC Tools] LSP client registered - diagnostics active for C++ files")
        return true
    else
        print("[QGC Tools] LSP: Failed to create client")
        return false
    end
end

-- Attempt LSP setup (may fail if no project is open yet)
local lspOk = setupLspClient()

-- Status message on load
local lspStatus = lspOk and " + LSP diagnostics" or ""
print("[QGC Tools] Extension loaded - access via Tools > QGC menu" .. lspStatus)
