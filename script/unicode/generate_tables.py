from __future__ import annotations

import io
import sys
import zipfile
from pathlib import Path
from collections import deque

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


def parse_decomposition(data: str) -> tuple[str, list[int]]:
    if not data:
        return "", []

    compat_name = ""

    if data.startswith("<"):
        # compatibility mapping
        decomp_head = data.find("> ")
        compat_name = data[1:decomp_head]
        data = data[decomp_head + 2:]

        if not compat_name:
            raise Exception("Compat name is empty!")

    return compat_name, [int(x, 16) for x in data.split(" ")]


def parse_composition_exclusions(data: str) -> set[int]:
    aliases = set()

    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        with zf.open("CompositionExclusions.txt") as f:
            for raw_line in io.TextIOWrapper(f, encoding="utf-8"):
                line = raw_line.split("#", 1)[0].strip()

                if not line:
                    continue

                aliases.add(int(line, 16))

    return aliases


def parse_semicolon_separated(data: str, file: str) -> list:
    res = []

    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        with zf.open(file) as f:
            for raw_line in io.TextIOWrapper(f, encoding="utf-8"):
                line = raw_line.split("#", 1)[0].strip()

                if not line:
                    continue

                res.append([x.strip() for x in line.split(";")])

    return res


def parse_unicode_data(data: bytes) -> list[tuple[int, int, str, str, int, str, tuple[str, list[int]], str, str, str, bool, str, int, int, int]]:
    aliases = []

    range = False

    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        with zf.open("UnicodeData.txt") as f:
            for raw_line in io.TextIOWrapper(f, encoding="utf-8"):
                line_data = raw_line.split(";", 14)

                if line_data[9] != "Y" and line_data[9] != "N":
                    raise Exception("Bidi mirrored is not Y or N!")

                if range:
                    range = False
                    if len(aliases) > 0:
                        prev = aliases[-1]
                        aliases[-1] = (
                            prev[0],
                            int(line_data[0], 16),
                            prev[2][1:-8],
                            prev[3],
                            prev[4],
                            prev[5],
                            prev[6],
                            prev[7],
                            prev[8],
                            prev[9],
                            prev[10],
                            prev[11],
                            prev[12],
                            prev[13],
                            prev[14]
                        )
                    else:
                        raise Exception("Range end without any previous data!")

                aliases.append((
                    int(line_data[0], 16),
                    int(line_data[0], 16),
                    line_data[1],
                    line_data[2],
                    int(line_data[3]),
                    line_data[4],
                    parse_decomposition(line_data[5]),
                    line_data[6],
                    line_data[7],
                    line_data[8],
                    True if line_data[9] == "Y" else False,
                    line_data[10],
                    line_data[12],
                    line_data[13],
                    line_data[14]
                ))

                if line_data[1].startswith('<') and line_data[1].endswith(', First>'):
                    range = True

    return aliases


def parse_codepoint(data: str) -> tuple[int, int]:
    data = data.strip()

    if ".." in data:
        first, last = data.split("..")
        return int(first, 16), int(last, 16)
    else:
        point = int(data, 16)
        return point, point



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

def merge_to_ranges(points: list[tuple[int, int, int]]) -> list[tuple[int, int, int]]:
    if not points:
        return []

    points = sorted(points)

    merged = [(points[0][0], points[0][0], points[0][1])]

    for start, end, data in points[1:]:
        prev_start, prev_end, prev_data = merged[-1]

        if start <= prev_end + 1 and prev_data == data:
            merged[-1] = (prev_start, end, data)
        else:
            merged.append((start, end, data))

    return merged


def generate_compositions(comp_exclusions: set[int], data: list[tuple[int, int, str, str, int, str, tuple[str, list[int]], str, str, str, bool, str, int, int, int]]) -> tuple[list[tuple[int, str, list[int]]], list[tuple[int, str, list[int]]], dict[int, dict[int, int]]]:
    decompositions = {}
    compat_decompositions = {}
    ccc = {}

    for entry in data:
        ccc[entry[0]] = entry[4]

        if not entry[6][1]:
            continue

        if entry[0] != entry[1]:
            raise Exception("Decompositions make no sense for ranges!")

        if entry[0] in decompositions:
            raise Exception(f"Multiple decompositions found for point 0x{entry[0]:06x}")

        compat_decompositions[entry[0]] = entry[6]
        if not entry[6][0]:
            decompositions[entry[0]] = entry[6]

    compositions = {}

    for key in decompositions:
        value = decompositions[key]
        if len(value[1]) != 2:
            # Composition only applies to pairs
            continue

        if len(value[0]) != 0:
            # Compatibility entries do not apply for composition
            continue

        if key in comp_exclusions:
            # excluded from composition
            continue

        # Non-starter decompositions do not apply
        if value[1][0] in ccc and ccc[value[1][0]] != 0:
            continue
        if key in ccc and ccc[key] != 0:
            continue

        if value[1][0] in compositions and value[1][1] in compositions[value[1][0]]:
            raise Exception(f"Already saw one composition for points 0x{value[1][0]:06x} 0x{value[1][1]:06x}")

        if value[1][0] not in compositions:
            compositions[value[1][0]] = {}

        compositions[value[1][0]][value[1][1]] = key

    for entry in decompositions:
        value = decompositions[entry]
        expanded = []
        points = deque(value[1])

        while len(points) > 0:
            head = points.popleft()

            if head in decompositions:
                compat, decomp = decompositions[head]
                points.extendleft(decomp[::-1])
            else:
                expanded.append(head)

        decompositions[entry] = (value[0], expanded)

    for entry in compat_decompositions:
        value = compat_decompositions[entry]
        expanded = []
        points = deque(value[1])

        while len(points) > 0:
            head = points.popleft()

            if head in compat_decompositions:
                compat, decomp = compat_decompositions[head]
                points.extendleft(decomp[::-1])
            else:
                expanded.append(head)

        compat_decompositions[entry] = (value[0], expanded)

    return [(key, *value) for key, value in decompositions.items()], [(key, *value) for key, value in compat_decompositions.items()], compositions


