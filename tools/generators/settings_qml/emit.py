"""QML emitters for SettingsUI pages."""

from __future__ import annotations

import json
import re
from pathlib import Path

from .._jinja import make_env
from ..common.controls import (
    qml_tr,
    render_checkbox,
    render_combobox,
    render_slider,
    render_textfield,
)
from ..common.validation import reject_unknown_keys, require_list
from .metadata import get_fact_type, has_enum_strings
from .model import ControlDef, PageDef, load_page_def

_env = make_env(Path(__file__).parent / "templates")


def _object_name(text: str) -> str:
    """Sanitize free text (headings, page names) for use inside a QML objectName
    string literal. Restricting to identifier characters guarantees the generated
    QML is always syntactically valid and keeps objectNames grep-able for UI tests."""
    return re.sub(r"[^A-Za-z0-9_]", "", text)


def _wrap_with_description(control_qml: str, fact_ref: str, vis_expr: str, indent: str) -> str:
    """Wrap a control's QML in a ColumnLayout that appends a shortDescription label."""
    control_qml_indented = "\n".join(
        (f"    {line}" if line.strip() else line) for line in control_qml.splitlines()
    )
    return _env.get_template("wrap_with_description.qml.j2").render(
        indent=indent,
        vis_expr=vis_expr,
        fact_ref=fact_ref,
        control_qml_indented=control_qml_indented,
    )


def _qml_control(ctrl: ControlDef, settings_dir: Path, json_context: str = "") -> str:
    """Generate QML for a single control."""
    indent = "        "
    fact_ref = f"QGroundControl.settingsManager.{ctrl.setting}"

    def _vis_expr() -> str:
        if ctrl.showWhen:
            return f"({ctrl.showWhen}) && {fact_ref}.userVisible"
        return f"{fact_ref}.userVisible"

    if ctrl.control == "component":
        return _env.get_template("control_component.qml.j2").render(
            indent=indent,
            component=ctrl.component,
            show_when=ctrl.showWhen,
            enable_when=ctrl.enableWhen,
        )
    elif ctrl.control == "info":
        label_expr = qml_tr(ctrl.label, json_context) if ctrl.label else '""'
        has_button = ctrl.button is not None and ctrl.button.text
        if has_button:
            assert ctrl.button is not None
            return _env.get_template("control_info_button.qml.j2").render(
                indent=indent,
                inner_indent=indent + "    ",
                show_when=ctrl.showWhen,
                enable_when=ctrl.enableWhen,
                label_expr=label_expr,
                value=ctrl.value,
                button_text=qml_tr(ctrl.button.text, json_context),
                button_on_clicked=ctrl.button.onClicked,
                button_enabled=ctrl.button.enabled,
            )
        return _env.get_template("control_info_label.qml.j2").render(
            indent=indent,
            label_expr=label_expr,
            value=ctrl.value,
            show_when=ctrl.showWhen,
            enable_when=ctrl.enableWhen,
        )
    elif ctrl.control == "slider":
        control_qml = render_slider(
            fact_ref, indent,
            label=ctrl.label,
            enable_checkbox=ctrl.enableCheckbox,
            button=ctrl.button,
            enable_when=ctrl.enableWhen,
            tr_context=json_context,
        )
        return _wrap_with_description(control_qml, fact_ref, _vis_expr(), indent)
    elif ctrl.control in ("browse", "scaler"):
        qml_type = "LabelledFactBrowse" if ctrl.control == "browse" else "LabelledFactIncrementer"
        label_expr = qml_tr(ctrl.label, json_context) if ctrl.label else "fact.label"
        control_qml = _env.get_template("control_labelled_fact.qml.j2").render(
            indent=indent,
            qml_type=qml_type,
            label_expr=label_expr,
            fact_ref=fact_ref,
            enable_when=ctrl.enableWhen,
        )
        return _wrap_with_description(control_qml, fact_ref, _vis_expr(), indent)
    elif ctrl.control == "checkbox":
        use_checkbox, use_combobox = True, False
    elif ctrl.control == "combobox":
        use_checkbox, use_combobox = False, True
    elif ctrl.control == "textfield":
        use_checkbox, use_combobox = False, False
    else:
        fact_type = get_fact_type(ctrl.setting, settings_dir)
        has_enums = has_enum_strings(ctrl.setting, settings_dir)
        use_checkbox = fact_type == "bool"
        use_combobox = not use_checkbox and has_enums

    if use_checkbox:
        control_qml = render_checkbox(
            fact_ref, indent,
            label=ctrl.label,
            enable_when=ctrl.enableWhen,
            label_property="text",
            label_source="fact.label",
            qml_type="FactCheckBoxSlider",
            tr_context=json_context,
            object_name=f"settingsCheckBox_{ctrl.fact_name}",
        )
        return _wrap_with_description(control_qml, fact_ref, _vis_expr(), indent)
    if use_combobox:
        control_qml = render_combobox(
            fact_ref, indent,
            label=ctrl.label,
            enable_when=ctrl.enableWhen,
            label_source="fact.label",
            qml_type="LabelledFactComboBox",
            tr_context=json_context,
        )
        return _wrap_with_description(control_qml, fact_ref, _vis_expr(), indent)

    fact_type = get_fact_type(ctrl.setting, settings_dir)
    extra: list[str] = [f'objectName: "settingsTextField_{ctrl.fact_name}"']
    if fact_type == "string":
        extra.append("textFieldPreferredWidth: _stringFieldWidth")
    control_qml = render_textfield(
        fact_ref, indent,
        label=ctrl.label,
        enable_when=ctrl.enableWhen,
        placeholder=ctrl.placeholder,
        label_source="fact.label",
        qml_type="LabelledFactTextField",
        extra_lines=extra if extra else None,
        tr_context=json_context,
    )
    return _wrap_with_description(control_qml, fact_ref, _vis_expr(), indent)


