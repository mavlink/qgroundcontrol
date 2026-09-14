"""QML rendering for config page definitions."""

from __future__ import annotations

from pathlib import Path

from .._jinja import make_env
from ..common.controls import qml_tr
from .control_renderers import render_control as _qml_control
from .model import (
    PageDef,
    SectionDef,
    _build_search_terms,
    _build_translatable_terms,
    _propagate_optional,
)

_env = make_env(Path(__file__).parent / "templates")

_HEADER = """\
// This file is auto-generated. Do not edit.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls
"""


def _section_visible(sec: SectionDef) -> str:
    # The untranslated title is the section ID (must match VehicleComponent::sectionIds())
    name_vis = f'sectionMatchesFilter("{sec.title}")'
    return f"{name_vis} && {sec.showWhen}" if sec.showWhen else name_vis


def _qml_generated_section(sec: SectionDef, tr_context: str = "") -> str:
    """Generate QML for a section with auto-generated controls."""
    ind = "                "
    ctrl_indent = ind + "    "
    controls = [_qml_control(ctrl, ctrl_indent, tr_context=tr_context) for ctrl in sec.controls]
    return _env.get_template("generated_section.qml.j2").render(
        ind=ind,
        visible=_section_visible(sec),
        title_qtr=qml_tr(sec.title, tr_context),
        image=sec.image,
        controls=controls,
    )


def _qml_component_section(sec: SectionDef, tr_context: str = "") -> str:
    """Generate QML for a component escape-hatch section."""
    return _env.get_template("component_section.qml.j2").render(
        ind="                ",
        title_qtr=qml_tr(sec.title, tr_context),
        visible=_section_visible(sec),
        component=sec.component,
    )


def _safe_id(name: str) -> str:
    """Convert a section name to a safe QML identifier."""
    return "".join(c if c.isalnum() else "_" for c in name).lower()


def _qml_repeat_count_property(sec: SectionDef) -> str:
    """Generate the count property for a repeat section (emitted at Item level)."""
    rep = sec.repeat
    assert rep is not None
    ind = "            "  # Item-level indent
    safe = _safe_id(sec.title)

    if rep.indexing == "apm_battery":
        return _env.get_template("repeat_count_apm_battery.qml.j2").render(
            ind=ind,
            safe=safe,
            param_prefix=rep.paramPrefix,
            probe_postfix=rep.probePostfix,
        )

    idx_expr = f'(_i === {rep.startIndex}) ? "" : _i' if rep.firstIndexOmitsNumber else "_i"
    return _env.get_template("repeat_count_default.qml.j2").render(
        ind=ind,
        safe=safe,
        start_index=rep.startIndex,
        idx_expr=idx_expr,
        probe_param=f'"{rep.paramPrefix}" + _idx + "{rep.probePostfix}"',
    )


def _qml_repeat_section(sec: SectionDef, sec_idx: int, tr_context: str = "") -> str:
    """Generate QML for a repeated (indexed) section using a Repeater."""
    rep = sec.repeat
    assert rep is not None
    ind = "                "

    # Filter on the untranslated per-index section ID, not the translated heading
    name_vis = f'sectionMatchesFilter(_sectionId, "{sec.title}")'
    show_vis = f"{name_vis} && {sec.showWhen}" if sec.showWhen else name_vis
    safe = _safe_id(sec.title)

    if rep.enableParam:
        dv = rep.disabledParamValue
        enable_expr = (
            f'controller.getParameterFact(-1, _fullParamName("{rep.enableParam}")).value !== {dv}'
        )
        visible = f"{show_vis} && {enable_expr}"
    else:
        visible = show_vis

    heading_base = sec.title.replace("{index}", '" + _displayIndex + "')
    if "{index}" in sec.title:
        heading_expr = qml_tr(heading_base, tr_context)
    else:
        heading_expr = (
            f'_{safe}Count > 1 ? {qml_tr(sec.title, tr_context)} + " " + _displayIndex '
            f": {qml_tr(sec.title, tr_context)}"
        )

    apm_battery = rep.indexing == "apm_battery"
    if rep.firstIndexOmitsNumber:
        index_str_expr = f'(_rawIndex === {rep.startIndex}) ? "" : String(_rawIndex)'
    else:
        index_str_expr = "String(_rawIndex)"

    ctrl_indent = ind + "        "
    dialog_counter = [0]
    controls = [
        _qml_control(
            ctrl, ctrl_indent, indexed=True, dialog_counter=dialog_counter, tr_context=tr_context
        )
        for ctrl in sec.controls
    ]

    disabled_companion = None
    if rep.enableParam and rep.disabledSection:
        disabled_companion = _qml_disabled_companion_section(sec, tr_context=tr_context)

    return (
        _env.get_template("repeat_section.qml.j2")
        .render(
            ind=ind,
            safe=safe,
            visible=visible,
            raw_title=sec.title,
            heading_expr=heading_expr,
            image=sec.image,
            apm_battery=apm_battery,
            start_index=rep.startIndex,
            index_str_expr=index_str_expr,
            param_prefix=rep.paramPrefix,
            controls=controls,
            disabled_companion=disabled_companion,
        )
        .rstrip("\n")
    )


