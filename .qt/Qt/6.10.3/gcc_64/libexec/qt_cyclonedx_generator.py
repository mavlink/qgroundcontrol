# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import importlib.util
import logging
import sys


def setup_log() -> logging.Logger:
    logging.basicConfig(format="[%(levelname)s]: %(message)s", level=logging.INFO)
    log_object = logging.getLogger("qt-cyclonedx-generator")
    log_object.setLevel(logging.INFO)
    return log_object


log = setup_log()


def import_tomllib():
    """Import the TOML library with fallback options for different Python versions.

    Stock toml lib is available on Python 3.11+,
    if not found try the tomli library, and if that isn't found
    try to use the pip vendored tomli library.

    Returns:
        Tuple of (tomllib_module, success_flag)
    """
    if sys.version_info >= (3, 11):
        import tomllib as tomllib_local

        return tomllib_local, True
    else:
        try:
            import tomli as tomllib_local

            return tomllib_local, True
        except ModuleNotFoundError:
            try:  # noinspection PyProtectedMember
                import pip._vendor.tomli as tomllib_local

                return tomllib_local, True
            except ModuleNotFoundError:
                return None, False


tomllib, tomllib_available = import_tomllib()


def check_package_dependencies() -> None:
    """Check if required package dependencies are available."""
    missing_packages = []

    if not tomllib_available:
        missing_packages.append("tomli")

    if importlib.util.find_spec("cyclonedx") is None:
        missing_packages.append("cyclonedx")

    if importlib.util.find_spec("packageurl") is None:
        missing_packages.append("packageurl")

    if missing_packages:
        log.error(f"Missing required dependencies: {', '.join(missing_packages)}")
        log.error(f"Please install with: `pip install {' '.join(missing_packages)}`")
        sys.exit(1)


check_package_dependencies()

import argparse
from datetime import datetime, timezone
from license_expression import get_spdx_licensing
from pathlib import Path
from packageurl import PackageURL
from typing import (
    TYPE_CHECKING,
    Optional,
    Any,
    TypedDict,
    Union,
    cast,
    Callable,
)
from uuid import UUID

from cyclonedx.factory.license import LicenseFactory
from cyclonedx.exception import MissingOptionalDependencyException
from cyclonedx.model import (
    XsUri,
    ExternalReference,
    ExternalReferenceType,
    Property,
    AttachedText,
)
from cyclonedx.model.bom import Bom
from cyclonedx.model.component import Component, ComponentType
from cyclonedx.model.contact import OrganizationalEntity, OrganizationalContact
from cyclonedx.model.license import LicenseAcknowledgement, LicenseExpression
from cyclonedx.model.lifecycle import PredefinedLifecycle, LifecyclePhase
from cyclonedx.output.json import JsonV1Dot6
from cyclonedx.schema import SchemaVersion
from cyclonedx.validation.json import JsonStrictValidator

if TYPE_CHECKING:
    from cyclonedx.output.json import Json as JsonOutputter


class TomlRequiredFieldError(Exception):
    pass


class MissingTomlFileError(Exception):
    pass


class SBOMWriteFailedError(Exception):
    pass


class ValidationFailedError(Exception):
    pass


class ValidationDependencyMissingError(Exception):
    pass


# TypedDict definitions for TOML data structures
class CyclonePropertyDict(TypedDict):
    name: str
    value: str


class LicenseDict(TypedDict):
    license_id: str
    text: str


class RelationshipDict(TypedDict):
    # Optional fields
    relationship_from: str
    relationship_type: str
    relationship_to: str
    relationship_comment: str

class RootComponentDict(TypedDict):
    name: str
    spdx_id: str
    # Optional fields (since TypedDict doesn't support NotRequired in Python 3.9)
    # We'll use .get() to access these safely
    version: str
    description: str
    download_location: str
    build_date: str
    supplier: str
    supplier_url: str
    serial_number_uuid: str
    relationships: list[RelationshipDict]


class ComponentDict(TypedDict):
    name: str
    spdx_id: str
    # Optional fields
    version: str
    component_type: str
    copyright: str
    external_bom_link: str
    supplier: str
    purl_list: list[str]
    cpe_list: list[str]
    properties: list[CyclonePropertyDict]
    relationships: list[RelationshipDict]