def _qml_missing_placeholder(description: str) -> str:
    return _env.get_template("missing_placeholder.qml.j2").render(description=description)


def _needs_string_field_width(page: PageDef, settings_dir: Path) -> bool:
    for grp in page.groups:
        for ctrl in grp.controls:
            fact_type = get_fact_type(ctrl.setting, settings_dir)
            has_enums = has_enum_strings(ctrl.setting, settings_dir)
            if fact_type == "string" and not has_enums:
                return True
    return False


def _binding_qml_type(expr: str) -> str:
    if expr.startswith(("QGroundControl.", "_settingsManager.")):
        return "var"
    if any(op in expr for op in ["&&", "||", "===", "!=="]):
        return "bool"
    if expr.startswith(("qsTr(", '"')):
        return "string"
    return "var"


def _group_auto_vis(grp) -> str:
    fact_refs = [
        f"QGroundControl.settingsManager.{c.setting}"
        for c in grp.controls
        if c.setting
    ]
    return " || ".join(f"{ref}.userVisible" for ref in fact_refs)


def generate_page_qml(
    page: PageDef, settings_dir: Path, json_context: str = "", page_name: str = ""
) -> str:
    """Generate a complete QML settings page from a page definition."""
    _tr = (
        (lambda s: f'qsTranslate("{json_context}", "{s}")')
        if json_context
        else (lambda s: f'qsTr("{s}")')
    )

    bindings = [
        {"type": _binding_qml_type(expr), "name": name, "expr": expr}
        for name, expr in page.bindings.items()
    ]

    section_cases: list[dict] = []
    for grp_idx, grp in enumerate(page.groups):
        vis_parts: list[str] = []
        if grp.showWhen:
            vis_parts.append(f"({grp.showWhen})")
        auto_vis = _group_auto_vis(grp)
        if auto_vis:
            vis_parts.append(f"({auto_vis})")
        if vis_parts:
            section_cases.append({"idx": grp_idx, "expr": " && ".join(vis_parts)})

    group_blocks: list[str] = []
    seen_object_names: dict[str, str] = {}  # objectName -> heading that produced it
    for grp_idx, grp in enumerate(page.groups):
        section_vis = f"(sectionFilter === -1 || sectionFilter === {grp_idx})"
        vis_parts = [section_vis]
        if grp.showWhen:
            vis_parts.append(f"({grp.showWhen})")
        auto_vis = _group_auto_vis(grp)
        if auto_vis:
            vis_parts.append(f"({auto_vis})")
        visible_expr = " && ".join(vis_parts)

        if grp.component:
            group_blocks.append(_env.get_template("group_component.qml.j2").render(
                visible=visible_expr,
                component=grp.component,
            ))
            continue

        # The sanitizer is lossy, so guard the objectName invariants at generation time:
        # UI tests look groups up by objectName and would silently match the wrong one
        group_object_name = None
        if grp.heading:
            sanitized = _object_name(grp.heading)
            if not sanitized:
                raise ValueError(
                    f"Page '{page_name or json_context}': heading '{grp.heading}' sanitizes to an "
                    f"empty objectName. Use a heading with ASCII letters or digits."
                )
            group_object_name = f"settingsGroup_{sanitized}"
            if group_object_name in seen_object_names:
                raise ValueError(
                    f"Page '{page_name or json_context}': headings '{seen_object_names[group_object_name]}' "
                    f"and '{grp.heading}' both produce objectName '{group_object_name}'. Rename one."
                )
            seen_object_names[group_object_name] = grp.heading

        blocks = [_qml_control(ctrl, settings_dir, json_context) for ctrl in grp.controls]
        blocks.extend(_qml_missing_placeholder(desc) for desc in grp.missing)
        group_blocks.append(_env.get_template("group_settings.qml.j2").render(
            object_name=group_object_name,
            heading=_tr(grp.heading) if grp.heading else None,
            heading_description=grp.headingDescription,
            visible=visible_expr,
            enable_when=grp.enableWhen,
            blocks=blocks,
        ))

    page_object_name = None
    if page_name:
        page_object_name = _object_name(page_name)
        if not page_object_name:
            raise ValueError(
                f"Page '{page_name}': page name sanitizes to an empty objectName. "
                f"Use a name with ASCII letters or digits."
            )

    return _env.get_template("page.qml.j2").render(
        imports=page.imports,
        object_name=page_object_name,
        has_string_fields=_needs_string_field_width(page, settings_dir),
        bindings=bindings,
        section_cases=section_cases,
        groups=group_blocks,
    ) + "\n"


