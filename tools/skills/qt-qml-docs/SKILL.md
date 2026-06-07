---
name: qt-qml-docs
description: >-
  Generates standalone Markdown reference documentation for QML components and
  applications. Use this skill whenever you want to document QML files,
  create API reference docs for a QML component or module, document a Qt Quick
  application, or produce developer-facing documentation from .qml source code.
  Triggers on: "document this QML", "write docs for my QML", "create reference
  docs", "document QML component", "QML API docs", "document my Qt Quick
  component", "document my Qt app", or any time one or more .qml files are
  provided and documentation is needed. Works with single files, pasted code,
  or entire project folders. DO NOT use if the user asks for QDoc format output.
license: LicenseRef-Qt-Commercial OR BSD-3-Clause
compatibility: >-
  Designed for Claude Code, GitHub Copilot, and similar agents.
disable-model-invocation: false
metadata:
  author: qt-ai-skills
  version: "1.0"
  qt-version: "6.x"
  category: process
---

# QML Documentation Skill

You are an expert in Qt/QML who writes clear, accurate, developer-friendly reference documentation for QML components. Your task is to read QML source files — along with any related files (C++ backends, QML modules, resource files, CMakeLists.txt, qmldir, etc.) — and produce structured Markdown reference docs that give developers a complete picture of how components fit into the project.

## Core requirements

- **No code snippets (except Usage Example).** Do not wrap any code in markdown code fences, *except* in the Usage Example section (Section 8) for reusable components — see below. Describe code behaviour, method signatures, and property types in prose and tables instead.
- **Context-aware.** Understand how each component fits into the project: what the application/module does, what role this component plays, and what it depends on.
- **Tables for properties.** Always use Markdown tables (not bullet lists) to document properties.
- **Follow project conventions.** Infer and respect any QML development conventions from the project's documentation or code patterns.

## Document structure

For each QML component, generate a Markdown file named `<ComponentName>.md` with the following sections (omit any section that has no content):

### 1. Component Overview
Describe what the application or module does and where this component fits in the project architecture. Then explain what this specific component does — its visual or logical role, when a developer would reach for it, and what problem it solves. Keep this concise: a developer new to the codebase should understand the component's purpose at a glance.

### 2. Project Structure and Dependencies
Explain how the component relates to the project:
- What files import or instantiate it?
- What does it import (Qt Quick modules, custom project QML types, C++ registered types)?
- For **custom QML types**, describe what they provide and where they come from.
- Relevant build or module requirements (e.g. CMake targets, qmldir, qmltypes).

### 3. Component Hierarchy and Role
If the component inherits from or composes other elements, describe the hierarchy. Explain what the base type provides and what this component adds or overrides.

### 4. Properties

Use a Markdown table with these columns:

| Property | Type | Default | Required | Description |
|----------|------|---------|----------|-------------|

- List every declared property, including `property alias` entries.
- For `required` properties, mark the Required column as **Yes**.
- Describe each property in terms of what it *controls* or *enables*.
- For properties that accept a fixed set of values (enums, string literals), list valid values and their meanings.

### 5. Signals

For each signal:
- State its name and parameter list (type and name for each argument).
- Explain *what condition triggers* the signal.
- Describe *what a connected handler is expected to do* in response.

Format as a sub-section per signal: `#### signalName(paramType paramName)`

### 6. Methods

For each function:
- State its name, parameter names and types, and return type (if any).
- Explain what it does and when to call it.
- Note any side effects (e.g. emits a signal, modifies state, restarts a timer).

Format as a sub-section per method: `#### methodName(paramType paramName) : returnType`

### 7. Inter-Component Interactions

Describe how this component communicates with other parts of the application:
- Which properties are driven by external bindings?
- Which signals are consumed by parent or sibling components?
- Which functions are called from outside this file?
- Shared state, models, or singletons it reads from or writes to.

### 8. Usage Example *(reusable components only)*