class BuildToolDict(TypedDict):
    # Optional fields
    name: str
    component_type: str
    version: str
    description: str


class TomlDataDict(TypedDict):
    root_component: RootComponentDict
    # Optional fields
    project_build_tools: list[BuildToolDict]
    components: list[ComponentDict]
    licenses: list[LicenseDict]


# Type alias for licenses dictionary
LicensesDict = dict[str, LicenseDict]


def validate_required_field(
    data: Union[dict[str, Any], TomlDataDict, ComponentDict, RootComponentDict],
    field: str,
    context: str,
) -> Any:
    """Validate that a required field exists in the data dictionary.

    Args:
        data: The dictionary to check
        field: The required field name
        context: Description of where this validation is happening (for error messages)

    Returns:
        The value of the required field

    Raises:
        TomlRequiredFieldError: If the required field is missing
    """
    if field not in data:
        raise TomlRequiredFieldError(f"Missing required field '{field}' in {context}")
    return data[field]


def _parse_build_date(date_str: str) -> datetime:
    """Parse build date string, handling Python 3.9+ compatibility for Z suffix."""
    override_to_utc = False
    if sys.version_info < (3, 11) and date_str.endswith("Z"):
        # Remove the Z at the end to support Python 3.9+, because
        # datetime.fromisoformat learned to parse the Z in 3.11 only.
        # In that case we need to override the timezone to be UTC manually.
        date_str = date_str[:-1]
        override_to_utc = True

    build_date = datetime.fromisoformat(date_str)
    if override_to_utc:
        build_date = build_date.replace(tzinfo=timezone.utc)
    return build_date


def assign_optional_field(
    source_data: Union[
        dict[str, Any], TomlDataDict, ComponentDict, RootComponentDict, BuildToolDict
    ],
    target_args: dict[str, Any],
    source_field: str,
    target_field: Optional[str] = None,
    transform: Optional[Callable[[Any], Any]] = None,
) -> None:
    """Assign an optional field from source data to target args if it exists.

    Args:
        source_data: The source dictionary to read from
        target_args: The target dictionary to write to
        source_field: The field name in the source data
        target_field: The field name in the target args (defaults to source_field if None)
        transform: Optional function to transform the value before assignment
    """
    if target_field is None:
        target_field = source_field

    if value := source_data.get(source_field):
        if transform:
            value = transform(value)
        # Only assign if the transformed value is not None
        if value is not None:
            target_args[target_field] = value


def main() -> None:
    parser = get_cli_parser()
    args = get_cli_args(parser)

    handle_verbose_logging(args.verbose)

    try:
        create_cyclone_dx_sbom(
            args.input_path,
            args.output_path,
            args.validate_json,
            args.validate_json_required,
        )
    except (
        TomlRequiredFieldError,
        MissingTomlFileError,
        SBOMWriteFailedError,
        ValidationFailedError,
        ValidationDependencyMissingError,
    ) as e:
        log.error(str(e))
        sys.exit(1)


def get_cli_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description="""
Generates a CycloneDX v1.6 SBOM using intermediate information from a toml file created by Qt's
SBOM generator.

Example usage:
python qt_cyclonedx_generator.py --input-path /qt/qtbase.cdx.toml --output-path /qt/qtbase.cdx.json
""",
    )
    parser.add_argument(
        "-i",
        "--input-path",
        type=Path,
        help="Path to the intermediate toml file.",
        required=True,
    )
    parser.add_argument(
        "-o",
        "--output-path",
        type=Path,
        help="Path to the CycloneDX output file.",
        required=True,
    )
    parser.add_argument(
        "--validate-json",
        action="store_true",
        help="Validate the generated CycloneDX JSON output against the CycloneDX schema.",
    )
    parser.add_argument(
        "--validate-json-required",
        action="store_true",
        help="Error out if validation dependencies are missing.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose logging output.",
    )

    return parser


def get_cli_args(parser: argparse.ArgumentParser) -> argparse.Namespace:
    args = parser.parse_args(args=None if sys.argv[1:] else ["--help"])
    return args