def _qml_disabled_companion_section(sec: SectionDef, tr_context: str = "") -> str:
    """Emit a compact section showing only the enable-param control for disabled items."""
    rep = sec.repeat
    assert rep is not None
    assert rep.disabledSection is not None
    ind = "                "
    safe = _safe_id(sec.title)
    dv = rep.disabledParamValue
    camel = "".join(w.capitalize() for w in safe.split("_") if w)
    apm_battery = rep.indexing == "apm_battery"

    # idx_init_expr: used in the for-loop and as the _idx property initializer
    if rep.firstIndexOmitsNumber:
        loop_idx_expr = (
            f'(i + {rep.startIndex}) === {rep.startIndex} ? "" : String(i + {rep.startIndex})'
        )
        item_idx_expr = f'(index + {rep.startIndex}) === {rep.startIndex} ? "" : String(index + {rep.startIndex})'
    else:
        loop_idx_expr = f"String(i + {rep.startIndex})"
        item_idx_expr = f"String(index + {rep.startIndex})"

    title_qtr = qml_tr(sec.title, tr_context)
    if apm_battery:
        fact_expr = (
            f'controller.getParameterFact(-1, _battPrefixForIndex(index) + "{rep.enableParam}")'
        )
        label_expr = (
            f'_{safe}Count > 1 ? {title_qtr} + " " + _battLabelForIndex(index) : {title_qtr}'
        )
    else:
        fact_expr = (
            f'controller.getParameterFact(-1, "{rep.paramPrefix}" + _idx + "{rep.enableParam}")'
        )
        label_expr = (
            f'_{safe}Count > 1 ? {title_qtr} + " " + String(index + {rep.startIndex}) : {title_qtr}'
        )

    return (
        _env.get_template("disabled_companion.qml.j2")
        .render(
            ind=ind,
            safe=safe,
            camel=camel,
            dv=dv,
            apm_battery=apm_battery,
            enable_param=rep.enableParam,
            param_prefix=rep.paramPrefix,
            loop_idx_expr=loop_idx_expr,
            disabled_heading=rep.disabledSection.heading,
            disabled_heading_qtr=qml_tr(rep.disabledSection.heading, tr_context),
            use_checkbox=bool(rep.disabledSection.enabledParamValue),
            enabled_value=rep.disabledSection.enabledParamValue,
            fact_expr=fact_expr,
            label_expr=label_expr,
            item_idx_expr=item_idx_expr,
        )
        .rstrip("\n")
    )


def _param_line(name: str, p) -> str:
    if p.existsOnly:
        return f'property var {name}: controller.parameterExists(-1, "{p.name}")'
    if p.required:
        return f'property var {name}: controller.getParameterFact(-1, "{p.name}")'
    return f'property var {name}: controller.getParameterFact(-1, "{p.name}", false)'


def _terms_entries(terms_map: dict) -> list[dict]:
    items = list(terms_map.items())
    return [
        {
            "section_id": section_id,
            "terms": ", ".join(f'"{t}"' for t in terms),
            "comma": "," if i < len(items) - 1 else "",
        }
        for i, (section_id, terms) in enumerate(items)
    ]


def generate_config_page_qml(page: PageDef) -> str:
    """Generate a complete QML file for a vehicle config page."""
    _propagate_optional(page)
    tr_ctx = page.json_filename

    repeat_count_props = [_qml_repeat_count_property(sec) for sec in page.sections if sec.repeat]

    visible_conditions_map: dict[str, list[str]] = {}
    for sec in page.sections:
        if sec.showWhen:
            visible_conditions_map.setdefault(sec.title, []).append(sec.showWhen)
    visible_conditions = [
        {"name": name, "expr": " || ".join(dict.fromkeys(conditions))}
        for name, conditions in visible_conditions_map.items()
    ]

    sections_qml: list[str] = []
    for sec_idx, sec in enumerate(page.sections):
        if sec.repeat:
            sections_qml.append(_qml_repeat_section(sec, sec_idx, tr_context=tr_ctx))
        elif sec.component:
            sections_qml.append(_qml_component_section(sec, tr_context=tr_ctx))
        else:
            sections_qml.append(_qml_generated_section(sec, tr_context=tr_ctx))

    return (
        _env.get_template("page.qml.j2").render(
            header=_HEADER.rstrip("\n"),
            imports=page.imports,
            controller_type=page.controllerType,
            constants=list(page.constants.items()),
            param_lines=[_param_line(name, p) for name, p in page.params.items()],
            bindings=list(page.bindings.items()),
            repeat_count_props=repeat_count_props,
            search_terms=_terms_entries(_build_search_terms(page)),
            tr_terms=_terms_entries(_build_translatable_terms(page)),
            tr_ctx=tr_ctx,
            visible_conditions=visible_conditions,
            sections=sections_qml,
        )
        + "\n"
    )


def get_section_names(page: PageDef) -> list[str]:
    """Return the display names of all sections in a page."""
    names = [sec.title for sec in page.sections if sec.title]
    for sec in page.sections:
        if sec.repeat and sec.repeat.disabledSection:
            names.append(sec.repeat.disabledSection.heading)
    return names
