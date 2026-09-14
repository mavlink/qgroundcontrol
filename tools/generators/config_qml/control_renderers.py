"""Control-specific rendering; page and section layout lives in emit.py."""

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
from .model import ControlDef, _vis_expr

_env = make_env(Path(__file__).parent / "templates")


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


def _inject_prop(qml: str, prop_line: str) -> str:
    """Insert a property line after the opening brace of a QML block."""
    qml_lines = qml.split("\n")
    qml_lines.insert(1, prop_line)
    return "\n".join(qml_lines)


def _render_component(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
    qml = (
        _env.get_template("control_component.qml.j2")
        .render(
            indent=indent,
            component=ctrl.component,
            show_when=ctrl.showWhen,
        )
        .rstrip("\n")
    )
    return qml


def _render_label(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
    qml = render_label(
        indent,
        text=ctrl.label,
        warning=ctrl.warning,
        small_font=ctrl.smallFont,
        tr_context=tr_context,
    )
    if ctrl.showWhen:
        qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
    return qml


def _render_dialog_button(
    ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str, dialog_counter: list[int] | None
) -> str:
    if ctrl.dialogButton is None:
        raise ValueError("dialogButton requires dialogButton")
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
    return qml


def _render_action_button(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
    if ctrl.actionButton is None:
        raise ValueError("actionButton requires actionButton")
    qml = render_action_button(
        indent,
        action_button=ctrl.actionButton,
        enable_when=ctrl.enableWhen,
        tr_context=tr_context,
    )
    if ctrl.showWhen:
        qml = _inject_prop(qml, f"{indent}    visible: {ctrl.showWhen}")
    return qml


def _render_factslider(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
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
    return qml


def _render_slider(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
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
    return qml


def _render_radiogroup(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
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
    return qml


def _render_bitmask_checkbox(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
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
    return qml


def _render_bitmask(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
    qml = render_bitmask(
        fact_ref,
        indent,
        first_entry_is_all=ctrl.firstEntryIsAll,
        enable_when=ctrl.enableWhen,
    )
    _v = _vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, "fact")
    if _v:
        qml = _inject_prop(qml, f"{indent}    visible: {_v}")
    return qml


def _render_toggle_checkbox(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
    if ctrl.toggleCheckbox is None:
        raise ValueError("toggleCheckbox requires toggleCheckbox")
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
    return qml


def _render_checkbox(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
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
    return qml


def _render_combobox(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:
    if ctrl.enumValues:
        qml = (
            _env.get_template("control_combo_enum.qml.j2")
            .render(
                indent=indent,
                fact_ref=fact_ref,
                display_label=qml_tr(ctrl.label, tr_context)
                if ctrl.label
                else '_fact ? _fact.shortDescription : ""',
                model_items=", ".join(qml_tr(label, tr_context) for _, label in ctrl.enumValues),
                values_list=", ".join(json.dumps(v) for v, _ in ctrl.enumValues),
                enable_when=ctrl.enableWhen,
                visible=_vis_expr(ctrl.showWhen, ctrl.optional, ctrl.param, "_fact"),
            )
            .rstrip("\n")
        )
        return qml
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
    return qml


def _render_textfield(ctrl: ControlDef, indent: str, fact_ref: str, tr_context: str) -> str:

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
    return qml


_RENDERERS = {
    "component": _render_component,
    "label": _render_label,
    "actionButton": _render_action_button,
    "factslider": _render_factslider,
    "slider": _render_slider,
    "radiogroup": _render_radiogroup,
    "bitmaskCheckbox": _render_bitmask_checkbox,
    "bitmask": _render_bitmask,
    "toggleCheckbox": _render_toggle_checkbox,
    "checkbox": _render_checkbox,
    "combobox": _render_combobox,
}


def render_control(
    ctrl: ControlDef,
    indent: str,
    *,
    indexed: bool = False,
    dialog_counter: list[int] | None = None,
    tr_context: str = "",
) -> str:
    fact_ref = _fact_ref(ctrl, indexed=indexed)
    kind = ctrl.control or "textfield"
    if kind == "dialogButton" and ctrl.dialogButton:
        qml = _render_dialog_button(ctrl, indent, fact_ref, tr_context, dialog_counter)
    else:
        missing_fragment = (
            (kind == "component" and not ctrl.component)
            or (kind == "actionButton" and not ctrl.actionButton)
            or (kind == "toggleCheckbox" and not ctrl.toggleCheckbox)
        )
        renderer = (
            _render_textfield if missing_fragment else _RENDERERS.get(kind, _render_textfield)
        )
        qml = renderer(ctrl, indent, fact_ref, tr_context)
    if ctrl.indent:
        qml = _inject_prop(
            qml, f"{indent}    Layout.leftMargin: ScreenTools.defaultFontPixelWidth * 2"
        )
    return qml