def handle_verbose_logging(verbose: bool) -> None:
    if verbose:
        log.setLevel(logging.DEBUG)


# Reads a toml file and generates a CycloneDX SBOM file.
def create_cyclone_dx_sbom(
    input_path: Path,
    output_path: Path,
    validate_json=False,
    validate_json_required=False,
) -> None:
    if not input_path or not input_path.exists():
        raise MissingTomlFileError(
            f"Failed to read toml file. Input file '{input_path}' does not exist."
        )

    cydx_bom = get_cydx_bom()
    toml_data = read_toml_file(input_path)
    process_toml(toml_data, cydx_bom)
    json = write_cydx_sbom(cydx_bom, output_path)
    if validate_json:
        validate_cydx_bom(json, validate_json_required)


def read_toml_file(input_path: Path) -> TomlDataDict:
    log.debug(f"Reading toml file from '{input_path}'")
    with open(input_path, "rb") as f:
        # Manually read and decode the file contents, because older
        # tomli versions vendored in pip don't handle this properly.
        buffer = f.read()
        toml_str = buffer.decode()
        return cast(TomlDataDict, cast(Any, tomllib).loads(toml_str))


# Serializes the CycloneDX BOM to JSON and writes it to the output path.
def write_cydx_sbom(cydx_bom: Bom, output_path: Path) -> str:
    json_outputter: JsonOutputter = JsonV1Dot6(cydx_bom)
    log.debug(f"Serializing CycloneDX BOM to JSON at '{output_path}'")
    serialized_json = json_outputter.output_as_string(indent=2)

    try:
        with open(output_path, "w") as f:
            f.write(serialized_json)
    except (OSError, IOError) as e:
        raise SBOMWriteFailedError(f"Failed to write SBOM to '{output_path}': {e}")

    return serialized_json


def validate_cydx_bom(serialized_json: str, validate_json_required: bool) -> None:
    log.debug("Validating CycloneDX JSON output against schema v1.6")
    json_validator = JsonStrictValidator(SchemaVersion.V1_6)
    try:
        validation_errors = json_validator.validate_str(serialized_json)
        if validation_errors:
            raise ValidationFailedError(f"JSON validation failed: {validation_errors}")
    except MissingOptionalDependencyException as error:
        missing_dep_message = (
            f"JSON-validation was failed due to missing Python dependency: {error}"
        )
        if validate_json_required:
            raise ValidationDependencyMissingError(missing_dep_message)
        else:
            log.warning(missing_dep_message)


def get_cydx_bom() -> Bom:
    bom = Bom()
    return bom


# Reads toml data created by Qt's SBOM generator and populates the CycloneDX BOM.
# At minimum, expects a root component dict, and a list of components.
def process_toml(toml_data: TomlDataDict, cydx_bom: Bom) -> None:
    components: dict[str, Component] = {}

    log.debug("Processing TOML data.")

    root_component_object, main_supplier, project_spdx_id = handle_root_component(
        cydx_bom, toml_data
    )
    components[project_spdx_id] = root_component_object

    if "project_build_tools" in toml_data:
        project_build_tools = toml_data["project_build_tools"]
        handle_project_build_tools(cydx_bom, project_build_tools)

    external_components_spdx_ids = set()
    components_toml = []
    license_factory = LicenseFactory()

    # Create one component per CMake target (in spdx it would be a spdx package).
    if "components" in toml_data:
        components_toml = toml_data["components"]

        components_len = len(components_toml)
        log.debug(f"Processing {components_len} components from TOML data.")

        licenses_dict: LicensesDict = {}
        if "licenses" in toml_data:
            licenses_toml = toml_data["licenses"]
            for license_entry in licenses_toml:
                license_id = license_entry["license_id"]
                licenses_dict[license_id] = license_entry

        for component_toml in components_toml:
            process_component(
                cydx_bom,
                component_toml,
                licenses_dict,
                components,
                external_components_spdx_ids,
                main_supplier,
                license_factory,
            )

    # Register dependencies for each component.
    log.debug("Processing component dependencies.")
    for component_toml in components_toml:
        handle_component_dependencies(
            cydx_bom,
            component_toml,
            components,
            external_components_spdx_ids,
            root_component_object,
        )

    log.debug("Processing project relationships.")
    handle_project_relationships(
        cydx_bom,
        toml_data,
        components,
    )