_ALLOWED_PAGES_ROOT_KEYS = frozenset({"fileType", "version", "comment", "pages"})
_ALLOWED_PAGE_ENTRY_KEYS = frozenset({
    "comment", "divider", "name", "url", "qml", "icon", "visible", "pageDefinition",
})


def generate_pages_model_qml(pages_json_path: Path) -> str:
    """Generate SettingsPagesModel.qml from SettingsPages.json."""
    with open(pages_json_path, encoding="utf-8") as f:
        data = json.load(f)

    reject_unknown_keys(data, _ALLOWED_PAGES_ROOT_KEYS, "pages file", pages_json_path)

    pages_dir = pages_json_path.parent
    entries: list[dict] = []

    for entry in require_list(data.get("pages", []), "'pages'", pages_json_path):
        reject_unknown_keys(entry, _ALLOWED_PAGE_ENTRY_KEYS, "page entry", pages_json_path)
        if entry.get("divider"):
            entries.append({"divider": True})
            continue

        name = entry["name"]
        url = entry.get("url") or f"qrc:/qml/QGroundControl/AppSettings/{entry['qml']}"

        sections: list[str] = []
        search_terms: list[dict] = []
        translatable_terms: list[dict] = []
        page_def_name = entry.get("pageDefinition")
        if page_def_name:
            page_def_path = pages_dir / page_def_name
            if page_def_path.exists():
                page_def = load_page_def(page_def_path)
                for grp_idx, grp in enumerate(page_def.groups):
                    section_name = grp.display_name
                    sections.append(section_name)

                    terms_parts = [name.lower(), section_name.lower()]
                    terms_parts.extend(kw.lower() for kw in grp.keywords)
                    terms_parts.extend(c.label.lower() for c in grp.controls if c.label)
                    search_terms.append({
                        "section": grp_idx,
                        "terms": " ".join(dict.fromkeys(terms_parts)),
                    })

                    tr_parts: list[str] = [section_name, *grp.keywords]
                    tr_parts.extend(c.label for c in grp.controls if c.label)
                    translatable_terms.append({
                        "section": grp_idx,
                        "context": page_def_name,
                        "terms": list(dict.fromkeys(tr_parts)),
                    })

        entries.append({
            "divider": False,
            "name": name,
            "url": url,
            "icon": entry["icon"],
            "sections_json": json.dumps(sections),
            "search_json": json.dumps(search_terms).replace("'", "\\'"),
            "translatable_json": json.dumps(translatable_terms).replace("'", "\\'"),
            "visible": entry.get("visible", ""),
        })

    return _env.get_template("pages_model.qml.j2").render(entries=entries) + "\n"
