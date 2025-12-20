#!/usr/bin/env python3
"""
FactGroup Generator CLI for QGroundControl

Usage:
    python -m tools.generators.factgroup.cli --name Wind \\
        --facts "direction:double:deg,speed:double:m/s" \\
        --mavlink "WIND_COV,HIGH_LATENCY2" \\
        --output src/Vehicle/FactGroups/

    python -m tools.generators.factgroup.cli --name Example \\
        --facts "temp:double:degC,pressure:double:Pa" \\
        --dry-run

Options:
    --name NAME         Domain name (e.g., Wind -> VehicleWindFactGroup)
    --facts FACTS       Comma-separated fact specs: "name:type:units,..."
    --mavlink MSGS      Comma-separated MAVLink message IDs (optional)
    --output DIR        Output directory (default: current directory)
    --dry-run           Preview generated files without writing
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
)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Generate QGC FactGroup boilerplate files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        '--name', '-n',
        required=True,
        help='Domain name (e.g., Wind -> VehicleWindFactGroup)',
    )
    parser.add_argument(
        '--facts', '-f',
        required=True,
        help='Fact specifications: "name:type:units,..." (e.g., "speed:double:m/s")',
    )
    parser.add_argument(
        '--mavlink', '-m',
        default='',
        help='MAVLink message IDs: "MSG1,MSG2,..." (optional)',
    )
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
        '--update-rate', '-r',
        type=int,
        default=1000,
        help='Update rate in milliseconds (default: 1000)',
    )

    args = parser.parse_args()

    # Parse specifications
    facts = parse_facts_string(args.facts)
    mavlink_messages = parse_mavlink_string(args.mavlink) if args.mavlink else []

    if not facts:
        print("Error: At least one fact is required", file=sys.stderr)
        return 1

    # Create spec
    spec = FactGroupSpec(
        domain=args.name,
        facts=facts,
        mavlink_messages=mavlink_messages,
        update_rate_ms=args.update_rate,
    )

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