# Creates the root component of the BOM.
# The CycloneDX root component is mapped to the Qt spdx project package e.g. qtbase.
def handle_root_component(
    cydx_bom: Bom, toml_data: TomlDataDict
) -> tuple[Component, Optional[OrganizationalEntity], str]:
    root_components_args: dict[str, Any] = {
        "type": ComponentType.FRAMEWORK,
    }

    root_component = validate_required_field(toml_data, "root_component", "TOML data")
    error_context = "root_component"
    project_name = validate_required_field(root_component, "name", error_context)
    root_components_args["name"] = project_name

    project_spdx_id = validate_required_field(root_component, "spdx_id", error_context)
    root_components_args["bom_ref"] = project_spdx_id

    # Process optional fields
    assign_optional_field(root_component, root_components_args, "version")
    assign_optional_field(root_component, root_components_args, "description")

    # Process optional fields with transformations
    assign_optional_field(
        root_component,
        root_components_args,
        source_field="download_location",
        target_field="external_references",
        transform=lambda url: [
            ExternalReference(
                type=ExternalReferenceType.DISTRIBUTION,  # pyright: ignore [reportCallIssue]
                url=XsUri(url),  # pyright: ignore [reportCallIssue]
            )
        ],
    )

    if project_build_date_str := root_component.get("build_date"):
        project_build_date = _parse_build_date(project_build_date_str)
        cydx_bom.metadata.timestamp = project_build_date  # pyright: ignore [reportAttributeAccessIssue]

    if serial_number_uuid := root_component.get("serial_number_uuid"):
        cydx_bom.serial_number = UUID(serial_number_uuid)  # pyright: ignore [reportAttributeAccessIssue]

    project_supplier = root_component.get("supplier")
    project_supplier_url = root_component.get("supplier_url")

    # TODO: This is currently not supplied by the build system API.
    project_supplier_email = root_component.get("supplier_email")

    main_supplier = create_organization_entity(project_supplier, project_supplier_url)
    main_author = create_organization_contact(project_supplier, project_supplier_email)

    if main_supplier:
        cydx_bom.metadata.manufacturer = main_supplier  # pyright: ignore [reportAttributeAccessIssue]
        cydx_bom.metadata.supplier = main_supplier  # pyright: ignore [reportAttributeAccessIssue]

        # FOSSA complains if author is not set.
        cydx_bom.metadata.authors = [main_author]  # pyright: ignore [reportAttributeAccessIssue]

        root_components_args["supplier"] = main_supplier
        root_components_args["manufacturer"] = main_supplier

    root_component_object = Component(**root_components_args)
    cydx_bom.metadata.component = root_component_object  # pyright: ignore [reportAttributeAccessIssue]

    cydx_bom.metadata.lifecycles = [PredefinedLifecycle(phase=LifecyclePhase.BUILD)]  # pyright: ignore [reportAttributeAccessIssue, reportCallIssue]

    root_component_name_str = f"{root_component_object.name}"  # pyright: ignore [reportAttributeAccessIssue]
    root_component_version_str = f"(version {root_component_object.version})"  # pyright: ignore [reportAttributeAccessIssue]
    log.debug(
        f"Created root component of the BOM. "
        f"{root_component_name_str} ${root_component_version_str}"
    )

    return root_component_object, main_supplier, project_spdx_id


