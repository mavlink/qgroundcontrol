"""Serializador XML write-only para o qgc4qgis (decisão D58 do plano 0.6.3).

Substitui o ``xml.etree.ElementTree`` em ``kml.py`` e ``wpml.py`` — que só
**geram** XML, nunca fazem parse — com a mesma superfície de escrita que eles
usam: ``register_namespace``, ``Element``, ``SubElement``, ``.text``,
``.append``, ``indent`` e ``tostring``. Sem parser, a classe de vulnerabilidade
que o B405 existe para pegar (XXE, billion laughs) é estruturalmente impossível.

A saída é byte-idêntica à do ``xml.etree`` para as árvores que o plugin produz;
``tests/test_xmlwrite.py`` é o oráculo diferencial (em ``tests/`` o import do
``xml.etree`` é permitido, pois não entra no ZIP do plugin).
"""

from __future__ import annotations

__all__ = ["Element", "SubElement", "indent", "register_namespace", "tostring"]

# uri -> prefixo; "" significa namespace default (xmlns=...).
_NAMESPACE_MAP: dict[str, str] = {}


def register_namespace(prefix: str, uri: str) -> None:
    """Registrar o prefixo usado para a URI na serialização.

    :param prefix: Prefixo ("" para o namespace default).
    :param uri: URI do namespace.
    """
    _NAMESPACE_MAP[uri] = prefix


def _escape(text: str) -> str:
    """Escapar os caracteres especiais de texto XML (&, <, >).

    :param text: Texto a escapar.
    :return: Texto escapado.
    """
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def _split_path(path: str) -> list[str]:
    """Dividir um caminho de tags em segmentos, ignorando ``/`` dentro de ``{uri}``.

    :param path: Caminho de tags (ex.: ``{uri}a/{uri}b``).
    :return: Lista de segmentos.
    """
    partes: list[str] = []
    atual: list[str] = []
    profundidade = 0
    for caractere in path.lstrip("./"):
        if caractere == "{":
            profundidade += 1
        elif caractere == "}":
            profundidade -= 1
        if caractere == "/" and profundidade == 0:
            partes.append("".join(atual))
            atual = []
        else:
            atual.append(caractere)
    partes.append("".join(atual))
    return partes


class Element:
    """Elemento XML write-only, compatível com a superfície do xml.etree usada."""

    def __init__(self, tag: str) -> None:
        self.tag = tag
        self.text: str | None = None
        self.tail: str | None = None
        self._children: list[Element] = []

    def append(self, child: Element) -> None:
        """Anexar um elemento filho.

        :param child: Elemento a anexar.
        """
        self._children.append(child)

    def find(self, path: str) -> Element | None:
        """Retornar o primeiro descendente que casa com o caminho.

        :param path: Caminho de tags.
        :return: Primeiro elemento que casa ou None.
        """
        matches = self.findall(path)
        return matches[0] if matches else None

    def findall(self, path: str) -> list[Element]:
        """Retornar todos os descendentes que casam com o caminho.

        :param path: Caminho de tags.
        :return: Lista de elementos que casam (vazia se nenhum).
        """
        partes = _split_path(path)

        def _match_all(elem: Element, tags: list[str]) -> list[Element]:
            wanted, rest = tags[0], tags[1:]
            results: list[Element] = []
            for child in elem:
                if child.tag == wanted:
                    if not rest:
                        results.append(child)
                    else:
                        results.extend(_match_all(child, rest))
            return results

        return _match_all(self, partes)

    def __iter__(self):
        return iter(self._children)

    def __len__(self) -> int:
        return len(self._children)

    def __getitem__(self, index: int) -> Element:
        return self._children[index]


def SubElement(parent: Element, tag: str) -> Element:
    """Criar um elemento filho de ``parent`` e retorná-lo.

    :param parent: Elemento pai.
    :param tag: Tag do filho, no formato ``{uri}local``.
    :return: O elemento criado.
    """
    child = Element(tag)
    parent.append(child)
    return child


def indent(elem: Element, space: str = "  ", level: int = 0) -> None:
    """Endentar a árvore em-place, com a mesma semântica do ``ET.indent``.

    :param elem: Elemento raiz da subárvore.
    :param space: String de endentação por nível.
    :param level: Nível inicial (0 para a raiz).
    """
    if level < 0:
        raise ValueError(f"Level must be non-negative, got {level}")
    if not len(elem):
        return

    indentations = ["\n" + level * space]

    def _indent_children(elem: Element, level: int) -> None:
        child_level = level + 1
        try:
            child_indentation = indentations[child_level]
        except IndexError:
            child_indentation = indentations[level] + space
            indentations.append(child_indentation)

        if not elem.text or not elem.text.strip():
            elem.text = child_indentation

        for child in elem:
            if len(child):
                _indent_children(child, child_level)
            if not child.tail or not child.tail.strip():
                child.tail = child_indentation

        if not child.tail.strip():
            child.tail = indentations[level]

    _indent_children(elem, 0)


def _collect_uris(elem: Element, uris: list[str]) -> None:
    tag = elem.tag
    if tag.startswith("{"):
        uri = tag[1:].partition("}")[0]
        if uri and uri not in uris:
            uris.append(uri)
    for child in elem:
        _collect_uris(child, uris)


def _prefixes_for(uris: list[str]) -> dict[str, str]:
    prefixes: dict[str, str] = {}
    auto = 0
    for uri in uris:
        prefix = _NAMESPACE_MAP.get(uri)
        if prefix is None:
            prefix = f"ns{auto}"
            auto += 1
        prefixes[uri] = prefix
    return prefixes


def _qname(tag: str, prefixes: dict[str, str]) -> str:
    if tag.startswith("{"):
        uri, _, local = tag[1:].partition("}")
        prefix = prefixes[uri]
        return f"{prefix}:{local}" if prefix else local
    return tag


def tostring(elem: Element, encoding: str = "utf-8", xml_declaration: bool = True) -> bytes:
    """Serializar a árvore em bytes, com a mesma saída do ``ET.tostring``.

    :param elem: Elemento raiz.
    :param encoding: Codificação (declarada e usada nos bytes de saída).
    :param xml_declaration: Se True, anteceder a declaração XML.
    :return: Documento serializado em bytes.
    """
    parts: list[str] = []
    if xml_declaration:
        parts.append(f"<?xml version='1.0' encoding='{encoding}'?>\n")

    uris: list[str] = []
    _collect_uris(elem, uris)
    prefixes = _prefixes_for(uris)
    declarations = "".join(
        f' xmlns="{uri}"' if not prefixes[uri] else f' xmlns:{prefixes[uri]}="{uri}"'
        for uri in uris
    )

    def _write(node: Element, node_declarations: str) -> None:
        # Elemento sem texto nem filhos vira self-closing (<tag />), como o xml.etree.
        if not node.text and not len(node):
            parts.append(f"<{_qname(node.tag, prefixes)}{node_declarations} />")
            return
        parts.append(f"<{_qname(node.tag, prefixes)}{node_declarations}>")
        if node.text:
            parts.append(_escape(node.text))
        for child in node:
            _write(child, "")
            if child.tail:
                parts.append(_escape(child.tail))
        parts.append(f"</{_qname(node.tag, prefixes)}>")

    _write(elem, declarations)

    return "".join(parts).encode(encoding)
