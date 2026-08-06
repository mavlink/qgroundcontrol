# QGroundControl string translations

QGC uses the standard Qt Linguist mechanism for string translation, sourced through a crowd-sourced
[Crowdin project](https://crowdin.com/project/qgroundcontrol).

## Crowdin integration

Crowdin synchronizes `qgc.ts` once a day, picking up new changes automatically and opening a pull
request with the resulting translations.

### Adding a new language

Add the language from the Crowdin settings.

### Updating base translation files during the release cycle

The `lupdate.yml` workflow runs every Sunday to regenerate the `.ts` source files and open a PR with
any changes. Trigger it manually from the Actions tab, or run it locally from the repo root:

```bash
python3 tools/translations/qgc_lupdate.py
```

Crowdin picks these up and submits translations back as they become available.

## C++ and QML code strings

Use the standard Qt `tr()` (C++) and `qsTr()` (QML) mechanisms.

## Translating strings in JSON files

QGC stores metadata in JSON files that also need translation. The
[JSON parser](https://github.com/mavlink/qgroundcontrol/blob/master/tools/translations/qgc_lupdate_json.py)
finds every JSON file in the source tree, extracts translatable strings, and emits a Qt `.ts`
localization file. To mark which strings to translate, add keys at the root object level.

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