def cpp_table(body_type: str, name: str, entries_fmt: str, entries: list[tuple]) -> list[str]:
    last_key = 0

    lines = [f"static table<{body_type}>::elem {name}_ranges[] = {{"]

    for entry in entries:
        if last_key > entry[0]:
            raise Exception(f"Expected entry[0]({entry[0]}) > last_key({last_key})")

        last_key = entry[0]

    fmtstr = f"    {entries_fmt}, "
    lines.extend([fmtstr.format(*entry) for entry in entries])

    lines.extend([
        "};",
        "",
        f"table<{body_type}> {name} {{{name}_ranges, {name}_ranges + std::size({name}_ranges)}};"
    ])

    return lines


def main() -> None:
    if len(sys.argv) != 3:
        print("Usage: generate.py <UCD.zip location> <output file>")

    zipf = Path(sys.argv[1]).resolve()
    sfile = Path(sys.argv[2]).resolve()

    with open(zipf, 'rb') as file:
        data = file.read()

    derived_core_properties = parse_semicolon_separated(data, "DerivedCoreProperties.txt")
    prop_list = parse_semicolon_separated(data, "PropList.txt")

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

    parsed_unicode_data = parse_unicode_data(data)

    parsed_comp_exclusions = parse_composition_exclusions(data)

    canonical_combining_class = merge_to_ranges([(x[0], x[1], x[4]) for x in parsed_unicode_data])

    decompositions, compat_decompositions, compositions = generate_compositions(parsed_comp_exclusions, parsed_unicode_data)

    name_aliases = parse_name_aliases(data)

    derived_normalization_props = parse_semicolon_separated(data, "DerivedNormalizationProps.txt")

    print(f"XID_Start:     {len(xid_start):,} ranges")
    print(f"XID_Continue:  {len(xid_continue):,} ranges")
    print(f"ID_Compat_Math_Start:     {len(id_copmpat_math_start):,} ranges")
    print(f"ID_Compat_Math_Continue:  {len(id_copmpat_math_continue):,} ranges")
    print(f"NameAliases.txt: {len(name_aliases):,} aliases")
    print(f"UnicodeData.txt (Canonical Combining Class): {len(name_aliases):,} ranges")
    print(f"Compositions: {len(compositions)} entries")
    print(f"Composition Exclusions: {len(parsed_comp_exclusions)} entries")
    print(f"Decompositions: {len(decompositions)} entries")
    print(f"Compat Decomposition: {len(compat_decompositions)} entries")

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
                    props.append_lines(cpp_table("empty_struct", name, "{{0x{:06X}, 0x{:06X}}}", body))

            with GeneratedNamespace(unicode_ns, "aliases") as aliases:
                aliases.append_lines(["std::unordered_map<std::u32string, char32_t> name_aliases_to_codepoint {"])
                aliases.append_lines([f"    {{U\"{name}\", 0x{codepoint:06X}}}," for name, codepoint in name_aliases])
                aliases.append_lines(["};"])

            with GeneratedNamespace(unicode_ns, "base") as base:
                base.append_lines(cpp_table("int", "canonical_combining_class", "{{0x{:06X}, 0x{:06X}, {}}}", canonical_combining_class))
                base.append_lines(cpp_table("decomposition", "decomposition_table", "{{0x{:06X}, 0x{:06X}, {{{}}}}}", sorted([
                    (val, val, ', '.join([f"0x{decomp:06X}" for decomp in decomposed]))
                    for val, compat, decomposed in decompositions
                    if not compat
                ])))

                base.append_lines(cpp_table("decomposition", "compat_decomposition_table", "{{0x{:06X}, 0x{:06X}, {{{}}}}}", sorted([
                    (val, val, ', '.join([f"0x{decomp:06X}" for decomp in decomposed]))
                    for val, compat, decomposed in compat_decompositions
                ])))

                for key in compositions:
                    lines = [f"static composition composition_u{key:06x}_ranges[] = {{"]
                    lines.extend([
                        f"    {{0x{val:06X}, 0x{composed:06X}}}," for val, composed in sorted(list(compositions[key].items()))
                    ])
                    lines.append("};")
                    lines.append("")

                    base.append_lines(lines)

                base.append_lines(cpp_table("compositions", "composition_table", "{{0x{:06X}, 0x{:06X}, {}}}", sorted([
                    (key, key, f"{{composition_u{key:06x}_ranges, composition_u{key:06x}_ranges + std::size(composition_u{key:06x}_ranges)}}") for key in compositions
                ])))

            with GeneratedNamespace(unicode_ns, "normalization") as norm:
                # TODO: handle range merging
                for type in ["NFC_QC", "NFD_QC", "NFKC_QC", "NFKD_QC"]:
                    norm.append_lines(cpp_table("QC_VAL", type.lower(), "{{0x{:06X}, 0x{:06X}, {}}}", sorted([
                        (*parse_codepoint(data[0]), "QC_VAL::NO" if data[2] == "N" else ("QC_VAL::MAYBE" if data[2] == "M" else "???")) for
                        data in derived_normalization_props if data[1] == type
                    ])))

    print(f"Generated {sfile}")


if __name__ == "__main__":
    main()