# Handles project build tools and adds them as components to the BOM metadata tools section.
# This is for tools like CMake, Ninja, etc.
# Unclear yet if compilers and linkers should also go here.
def handle_project_build_tools(
    cydx_bom: Bom, project_build_tools_toml: list[BuildToolDict]
) -> None:
    log.debug("Processing project build tools.")

    for build_tool_toml in project_build_tools_toml:
        component_args = {}

        assign_optional_field(build_tool_toml, component_args, source_field="name")
        assign_optional_field(
            build_tool_toml,
            component_args,
            source_field="component_type",
            target_field="type",
            transform=get_component_type_for_str,
        )
        assign_optional_field(build_tool_toml, component_args, "version")
        assign_optional_field(build_tool_toml, component_args, "description")

        component = Component(**component_args)
        log.debug(
            f"Created build tool component '{component.name} (version {component.version})'."  # pyright: ignore [reportAttributeAccessIssue]
        )
        cydx_bom.metadata.tools.components.add(component)  # pyright: ignore [reportAttributeAccessIssue]


# Creates a CycloneDX component from the toml component data and adds it to the BOM.
# The SPDX equivalent would be a SPDX package backed by a CMake target.
# We mostly reuse spdx ids as bom-refs, rather than generating new ones. Makes it
# easier to cross-check with the original SPDX data. And it's not like CycloneDX forbids it.
def process_component(
    cydx_bom: Bom,
    component_toml: ComponentDict,
    licenses_dict: LicensesDict,
    components: dict[str, Component],
    external_components_spdx_ids: set[str],
    main_supplier: Optional[OrganizationalEntity],
    license_factory: LicenseFactory,
) -> None:
    component_args = {}
    properties = []

    error_context = "component"
    component_name = validate_required_field(component_toml, "name", error_context)
    component_args["name"] = component_name

    assign_optional_field(
        component_toml,
        component_args,
        "version",
        transform=lambda v: v if v != "unknown" else None,
    )

    # A spdx id maps to a bom-ref in CycloneDX.
    spdx_id = validate_required_field(component_toml, "spdx_id", error_context)
    component_args["bom_ref"] = spdx_id

    # Some components might be external to the current BOM.
    # For example for the QtSvg module, QtCore is external, because it's built in qtbase,
    # not qtsvg. To be able to declare a dependency on QtCore, we still need to add it as a
    # component. But we add a BOM external reference link to it, signalling that the full
    # component information is in a different BOM.
    assign_optional_field(
        component_toml,
        component_args,
        source_field="external_bom_link",
        target_field="external_references",
        transform=lambda link: [
            ExternalReference(
                type=ExternalReferenceType.BOM,  # pyright: ignore [reportCallIssue]
                url=XsUri(link),  # pyright: ignore [reportCallIssue]
            )
        ],
    )
    if component_toml.get("external_bom_link"):
        external_components_spdx_ids.add(spdx_id)

    assign_optional_field(
        component_toml,
        component_args,
        source_field="component_type",
        target_field="type",
        transform=get_component_type_for_str,
    )

    assign_optional_field(
        component_toml,
        component_args,
        source_field="copyright",
        target_field="copyright",
    )

    handle_component_licenses(
        component_toml, licenses_dict, license_factory, component_args
    )

    purl_list = component_toml.get("purl_list", [])
    cpe_list = component_toml.get("cpe_list", [])
    cpe_purl_args, extra_cpe_purl_properties = get_purl_and_cpe_component_args(
        purl_list, cpe_list
    )
    if extra_cpe_purl_properties:
        properties.extend(extra_cpe_purl_properties)

    if supplier_name := component_toml.get("supplier"):
        # If the specified supplier is the same as the main supplier, reuse it, because it has a
        # url.
        if main_supplier and supplier_name == main_supplier.name:  # pyright: ignore [reportAttributeAccessIssue]
            component_args["supplier"] = main_supplier
        else:
            component_supplier = create_organization_entity(supplier_name)
            component_args["supplier"] = component_supplier

    # Properties are extra vendor-specific information that are not part of the official spec.
    if properties_list := component_toml.get("properties"):
        for prop in properties_list:
            properties.append(
                Property(
                    name=prop["name"],  # pyright: ignore [reportCallIssue]
                    value=prop["value"],  # pyright: ignore [reportCallIssue]
                )
            )
    if properties:
        component_args["properties"] = properties

    component = Component(**component_args, **cpe_purl_args)

    log.debug(f"Created component '{component.name} (version {component.version})'.")  # pyright: ignore [reportAttributeAccessIssue]

    cydx_bom.components.add(component)  # pyright: ignore [reportAttributeAccessIssue]
    components[spdx_id] = component


