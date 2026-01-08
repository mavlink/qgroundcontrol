#!/usr/bin/env python3
"""
FactGroup Generator CLI for QGroundControl

Usage with spec file (recommended):
    python -m tools.generators.factgroup.cli --spec wind.yaml --output src/Vehicle/FactGroups/
    python -m tools.generators.factgroup.cli --spec wind.yaml --dry-run

Usage with CLI arguments:
    python -m tools.generators.factgroup.cli --name Wind \\
        --facts "direction:double:deg,speed:double:m/s" \\
        --mavlink "WIND_COV,HIGH_LATENCY2" \\
        --output src/Vehicle/FactGroups/

Options:
    --spec FILE         Load specification from YAML or JSON file
    --name NAME         Domain name (e.g., Wind -> VehicleWindFactGroup)
    --facts FACTS       Comma-separated fact specs: "name:type:units,..."
    --mavlink MSGS      Comma-separated MAVLink message IDs (optional)
    --output DIR        Output directory (default: current directory)
    --update-rate MS    Update rate in milliseconds (default: 1000)
    --dry-run           Preview generated files without writing
    --validate          Only validate spec, don't generate
    --help              Show this help message
"""

import argparse
import sys
from pathlib import Path

from .generator import (
    FactGroupSpec,
    FactGroupGenerator,
    parse_facts_string,
    parse_mavlink_string,
    load_spec_from_file,
    validate_spec,
)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Generate QGC FactGroup boilerplate files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    # Spec file input (preferred)
    parser.add_argument(
        '--spec', '-s',
        type=Path,
        help='Load specification from YAML or JSON file',
    )

    # CLI argument input (alternative)
    parser.add_argument(
        '--name', '-n',
        help='Domain name (e.g., Wind -> VehicleWindFactGroup)',
    )
    parser.add_argument(
        '--facts', '-f',
        help='Fact specifications: "name:type:units,..." (e.g., "speed:double:m/s")',
    )
    parser.add_argument(
        '--mavlink', '-m',
        default='',
        help='MAVLink message IDs: "MSG1,MSG2,..." (optional)',
    )
    parser.add_argument(
        '--update-rate', '-r',
        type=int,
        default=1000,
        help='Update rate in milliseconds (default: 1000)',
    )

    # Output options
    parser.add_argument(
        '--output', '-o',
        type=Path,
        default=Path('.'),
        help='Output directory (default: current directory)',
    )
    parser.add_argument(
        '--dry-run', '-d',
        action='store_true',
        help='Preview generated files without writing',
    )
    parser.add_argument(
        '--validate',
        action='store_true',
        help='Only validate spec, do not generate files',
    )

    args = parser.parse_args()

    # Load spec from file or CLI arguments
    if args.spec:
        if not args.spec.exists():
            print(f"Error: Spec file not found: {args.spec}", file=sys.stderr)
            return 1
        try:
            spec = load_spec_from_file(args.spec)
        except Exception as e:
            print(f"Error loading spec file: {e}", file=sys.stderr)
            return 1
    elif args.name and args.facts:
        facts = parse_facts_string(args.facts)
        mavlink_messages = parse_mavlink_string(args.mavlink) if args.mavlink else []
        spec = FactGroupSpec(
            domain=args.name,
            facts=facts,
            mavlink_messages=mavlink_messages,
            update_rate_ms=args.update_rate,
        )
    else:
        print("Error: Either --spec or both --name and --facts are required", file=sys.stderr)
        parser.print_help()
        return 1

    # Validate spec
    errors = validate_spec(spec)
    if errors:
        print("Specification errors:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    if args.validate:
        print("Specification is valid.")
        print(f"  Domain: {spec.domain}")
        print(f"  Class: {spec.class_name}")
        print(f"  Facts: {len(spec.facts)}")
        print(f"  MAVLink messages: {len(spec.mavlink_messages)}")
        return 0

    # Generate files
    generator = FactGroupGenerator(spec, args.output)

    if args.dry_run:
        print("=" * 60)
        print("DRY RUN - Files that would be generated:")
        print("=" * 60)

    files = generator.generate_all(dry_run=args.dry_run)

    if args.dry_run:
        for filename, content in files.items():
            print(f"\n{'=' * 60}")
            print(f"FILE: {filename}")
            print("=" * 60)
            print(content)
    else:
        print(f"\nGenerated {len(files)} files in {args.output}")
        print("\nNext steps:")
        print(f"1. Add files to src/Vehicle/FactGroups/CMakeLists.txt")
        print(f"2. Add #include \"{spec.header_filename}\" to Vehicle.h")
        print(f"3. Add member: {spec.class_name} _{spec.factgroup_name}FactGroup;")
        print(f"4. Add initializer in Vehicle.cc constructor")
        print(f"5. Call _addFactGroup() in _commonInit()")

    return 0


if __name__ == '__main__':
    sys.exit(main())
