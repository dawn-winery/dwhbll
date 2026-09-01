from __future__ import annotations

import io
import sys
import zipfile
from pathlib import Path

from script.codegen import GeneratedFile, GeneratedNamespace

def parse_property(data: bytes, file: str, wanted_property: str) -> list[tuple[int, int]]:
    ranges = []

    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        with zf.open(file) as f:
            for raw_line in io.TextIOWrapper(f, encoding="utf-8"):
                line = raw_line.split("#", 1)[0].strip()

                if not line:
                    continue

                codepoints, property_name = map(
                    str.strip,
                    line.split(";", 1),
                )

                if property_name != wanted_property:
                    continue

                if ".." in codepoints:
                    first, last = codepoints.split("..")
                    start = int(first, 16)
                    end = int(last, 16)
                else:
                    start = end = int(codepoints, 16)

                ranges.append((start, end))

    return ranges


def parse_name_aliases(data: bytes) -> list[tuple[str, int]]:
    aliases = []

    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        with zf.open("NameAliases.txt") as f:
            for raw_line in io.TextIOWrapper(f, encoding="utf-8"):
                line = raw_line.split("#", 1)[0].strip()

                if not line:
                    continue

                codepoint, alias, _ = map(
                    str.strip,
                    line.split(";", 2),
                )

                aliases.append((alias, int(codepoint, 16)))

    return aliases


def parse_unicode_data(data: bytes) -> list[tuple[int, int]]:
    aliases = []

    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        with zf.open("UnicodeData.txt") as f:
            for raw_line in io.TextIOWrapper(f, encoding="utf-8"):
                line_data = raw_line.split(";", 14)
                codepoint = int(line_data[0], 16)
                ccc = int(line_data[3])

                aliases.append((codepoint, ccc))

    return aliases


def merge_ranges(ranges: list[tuple[int, int]]) -> list[tuple[int, int]]:
    if not ranges:
        return []

    ranges = sorted(ranges)

    merged = [ranges[0]]

    for start, end in ranges[1:]:
        prev_start, prev_end = merged[-1]

        if start <= prev_end + 1:
            merged[-1] = (prev_start, max(prev_end, end))
        else:
            merged.append((start, end))

    return merged

def merge_to_ranges(points: list[tuple[int, int]]) -> list[tuple[int, int, int]]:
    if not points:
        return []

    points = sorted(points)

    merged = [(points[0][0], points[0][0], points[0][1])]

    for val, data in points[1:]:
        prev_start, prev_end, prev_data = merged[-1]

        if val == prev_end + 1 and prev_data == data:
            merged[-1] = (prev_start, val, data)
        else:
            merged.append((val, val, data))

    return merged


def main() -> None:
    if len(sys.argv) != 3:
        print("Usage: generate.py <UCD.zip location> <output file>")

    zipf = Path(sys.argv[1]).resolve()
    sfile = Path(sys.argv[2]).resolve()

    with open(zipf, 'rb') as file:
        data = file.read()

    xid_start = merge_ranges(
        parse_property(data, "DerivedCoreProperties.txt", "XID_Start")
    )

    xid_continue = merge_ranges(
        parse_property(data, "DerivedCoreProperties.txt", "XID_Continue")
    )

    id_copmpat_math_start = merge_ranges(
        parse_property(data, "PropList.txt", "ID_Compat_Math_Start")
    )

    id_copmpat_math_continue = merge_ranges(
        parse_property(data, "PropList.txt", "ID_Compat_Math_Continue")
    )

    unicode_data = merge_to_ranges(parse_unicode_data(data))

    name_aliases = parse_name_aliases(data)

    print(f"XID_Start:     {len(xid_start):,} ranges")
    print(f"XID_Continue:  {len(xid_continue):,} ranges")
    print(f"ID_Compat_Math_Start:     {len(id_copmpat_math_start):,} ranges")
    print(f"ID_Compat_Math_Continue:  {len(id_copmpat_math_continue):,} ranges")
    print(f"NameAliases.txt: {len(name_aliases):,} aliases")
    print(f"UnicodeData.txt (Canonical Combining Class): {len(name_aliases):,} ranges")

    properties = {
        "XID_Start": xid_start,
        "XID_Continue": xid_continue,
        "ID_Compat_Math_Start": id_copmpat_math_start,
        "ID_Compat_Math_Continue": id_copmpat_math_continue
    }

    with GeneratedFile("dwhbll/unicode/table.h", sfile) as f:
        f.add_include("array")
        f.add_include("cstdint")

        with GeneratedNamespace(f, "dwhbll::unicode") as unicode_ns:
            with GeneratedNamespace(unicode_ns, "properties") as props:
                for name, body in properties.items():
                    lines = [
                        f"static table<empty_struct>::elem {name}_ranges[] = {{",
                    ]

                    for start, end in body:
                        lines.append(
                            f"    {{0x{start:06X}, 0x{end:06X}}},"
                        )

                    lines.append("};")
                    lines.append("")
                    lines.append(f"table<empty_struct> {name} {{{name}_ranges, {name}_ranges + std::size({name}_ranges)}};")

                    props.append_lines(lines)

            with GeneratedNamespace(unicode_ns, "aliases") as aliases:
                aliases.append_lines(["std::unordered_map<std::string, char32_t> name_aliases_to_codepoint {"])

                aliases.append_lines([f"    {{\"{name}\", 0x{codepoint:06X}}},\n" for name, codepoint in name_aliases])

                aliases.append_lines(["};"])

            with GeneratedNamespace(unicode_ns, "base") as base:
                base.append_lines(["static table<int>::elem canonical_combining_class_ranges[] = {"])

                base.append_lines([f"    {{0x{start:06X}, 0x{end:06X}, {value}}}," for start, end, value in unicode_data])

                base.append_lines([
                    "};",
                    "",
                    "table<int> canonical_combining_class {canonical_combining_class_ranges, canonical_combining_class_ranges + std::size(canonical_combining_class_ranges)};"
                ])

    print(f"Generated {sfile}")


if __name__ == "__main__":
    main()