# Creates dependencies between components in the BOM, as well as between the root component
# and non-external components that are part of the current BOM.
def handle_component_dependencies(
    cydx_bom: Bom,
    component_toml: ComponentDict,
    components: dict[str, Component],
    external_components_spdx_ids: set[str],
    root_component_object: Component,
) -> None:
    component_id = component_toml["spdx_id"]
    component_object = components.get(component_id)

    # If the component is not external, register it as a dependency of the root component,
    # aka the root component "contains" the non-external component.
    if component_id not in external_components_spdx_ids:
        cydx_bom.register_dependency(root_component_object, [component_object])  # pyright: ignore [reportAttributeAccessIssue, reportArgumentType]

    # Register relationships declared in the toml.
    if relationships := component_toml.get("relationships"):
        process_relationships(relationships, cydx_bom, components)


def handle_project_relationships(
    cydx_bom: Bom,
    toml_data: TomlDataDict,
    components: dict[str, Component],
) -> None:
    root_component_toml = validate_required_field(toml_data, "root_component", "TOML data")
    if relationships := root_component_toml.get("relationships"):
        process_relationships(relationships, cydx_bom, components)


# Creates dependencies between components based on the relationships declared in the toml data.
# CycloneDX does not support different relationship types, like SPDX does, so the relationship
# type is ignored.
def process_relationships(relationships: list[RelationshipDict], cydx_bom: Bom, components: dict[str, Component]) -> None:
    for relationship in relationships:
        from_spdx_id = relationship["relationship_from"]
        to_spdx_id = relationship["relationship_to"]
        relationship_type = relationship["relationship_type"]
        if from_spdx_id not in components:
            log.warning(
                f"Component '{from_spdx_id}' in 'from' dependency is unknown, skipping."
            )
            continue
        if to_spdx_id not in components:
            log.warning(
                f"Component {to_spdx_id} in 'from' dependency is unknown, skipping."
            )
            continue
        from_component = components[from_spdx_id]
        to_component = components[to_spdx_id]
        cydx_bom.register_dependency(from_component,
                                     [to_component])  # pyright: ignore [reportAttributeAccessIssue, reportArgumentType]
        log.debug(f"Created dependency from '{from_spdx_id}' to '{to_spdx_id}'. The relationship type '{relationship_type}' is currently discarded.")


# Handles PURL and CPE entries for a component.
# The spec says that a component can have exactly one PURL. Qt generates several, because it's
# unclear what will end up being useful for tooling. The other PURLs are added as extra
# component properties. Same for CPEs.
# The field names were picked to match the names generated by the python spdx to cyclonedx converter.
def get_purl_and_cpe_component_args(
    purl_list: list[str],
    cpe_list: list[str],
) -> tuple[dict[str, Any], list[Property]]:
    component_args = {}
    properties = []

    purl_objects = [PackageURL.from_string(purl) for purl in purl_list if purl]

    if len(purl_objects) > 0:
        # First PURL will be the main one.
        purl = purl_objects.pop(0)
        component_args["purl"] = purl

    # Rest will be properties.
    for purl_entry in purl_objects:
        properties.append(
            Property(
                name="spdx:external-reference:package-manager:purl",  # pyright: ignore [reportCallIssue]
                value=str(purl_entry),  # pyright: ignore [reportCallIssue]
            )
        )

    if len(cpe_list) > 0:
        # First CPE will be the main one.
        cpe = cpe_list.pop(0)
        component_args["cpe"] = cpe

    # Rest will be properties.
    for cpe_entry in cpe_list:
        properties.append(
            Property(
                name="spdx:external-reference:security:cpe23",  # pyright: ignore [reportCallIssue]
                value=str(cpe_entry),  # pyright: ignore [reportCallIssue]
            )
        )

    return component_args, properties


