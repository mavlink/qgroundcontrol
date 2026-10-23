# QGroundControl string translations

QGC uses the standard Qt Linguist mechanism for string translation, sourced through a crowd-sourced
[Crowdin project](https://crowdin.com/project/qgroundcontrol).

## Crowdin integration

Crowdin's GitHub integration ([`crowdin.yml`](../../crowdin.yml)) synchronizes `qgc.ts` and
`qgc-json.ts` once a day, picking up new changes automatically and opening a pull request with the
resulting translations.

### Adding a new language

Add the language from the Crowdin settings.

### Updating base translation files during the release cycle

The `lupdate.yml` workflow runs every Sunday to regenerate the `.ts` source files and open a PR with
any changes. Trigger it manually from the Actions tab, or run it locally from the repo root:

```bash
just translations   # Runs tools/translations/qgc_lupdate.py
```

`qgc_lupdate.py` runs Qt's `lupdate` (from `QT_ROOT_DIR`, a standard Qt install, or `PATH`), then
the JSON string extractor (`qgc_lupdate_json.py`), then `pseudo_loc.py`, which writes the
pseudo-localized `eo` catalogs used for layout testing.

Crowdin picks these up and submits translations back as they become available.

## Placeholder regression prevention

`check_placeholders.py` compares parsed TS messages against a Git baseline, not added diff lines.
It checks new messages and changed source/translation values, translation status, numerus status,
or language. Context, source, disambiguation comment, and message ID identify a message.
Locations, translator notes, XML formatting, message order, and deleted messages do not trigger
placeholder checks. Existing unchanged placeholder defects remain out of scope, even when another
message in the same catalog changes.

From the repository root:

```bash
uv run --directory tools --group scripts python -m translations.check_placeholders --base-ref HEAD
uv run --directory tools --group scripts python -m translations.check_placeholders \
  --base-ref origin/master translations/qgc_source_ko_KR.ts
uv run --project tools --group scripts --group test pytest -q tools/tests/test_translation_placeholders.py
```

The validator discovers changed tracked catalogs and untracked catalogs under `translations/`.
Optional paths are repository-relative. `--base-ref` selects the exact baseline tree; it does not
silently substitute a merge base. Locally, the default is `HEAD`; pre-commit's
`PRE_COMMIT_FROM_REF` overrides that default when comparing revisions. `--base-ref EMPTY` explicitly
treats all current messages as new. Missing refs/history, Git failures, unreadable files, malformed
XML, entity declarations, and ambiguous message identities fail with diagnostics.

Every nonempty translation and each supplied plural/length variant must preserve the source's
set of `%1` through `%99`, `%L1` through `%L99`, `%n`, and `%Ln` placeholders. Reordering and
repetition are allowed; localized and nonlocalized markers are distinct. Missing, extra, and
malformed markers such as `% 1`, `%L 1`, `%0`, or `%100` fail. Literal percent text already present
in the source is not treated as a newly malformed marker.

QGC uses default `lrelease` options (no `-nounfinished`): nonempty unfinished translations can ship
and are checked. Vanished/obsolete messages are ignored. Empty translations use Qt's fallback;
whitespace-only translations are not empty. An unfinished non-ID message with an empty first plural
form is omitted entirely by `lrelease`, so its other forms are also ignored. ID-based unfinished
messages can retain nonempty forms and are checked. The validator checks supplied plural forms
without installing Qt or enforcing a locale-specific plural count; it does not validate translation
quality, accelerator keys, or punctuation.

The `qt-translation-placeholders` pre-commit hook checks passed catalogs against the baseline.
`pre-commit.yml` runs on every path, so translation-only/Crowdin PRs and pushes to
`master`/`Stable*` get it too, without installing Qt (only QML changes need it). Its `--changed`
run compares the PR merge result with the merge's first parent, or a push with its `before` SHA,
and prek hands that baseline to the hook as `PRE_COMMIT_FROM_REF`.

## C++ and QML code strings

Use the standard Qt `tr()` (C++) and `qsTr()` (QML) mechanisms.

## Translating strings in JSON files

QGC stores metadata in JSON files that also need translation. The
[JSON parser](qgc_lupdate_json.py) finds every JSON file in the source tree, extracts translatable
strings, and emits a Qt `.ts` localization file. It walks `src/` in sorted order and, like `lupdate`,
records locations relative to the `.ts` file (`../src/...`), so the output does not depend on the
checkout path or filesystem order. To mark which strings to translate, add keys at the root object
level.

> **Important:** JSON files may not share a name — the filename is the translation-lookup context and
> must be unique (duplicates raise a parser error). The root filename must also match the Qt resource
> alias, because the parser sees filesystem names while QGC C++ reads files through the Qt resource
> system (which sees the alias path).

### Known file types

The parser supports two known types. Set a root `fileType` key to `MAVCmdInfo` or `FactMetaData` to
apply its default instructions:

```json
// MAVCmdInfo
"translateKeys": "label,enumStrings,friendlyName,description",
"arrayIDKeys":   "rawName,comment"

// FactMetaData
"translateKeys": "shortDescription,longDescription,enumStrings",
"arrayIDKeys":   "name"
```

### Manual parser instructions

Omit `fileType` and set these root keys as needed:

- `translateKeys` — comma-separated list of keys to translate.
- `arrayIDKeys` — for arrays, the per-element key(s) whose value uniquely identifies an entry, so the
  translator gets meaningful context instead of a bare array index.

### Disambiguation

When two equal strings in the same file need different translations, prefix the value with a
disambiguation marker:

```json
"foo": "#loc.disambiguation#This is the foo version of baz#baz"
"bar": "#loc.disambiguation#This is the bar version of baz#baz"
```

Here `baz` is the shared string; `#loc.disambiguation#...#` carries a note to the translator, with the
actual string following the final `#`.