Include this section only when the component is **reusable** — i.e., it is designed to be instantiated by other QML files rather than serving as a standalone application entry point. A component is reusable when:
- Its root type is **not** `Window` or `ApplicationWindow` (those are top-level application windows, not embeddable pieces).
- It declares one or more `property` entries (especially `required property` or `property alias`) that callers are expected to set.
- Its role is to be composed into larger UIs or used as a building block across the codebase.

Write a short, self-contained snippet showing a developer the minimal correct way to instantiate the component, setting every `required` property and any commonly needed properties.

---

## Pre-flight: check for existing documentation

Before reading any source file, check whether documentation already exists for the files you are about to document. This saves time and lets the user decide whether they want a fresh pass or just an update.

### How to check

1. Identify the expected output location. Documentation is written to a `doc/` subdirectory next to the source files (e.g. if sources are in `src/`, docs go in `src/doc/`). For a single file `Foo.h`, the expected doc is `src/doc/Foo.md`; for `main.cpp` it is `src/doc/main.md`.

2. Check whether the `doc/` directory and the relevant `.md` files already exist. Use the `Glob` tool or run a 'ls' shell command — do not read the source files yet.

3. Act on what you find:

   - **No existing docs found** — proceed normally with reading the source files and generating documentation.

   - **Some or all docs already exist** — do not read the source files yet. Instead, ask the user using `AskUserQuestion` with a multiple-choice reply:

     > "I found existing documentation for [list the files that already have docs]. What would you like me to do?"
     >
     > Options:
     > - **Update existing docs** — re-read the source files and rewrite the affected `.md` files in place.
     > - **Skip files that already have docs** — only generate docs for source files that are missing documentation.
     > - **Generate fresh docs for everything** — overwrite all existing docs unconditionally.
     > - **Cancel** — stop here; make no changes.

   Wait for the user's choice before doing anything else.

4. Honour the user's choice:
   - *Update* or *Generate fresh* → read all relevant source files and proceed normally, overwriting the existing `.md` files.
   - *Skip* → read only the source files that are missing a corresponding `.md`, and generate docs only for those.
   - *Cancel* → stop and confirm to the user that nothing was changed.

## Input handling

**Single file or pasted code:** Document just that component. Infer application context from imports, property names, and the component's structure.

**Folder / project:** Walk the directory tree, find all `.qml` files. Also read any `CMakeLists.txt`, `qmldir`, or C++ header files — they provide context about module structure and registered types. Generate one `.md` per component. **If documenting more than one file**, also create a `doc/index.md` that lists every component with a one-line description and links.

---

## Parsing QML accurately

Read the source carefully:

- The **root element** is the base type; note what it inherits.
- `property <type> <name>: <default>` — custom property with optional default.
- `property alias <name>: <target>` — alias; document as type matching the target.
- `required property` — must be explicitly set by the user of this component.
- `signal <name>(<params>)` — custom signal.
- `function <name>(<params>) { }` — JS function.
- `readonly property` — cannot be set externally; document as read-only.
- `component <Name> : BaseType { }` — inline component definition; document as a separate component within the same file.
- Internal helpers prefixed with `_` are usually private — skip them unless clearly intended as public API.
- If a property lacks a clear description, use its name, type, and usage context to infer a meaningful one.

---

## Tone and style

- Write for a developer who knows QML but has not seen this component before.
- Be precise about types: `string`, `int`, `real`, `color`, `bool`, `var`, `list<Type>`, etc.
- Use present tense: "Controls the width…" not "Will control…"
- Avoid filler: be direct and descriptive.
- Describe behaviour, not implementation: explain *what* happens.
- When the accepted values of a property are a fixed set, always enumerate them in the description.

---

## Output location

- Generate docs in a `doc/` subdirectory next to the source QML files.
- **Only create a `doc/index.md` if documenting 2 or more components.** For single-file documentation, just create the component `.md` file.

---

## Quality check

Before saving, verify:
- Every property, signal, and function is documented — nothing is silently skipped.
- Inter-Component Interactions is filled in wherever there are observable bindings or external calls.
- Documentation is project-agnostic and does not assume details not evident in the code or provided context.

---

AI assistance has been used to create this output.
