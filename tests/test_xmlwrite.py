"""Oráculo diferencial do serializador write-only (passo 111, decisão D58).

Monta a mesma árvore no shim (`qgc4qgis.core.xmlwrite`) e no
`xml.etree.ElementTree` de verdade — o import é permitido aqui porque `tests/`
não entra no ZIP do plugin — e exige **bytes idênticos**. Os casos cobrem tudo
que `kml.py` e `wpml.py` produzem: namespace default + prefixado (declarados na
raiz, em ordem de primeiro uso), texto com `&` e `<`, `indent`, elemento vazio
(self-closing) e a declaração XML (aspas simples + nova linha, como o ET).
"""

import xml.etree.ElementTree as ET

from qgc4qgis.core import xmlwrite

KML_NS = "http://www.opengis.net/kml/2.2"
WPML_NS = "http://www.dji.com/wpmz/1.0.2"


def _build_et() -> bytes:
    ET.register_namespace("", KML_NS)
    ET.register_namespace("wpml", WPML_NS)
    kml = ET.Element(f"{{{KML_NS}}}kml")
    doc = ET.SubElement(kml, f"{{{KML_NS}}}Document")
    author = ET.SubElement(doc, f"{{{WPML_NS}}}author")
    author.text = "Diego & <qgc4qgis>"
    create = ET.SubElement(doc, f"{{{WPML_NS}}}createTime")
    create.text = "2026-08-29T12:00:00"
    placemark = ET.SubElement(kml, f"{{{KML_NS}}}Placemark")
    coords = ET.SubElement(placemark, f"{{{KML_NS}}}coordinates")
    coords.text = "-46.6333,-23.5505,50.00"
    ET.SubElement(placemark, f"{{{KML_NS}}}altitudeMode")
    ET.indent(kml, space="  ")
    return ET.tostring(kml, encoding="utf-8", xml_declaration=True)


def _build_shim() -> bytes:
    xmlwrite.register_namespace("", KML_NS)
    xmlwrite.register_namespace("wpml", WPML_NS)
    kml = xmlwrite.Element(f"{{{KML_NS}}}kml")
    doc = xmlwrite.SubElement(kml, f"{{{KML_NS}}}Document")
    author = xmlwrite.SubElement(doc, f"{{{WPML_NS}}}author")
    author.text = "Diego & <qgc4qgis>"
    create = xmlwrite.SubElement(doc, f"{{{WPML_NS}}}createTime")
    create.text = "2026-08-29T12:00:00"
    placemark = xmlwrite.SubElement(kml, f"{{{KML_NS}}}Placemark")
    coords = xmlwrite.SubElement(placemark, f"{{{KML_NS}}}coordinates")
    coords.text = "-46.6333,-23.5505,50.00"
    xmlwrite.SubElement(placemark, f"{{{KML_NS}}}altitudeMode")
    xmlwrite.indent(kml, space="  ")
    return xmlwrite.tostring(kml, encoding="utf-8", xml_declaration=True)


def test_shim_serializa_bytes_identicos_ao_elementtree():
    """Árvore com dois namespaces + prefixo, texto com & e <, indent e vazio."""
    et_bytes = _build_et()
    shim_bytes = _build_shim()
    assert shim_bytes == et_bytes


def test_declaracao_xml_com_aspas_simples_e_newline():
    """A declaração sai igual à do ET: aspas simples e \\n antes da raiz."""
    shim_bytes = _build_shim()
    assert shim_bytes.startswith(b"<?xml version='1.0' encoding='utf-8'?>\n<kml ")


def test_kml_sem_indent_fica_em_linha_unica():
    """Sem indent a saída não tem quebras, como o route_to_litchi_kml do kml.py."""
    ET.register_namespace("", KML_NS)
    et_kml = ET.Element(f"{{{KML_NS}}}kml")
    et_doc = ET.SubElement(et_kml, f"{{{KML_NS}}}Document")
    et_name = ET.SubElement(et_doc, f"{{{KML_NS}}}name")
    et_name.text = "Missão & Teste"
    et_bytes = ET.tostring(et_kml, encoding="utf-8", xml_declaration=True)

    xmlwrite.register_namespace("", KML_NS)
    shim_kml = xmlwrite.Element(f"{{{KML_NS}}}kml")
    shim_doc = xmlwrite.SubElement(shim_kml, f"{{{KML_NS}}}Document")
    shim_name = xmlwrite.SubElement(shim_doc, f"{{{KML_NS}}}name")
    shim_name.text = "Missão & Teste"
    shim_bytes = xmlwrite.tostring(shim_kml, encoding="utf-8", xml_declaration=True)

    assert shim_bytes == et_bytes
    assert b"\n" not in shim_bytes[len(b"<?xml version='1.0' encoding='utf-8'?>\n"):]


def test_find_iteracao_e_tamanho_do_elemento():
    """Element expõe find/iter/len como os testes do wpml esperam."""
    xmlwrite.register_namespace("wpml", WPML_NS)
    mission = xmlwrite.Element(f"{{{WPML_NS}}}missionConfig")
    fly = xmlwrite.SubElement(mission, f"{{{WPML_NS}}}flyToWaylineMode")
    fly.text = "safely"
    drone = xmlwrite.SubElement(mission, f"{{{WPML_NS}}}droneInfo")
    xmlwrite.SubElement(drone, f"{{{WPML_NS}}}droneEnumValue").text = "68"

    assert len(mission) == 2
    assert [c.tag for c in mission] == [
        f"{{{WPML_NS}}}flyToWaylineMode",
        f"{{{WPML_NS}}}droneInfo",
    ]
    assert mission.find(f"{{{WPML_NS}}}flyToWaylineMode").text == "safely"
    enum_tag = f"{{{WPML_NS}}}droneEnumValue"
    assert mission.find(f"{{{WPML_NS}}}droneInfo/{enum_tag}").text == "68"
    assert mission.find(f"{{{WPML_NS}}}inexistente") is None


def test_register_namespace_automatico_ns0():
    """URI não registrada ganha prefixo ns0, como o xml.etree."""
    et_root = ET.Element("{urn:na}r")
    ET.SubElement(et_root, "{urn:na}c").text = "1"
    et_bytes = ET.tostring(et_root, encoding="utf-8", xml_declaration=True)

    shim_root = xmlwrite.Element("{urn:na}r")
    xmlwrite.SubElement(shim_root, "{urn:na}c").text = "1"
    shim_bytes = xmlwrite.tostring(shim_root, encoding="utf-8", xml_declaration=True)

    assert shim_bytes == et_bytes