# Handles licenses for a component.
def handle_component_licenses(
    component_toml: ComponentDict,
    licenses_dict: LicensesDict,
    license_factory: LicenseFactory,
    component_args: dict[str, Any],
) -> None:
    def transformer(license_expression: str) -> Optional[list[LicenseExpression]]:
        acknowledgement = LicenseAcknowledgement.CONCLUDED
        return convert_license_expression_license_objects(
            license_expression, licenses_dict, license_factory, acknowledgement
        )

    assign_optional_field(
        component_toml,
        component_args,
        source_field="license_concluded_expression",
        target_field="licenses",
        transform=transformer,
    )


# Transforms a spdx license expression into CycloneDX license objects.
def convert_license_expression_license_objects(
    license_expression: str,
    licenses_dict: LicensesDict,
    license_factory: LicenseFactory,
    acknowledgement: LicenseAcknowledgement,
) -> Optional[list[LicenseExpression]]:
    if "LicenseRef-" in license_expression:
        return handle_custom_license_ref_expression(
            license_expression, licenses_dict, license_factory, acknowledgement
        )
    else:
        return handle_standard_license_ref_expression(
            license_expression, license_factory, acknowledgement
        )


# CycloneDX v1.6 doesn't support an expression that contains LicenseRef- in it.
# CycloneDX v1.7 seemingly does.
# https://github.com/CycloneDX/specification/issues/454
# https://github.com/CycloneDX/specification/issues/554
# https://github.com/CycloneDX/specification/pull/582
#
# Split LicenseRef-* expressions into separate licenses, by which we lose the expression structure,
# but at least preserve the license information.
#
# TODO: Improve this when CycloneDX v1.7 generation is added.
def handle_custom_license_ref_expression(
    license_expression: str,
    licenses_dict: LicensesDict,
    license_factory: LicenseFactory,
    acknowledgement: LicenseAcknowledgement,
) -> list[LicenseExpression]:
    license_objects = []
    licensing = get_spdx_licensing()
    symbols = licensing.license_symbols(license_expression)

    for symbol in symbols:
        license_text = None
        # This can be either a known public spdx id, in which case we won't have text for.
        # Or it can be a LicenseRef-* for which we have license text in the licenses_dict.
        maybe_license_id = symbol.key  # pyright: ignore [reportAttributeAccessIssue]

        if maybe_license_id in licenses_dict:
            # Get the text for the LicenseRef-* entry.
            maybe_license_text = licenses_dict[maybe_license_id].get("text")
            license_text = AttachedText(content=maybe_license_text)  # pyright: ignore [reportCallIssue]

        license_obj = license_factory.make_from_string(
            maybe_license_id,
            license_text=license_text,
            license_acknowledgement=acknowledgement,
        )
        license_objects.append(license_obj)

    return license_objects


# Transforms a spdx license expression without LicenseRef-* into a CycloneDX license object.
def handle_standard_license_ref_expression(
    license_expression: str,
    license_factory: LicenseFactory,
    acknowledgement: LicenseAcknowledgement,
) -> list[LicenseExpression]:
    license_obj = license_factory.make_with_expression(
        expression=license_expression, acknowledgement=acknowledgement
    )
    return [license_obj]


# Transform from enum string value to actual enum.
def get_component_type_for_str(component_type_str: str) -> ComponentType:
    return ComponentType(component_type_str)


# Creates an OrganizationalEntity for a supplier/manufacturer.
def create_organization_entity(
    supplier_name: Optional[str] = None, supplier_url: Optional[str] = None
) -> Optional[OrganizationalEntity]:
    args = {}
    if supplier_name:
        args["name"] = supplier_name
    if supplier_url:
        args["urls"] = [XsUri(supplier_url)]  # pyright: ignore [reportCallIssue]

    # Check if args has at least one entry.
    if len(args) > 0:
        return OrganizationalEntity(**args)

    return None


# Creates an OrganizationalContact for a supplier/manufacturer.
def create_organization_contact(
    contact_name: Optional[str] = None, contact_email: Optional[str] = None
) -> Optional[OrganizationalContact]:
    args = {}
    if contact_name:
        args["name"] = contact_name
    if contact_email:
        args["email"] = contact_email  # pyright: ignore [reportCallIssue]

    # Check if args has at least one entry.
    if len(args) > 0:
        return OrganizationalContact(**args)

    return None


if __name__ == "__main__":
    main()
