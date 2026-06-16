"""QML rendering for config page definitions."""

from __future__ import annotations

import json
from pathlib import Path

from .._jinja import make_env
from ..common.controls import (
    qml_tr,
    render_action_button,
    render_bitmask,
    render_bitmask_checkbox,
    render_checkbox,
    render_combobox,
    render_dialog_button,
    render_factslider,
    render_label,
    render_radiogroup,
    render_slider,
    render_textfield,
    render_toggle_checkbox,
)
from .model import (
    ControlDef,
    PageDef,
    SectionDef,
    _build_search_terms,
    _build_translatable_terms,
    _propagate_optional,
    _vis_expr,
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


def _detect_control_type(ctrl: ControlDef) -> str:
    """Detect the best control type for a parameter when not explicitly set."""
    if ctrl.control:
        return ctrl.control
    return "textfield"


def _fact_ref(ctrl: ControlDef, indexed: bool = False) -> str:
    """Return the QML expression for the control's fact binding.

    When *indexed* is True the control lives inside a Repeater delegate and
    ``ctrl.param`` is a postfix (e.g. ``_SOURCE``).  The full param name is
    built at runtime from ``_fullParamName`` which is a JS function defined
    by the repeat section.
    """
    if ctrl.setting:
        return f"QGroundControl.settingsManager.{ctrl.setting}"
    if indexed:
        if ctrl.optional:
            return f'controller.getParameterFact(-1, _fullParamName("{ctrl.param}"), false)'
        return f'controller.getParameterFact(-1, _fullParamName("{ctrl.param}"))'
    if ctrl.optional:
        return f'controller.getParameterFact(-1, "{ctrl.param}", false)'
    return f'controller.getParameterFact(-1, "{ctrl.param}")'


def _qml_control(
    ctrl: ControlDef,
    indent: str,
    *,
    indexed: bool = False,
    dialog_counter: list[int] | None = None,
    tr_context: str = "",
) -> str:
    """Generate QML for a single control inside a ConfigSection ColumnLayout."""
    fact_ref = _fact_ref(ctrl, indexed=indexed)
    control_type = _detect_control_type(ctrl)

    def _apply_indent(qml: str) -> str:
        """Wrap output with Layout.leftMargin when ctrl.indent is set."""
        if ctrl.indent:
            qml = _inject_prop(
                qml, f"{indent}    Layout.leftMargin: ScreenTools.defaultFontPixelWidth * 2"
            )
        return qml

    if control_type == "component" and ctrl.component:
        qml = (
            _env.get_template("control_component.qml.j2")
            .render(
                indent=indent,
                component=ctrl.component,
                show_when=ctrl.showWhen,
            )
            .rstrip("\n")
        )
        return _apply_indent(qml)

    if control_type == "label":
        qml = render_label(
            indent,
            text=ctrl.label,
            warning=ctrl.warning,
            small_font=ctrl.smallFont,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    if control_type == "dialogButton" and ctrl.dialogButton:
        if dialog_counter is None:
            dialog_counter = [0]
        factory_id = f"_dlgFactory{dialog_counter[0]}"
        dialog_counter[0] += 1

        if ctrl.param:
            comp_id = f"{factory_id}Component"
            params_js = ", ".join(f'"{k}": {v}' for k, v in ctrl.dialogButton.dialogParams.items())
            open_arg = f"{{ {params_js} }}" if params_js else ""
            ri = indent + "    "

            label_text = ctrl.label if ctrl.label else f"{fact_ref}.shortDescription"
            label_qtr = qml_tr(label_text, tr_context) if ctrl.label else label_text

            textfield_qml = ""
            if ctrl.dialogButton.buttonAfter:
                textfield_qml = render_textfield(
                    fact_ref,
                    ri,
                    label=ctrl.label,
                    enable_when=ctrl.enableWhen,
                    label_source=f"{fact_ref}.shortDescription",
                    qml_type="LabelledFactTextField",
                    tr_context=tr_context,
                )

            qml = (
                _env.get_template("control_dialog_with_param.qml.j2")
                .render(
                    indent=indent,
                    ri=ri,
                    factory_id=factory_id,
                    comp_id=comp_id,
                    dialog_component=ctrl.dialogButton.dialogComponent,
                    button_after=bool(ctrl.dialogButton.buttonAfter),
                    textfield_qml=textfield_qml,
                    button_text=qml_tr(ctrl.dialogButton.text, tr_context),
                    open_arg=open_arg,
                    enable_when=ctrl.enableWhen,
                    label_qtr=label_qtr,
                    fact_ref=fact_ref,
                )
                .rstrip("\n")
            )
        else:
            qml = render_dialog_button(
                indent,
                dialog_button=ctrl.dialogButton,
                factory_id=factory_id,
                enable_when=ctrl.enableWhen,
                tr_context=tr_context,
            )

        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, fact_ref)
        if _v:
            qml = (
                f"{indent}ColumnLayout {{\n{indent}    visible: {_v}\n"
                + "\n".join(f"    {line}" for line in qml.splitlines())
                + f"\n{indent}}}"
            )
        return _apply_indent(qml)

    if control_type == "actionButton" and ctrl.actionButton:
        qml = render_action_button(
            indent,
            action_button=ctrl.actionButton,
            enable_when=ctrl.enableWhen,
            tr_context=tr_context,
        )
        if ctrl.showWhen:
            qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
        return _apply_indent(qml)

    if control_type == "factslider":
        qml = render_factslider(
            fact_ref,
            indent,
            label=ctrl.label,
            description=ctrl.description,
            from_=ctrl.sliderFrom,
            to=ctrl.sliderTo,
            major_tick_step_size=ctrl.majorTickStepSize,
            decimal_places=ctrl.decimalPlaces,
            linked_params=ctrl.linkedParams if ctrl.linkedParams else None,
            show_when=_vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, fact_ref),
            enable_when=ctrl.enableWhen,
            tr_context=tr_context,
        )
        return _apply_indent(qml)

    if control_type == "slider":
        qml = render_slider(
            fact_ref,
            indent,
            label=ctrl.label,
            enable_checkbox=ctrl.enableCheckbox,
            button=ctrl.button,
            enable_when=ctrl.enableWhen,
            allow_using_min_max=True,
            slider_min=ctrl.sliderMin,
            slider_max=ctrl.sliderMax,
            tr_context=tr_context,
        )
        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, fact_ref)
        if _v:
            qml = _inject_prop(qml, f"{indent}    visible: {_v}")
        return _apply_indent(qml)

    if control_type == "radiogroup":
        qml = render_radiogroup(
            fact_ref,
            indent,
            label=ctrl.label,
            options=ctrl.options,
            enable_when=ctrl.enableWhen,
            raw=ctrl.raw,
            optional=ctrl.optional,
            tr_context=tr_context,
        )
        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, fact_ref)
        if _v:
            inner_qml = render_radiogroup(
                fact_ref,
                indent + "    ",
                label=ctrl.label,
                options=ctrl.options,
                enable_when=ctrl.enableWhen,
                raw=ctrl.raw,
                optional=ctrl.optional,
                tr_context=tr_context,
            )
            qml = (
                f"{indent}ColumnLayout {{\n"
                f"{indent}    visible: {_v}\n"
                f"{indent}    spacing: 0\n"
                f"{inner_qml}\n"
                f"{indent}}}"
            )
        return _apply_indent(qml)

    if control_type == "bitmaskCheckbox":
        qml = render_bitmask_checkbox(
            fact_ref,
            indent,
            label=ctrl.label,
            bit_mask=ctrl.bitMask,
            enable_when=ctrl.enableWhen,
            tr_context=tr_context,
        )
        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, "fact")
        if _v:
            qml = _inject_prop(qml, f"{indent}    visible: {_v}")
        return _apply_indent(qml)

    if control_type == "bitmask":
        qml = render_bitmask(
            fact_ref,
            indent,
            first_entry_is_all=ctrl.firstEntryIsAll,
            enable_when=ctrl.enableWhen,
        )
        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, "fact")
        if _v:
            qml = _inject_prop(qml, f"{indent}    visible: {_v}")
        return _apply_indent(qml)

    if control_type == "toggleCheckbox" and ctrl.toggleCheckbox:
        qml = render_toggle_checkbox(
            indent,
            label=ctrl.label,
            toggle=ctrl.toggleCheckbox,
            enable_when=ctrl.enableWhen,
            optional=ctrl.optional,
            fact_ref=fact_ref,
            tr_context=tr_context,
        )
        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, fact_ref)
        if _v:
            qml = _inject_prop(qml, f"{indent}    visible: {_v}")
        return _apply_indent(qml)

    if control_type == "checkbox":
        qml = render_checkbox(
            fact_ref,
            indent,
            label=ctrl.label,
            enable_when=ctrl.enableWhen,
            label_source=f"{fact_ref}.shortDescription",
            qml_type="FactCheckBoxSlider",
            tr_context=tr_context,
        )
        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, "fact")
        if _v:
            qml = _inject_prop(qml, f"{indent}    visible: {_v}")
        return _apply_indent(qml)

    if control_type == "combobox":
        if ctrl.enumValues:
            qml = (
                _env.get_template("control_combo_enum.qml.j2")
                .render(
                    indent=indent,
                    fact_ref=fact_ref,
                    display_label=qml_tr(ctrl.label, tr_context)
                    if ctrl.label
                    else '_fact ? _fact.shortDescription : ""',
                    model_items=", ".join(
                        qml_tr(label, tr_context) for _, label in ctrl.enumValues
                    ),
                    values_list=", ".join(json.dumps(v) for v, _ in ctrl.enumValues),
                    enable_when=ctrl.enableWhen,
                    visible=_vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, "_fact"),
                )
                .rstrip("\n")
            )
            return _apply_indent(qml)
        qml = render_combobox(
            fact_ref,
            indent,
            label=ctrl.label,
            enable_when=ctrl.enableWhen,
            label_source=f"{fact_ref}.shortDescription",
            qml_type="LabelledFactComboBox",
            combo_preferred_width="ScreenTools.defaultFontPixelWidth * 30",
            tr_context=tr_context,
        )
        _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, "fact")
        if _v:
            qml = _inject_prop(qml, f"{indent}    visible: {_v}")
        return _apply_indent(qml)

    qml = render_textfield(
        fact_ref,
        indent,
        label=ctrl.label,
        enable_when=ctrl.enableWhen,
        label_source=f"{fact_ref}.shortDescription",
        qml_type="LabelledFactTextField",
        description=ctrl.description,
        tr_context=tr_context,
    )
    # When description is set, render_textfield wraps in a ColumnLayout which
    # has no `fact` property, so we must use the full fact_ref expression.
    _fact_for_vis = fact_ref if ctrl.description else "fact"
    _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, _fact_for_vis)
    if _v:
        qml = _inject_prop(qml, f"{indent}    visible: {_v}")
    return _apply_indent(qml)


def _inject_prop(qml: str, prop_line: str) -> str:
    """Insert a property line after the opening brace of a QML block."""
    qml_lines = qml.split("\n")
    qml_lines.insert(1, prop_line)
    return "\n".join(qml_lines)


def _section_visible(sec: SectionDef) -> str:
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

    name_vis = "sectionMatchesFilter(heading)"
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
            "title": title,
            "terms": ", ".join(f'"{t}"' for t in terms),
            "comma": "," if i < len(items) - 1 else "",
        }
        for i, (title, terms) in enumerate(items)
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
