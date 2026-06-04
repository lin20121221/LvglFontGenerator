#include "gpos_reader.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>

namespace lvgl {

// Query-based approach: Mimic Node.js opentype.js behavior
// This queries kerning for each character pair, just like Node.js does

bool GPOSReader::extract_kerning(FT_Face face,
                                  const std::vector<uint32_t>& codes,
                                  std::map<uint32_t, std::map<uint32_t, int8_t>>& kerning_map) {

    // Parse GPOS table
    ParsedGPOS gpos;
    if (!parse_gpos_table(face, gpos)) {
        return false;
    }

    fprintf(stderr, "Using query-based kerning extraction (like Node.js)\n");

    // Build code to glyph_index map
    std::map<uint32_t, FT_UInt> code_to_gid;
    for (uint32_t code : codes) {
        FT_UInt gid = FT_Get_Char_Index(face, code);
        if (gid > 0) {
            code_to_gid[code] = gid;
        }
    }

    // Query kerning for all character pairs (like Node.js does)
    int pair_count = 0;
    int non_zero_count = 0;
    int raw_non_zero = 0;
    int scaled_to_zero = 0;

    for (const auto& entry1 : code_to_gid) {
        uint32_t code1 = entry1.first;
        FT_UInt gid1 = entry1.second;

        for (const auto& entry2 : code_to_gid) {
            uint32_t code2 = entry2.first;
            FT_UInt gid2 = entry2.second;

            pair_count++;

            // Query GPOS for this pair
            int16_t kern_value = query_kerning_value(gpos, gid1, gid2);

            if (kern_value != 0) {
                raw_non_zero++;

                // Convert from font units to pixels, then to FP4.4 format
                // FP4.4 format: multiply by 16 to preserve 4 bits of fraction
                double scaled = (double)kern_value * gpos.pixel_size / gpos.units_per_em;
                int8_t kern_fp44 = static_cast<int8_t>(round(scaled * 16.0));

                if (kern_fp44 != 0) {
                    kerning_map[code1][code2] = kern_fp44;
                    non_zero_count++;
                } else {
                    scaled_to_zero++;
                }
            }
        }
    }

    fprintf(stderr, "Queried %d pairs, found %d raw non-zero, %d scaled to zero, %d final non-zero\n",
            pair_count, raw_non_zero, scaled_to_zero, non_zero_count);

    return non_zero_count > 0;
}

bool GPOSReader::parse_gpos_table(FT_Face face, ParsedGPOS& gpos) {
    // Load GPOS table
    FT_ULong gpos_size = 0;
    FT_Error error = FT_Load_Sfnt_Table(face, FT_MAKE_TAG('G','P','O','S'), 0, nullptr, &gpos_size);

    if (error || gpos_size == 0) {
        fprintf(stderr, "GPOS table not found\n");
        return false;
    }

    gpos.data.resize(gpos_size);
    error = FT_Load_Sfnt_Table(face, FT_MAKE_TAG('G','P','O','S'), 0, gpos.data.data(), &gpos_size);

    if (error) {
        fprintf(stderr, "Failed to load GPOS table\n");
        return false;
    }

    const uint8_t* data = gpos.data.data();

    // Parse header
    uint16_t major_version = read_uint16(data);
    if (major_version != 1) {
        fprintf(stderr, "Unsupported GPOS version %d\n", major_version);
        return false;
    }

    gpos.lookup_list_offset = read_uint16(data + 8);
    const uint8_t* lookup_list = data + gpos.lookup_list_offset;
    gpos.lookup_count = read_uint16(lookup_list);

    gpos.units_per_em = face->units_per_EM;
    gpos.pixel_size = face->size->metrics.y_ppem;

    // Pre-parse all Pair Adjustment (Type 2) subtables for fast queries
    for (uint16_t i = 0; i < gpos.lookup_count; i++) {
        uint16_t lookup_offset = read_uint16(lookup_list + 2 + i * 2);
        const uint8_t* lookup = data + gpos.lookup_list_offset + lookup_offset;

        uint16_t lookup_type = read_uint16(lookup);
        if (lookup_type != 2) continue; // Only Type 2 (Pair Adjustment)

        uint16_t subtable_count = read_uint16(lookup + 4);

        // Parse all subtables
        for (uint16_t j = 0; j < subtable_count; j++) {
            uint16_t subtable_offset = read_uint16(lookup + 6 + j * 2);
            const uint8_t* subtable = data + gpos.lookup_list_offset + lookup_offset + subtable_offset;

            uint16_t pos_format = read_uint16(subtable);

            if (pos_format == 1) {
                PairPosFormat1Data format1_data;

                uint16_t coverage_offset = read_uint16(subtable + 2);
                format1_data.value_format1 = read_uint16(subtable + 4);
                format1_data.value_format2 = read_uint16(subtable + 6);
                uint16_t pair_set_count = read_uint16(subtable + 8);

                // Parse coverage
                const uint8_t* coverage_table = subtable + coverage_offset;
                size_t remaining = gpos.data.size() - (coverage_table - data);
                parse_coverage(coverage_table, remaining, format1_data.coverage);

                format1_data.subtable_base = subtable;
                format1_data.pair_set_offsets.resize(pair_set_count);
                for (uint16_t k = 0; k < pair_set_count; k++) {
                    format1_data.pair_set_offsets[k] = read_uint16(subtable + 10 + k * 2);
                }

                gpos.format1_subtables.push_back(std::move(format1_data));

            } else if (pos_format == 2) {
                PairPosFormat2Data format2_data;

                uint16_t coverage_offset = read_uint16(subtable + 2);
                format2_data.value_format1 = read_uint16(subtable + 4);
                format2_data.value_format2 = read_uint16(subtable + 6);
                uint16_t class_def1_offset = read_uint16(subtable + 8);
                uint16_t class_def2_offset = read_uint16(subtable + 10);
                format2_data.class1_count = read_uint16(subtable + 12);
                format2_data.class2_count = read_uint16(subtable + 14);

                // Parse coverage
                const uint8_t* coverage_table = subtable + coverage_offset;
                size_t remaining = gpos.data.size() - (coverage_table - data);
                parse_coverage(coverage_table, remaining, format2_data.coverage);

                // Parse ClassDef tables
                const uint8_t* classdef1_table = subtable + class_def1_offset;
                remaining = gpos.data.size() - (classdef1_table - data);
                parse_classdef(classdef1_table, remaining, format2_data.class_def1);

                const uint8_t* classdef2_table = subtable + class_def2_offset;
                remaining = gpos.data.size() - (classdef2_table - data);
                parse_classdef(classdef2_table, remaining, format2_data.class_def2);

                size_t value1_size = get_value_record_size(format2_data.value_format1);
                size_t value2_size = get_value_record_size(format2_data.value_format2);
                format2_data.class_record_size = value1_size + value2_size;
                format2_data.class_records = subtable + 16;

                gpos.format2_subtables.push_back(std::move(format2_data));
            }
        }
    }

    fprintf(stderr, "GPOS table loaded: %lu bytes, %u lookups, %zu format1 subtables, %zu format2 subtables\n",
            gpos_size, gpos.lookup_count, gpos.format1_subtables.size(), gpos.format2_subtables.size());
    fprintf(stderr, "Font units_per_EM: %d, pixel_size: %d\n", gpos.units_per_em, gpos.pixel_size);

    return true;
}

int16_t GPOSReader::query_kerning_value(const ParsedGPOS& gpos, FT_UInt left_gid, FT_UInt right_gid) {
    // Query pre-parsed Format 1 subtables
    for (const auto& format1_data : gpos.format1_subtables) {
        int16_t value = query_pair_pos_format1(format1_data, left_gid, right_gid);
        if (value != 0) {
            return value; // Return first non-zero value found
        }
    }

    // Query pre-parsed Format 2 subtables
    for (const auto& format2_data : gpos.format2_subtables) {
        int16_t value = query_pair_pos_format2(format2_data, left_gid, right_gid);
        if (value != 0) {
            return value; // Return first non-zero value found
        }
    }

    return 0;
}

int16_t GPOSReader::query_pair_pos_format1(const PairPosFormat1Data& data,
                                           FT_UInt left_gid, FT_UInt right_gid) {
    // Check if left glyph is in coverage
    if (!data.coverage.contains(left_gid)) {
        return 0;
    }

    // Find coverage index
    int coverage_index = data.coverage.get_coverage_index(left_gid);
    if (coverage_index < 0 || coverage_index >= (int)data.pair_set_offsets.size()) {
        return 0;
    }

    uint16_t pair_set_offset = data.pair_set_offsets[coverage_index];
    if (pair_set_offset == 0) return 0;

    const uint8_t* pair_set = data.subtable_base + pair_set_offset;
    uint16_t pair_value_count = read_uint16(pair_set);

    size_t value1_size = get_value_record_size(data.value_format1);
    size_t value2_size = get_value_record_size(data.value_format2);
    size_t pair_value_size = 2 + value1_size + value2_size;

    // Search for right glyph
    for (uint16_t i = 0; i < pair_value_count; i++) {
        const uint8_t* pair_value = pair_set + 2 + i * pair_value_size;
        uint16_t second_gid = read_uint16(pair_value);

        if (second_gid == right_gid) {
            const uint8_t* value_ptr = pair_value + 2;
            ValueRecord value1 = parse_value_record(value_ptr, data.value_format1);
            return value1.x_advance;
        }
    }

    return 0;
}

int16_t GPOSReader::query_pair_pos_format2(const PairPosFormat2Data& data,
                                           FT_UInt left_gid, FT_UInt right_gid) {
    // Check if left glyph is in coverage
    if (!data.coverage.contains(left_gid)) {
        return 0;
    }

    // Get classes
    uint16_t class1 = data.class_def1.get_class(left_gid);
    uint16_t class2 = data.class_def2.get_class(right_gid);

    if (class1 >= data.class1_count || class2 >= data.class2_count) {
        return 0;
    }

    // Calculate offset in class records
    size_t record_offset = (class1 * data.class2_count + class2) * data.class_record_size;

    const uint8_t* record = data.class_records + record_offset;
    const uint8_t* value_ptr = record;
    ValueRecord value1 = parse_value_record(value_ptr, data.value_format1);

    return value1.x_advance;
}

// Keep all the helper functions from the original implementation
bool GPOSReader::parse_coverage(const uint8_t* data, size_t max_size, Coverage& coverage) {
    if (max_size < 4) return false;

    uint16_t coverage_format = read_uint16(data);

    if (coverage_format == 1) {
        uint16_t glyph_count = read_uint16(data + 2);
        if (4 + glyph_count * 2 > max_size) return false;

        for (uint16_t i = 0; i < glyph_count; i++) {
            uint16_t glyph_id = read_uint16(data + 4 + i * 2);
            coverage.glyphs.insert(glyph_id);
        }
        return true;

    } else if (coverage_format == 2) {
        uint16_t range_count = read_uint16(data + 2);
        if (4 + range_count * 6 > max_size) return false;

        for (uint16_t i = 0; i < range_count; i++) {
            const uint8_t* range_rec = data + 4 + i * 6;
            uint16_t start_glyph = read_uint16(range_rec);
            uint16_t end_glyph = read_uint16(range_rec + 2);

            for (uint16_t g = start_glyph; g <= end_glyph; g++) {
                coverage.glyphs.insert(g);
            }
        }
        return true;
    }

    return false;
}

bool GPOSReader::parse_classdef(const uint8_t* data, size_t max_size, ClassDef& classdef) {
    if (max_size < 4) return false;

    uint16_t class_format = read_uint16(data);

    if (class_format == 1) {
        uint16_t start_glyph = read_uint16(data + 2);
        uint16_t glyph_count = read_uint16(data + 4);
        if (6 + glyph_count * 2 > max_size) return false;

        for (uint16_t i = 0; i < glyph_count; i++) {
            uint16_t class_id = read_uint16(data + 6 + i * 2);
            classdef.glyph_to_class[start_glyph + i] = class_id;
        }
        return true;

    } else if (class_format == 2) {
        uint16_t range_count = read_uint16(data + 2);
        if (4 + range_count * 6 > max_size) return false;

        for (uint16_t i = 0; i < range_count; i++) {
            const uint8_t* range_rec = data + 4 + i * 6;
            uint16_t start_glyph = read_uint16(range_rec);
            uint16_t end_glyph = read_uint16(range_rec + 2);
            uint16_t class_id = read_uint16(range_rec + 4);

            for (uint16_t g = start_glyph; g <= end_glyph; g++) {
                classdef.glyph_to_class[g] = class_id;
            }
        }
        return true;
    }

    return false;
}

size_t GPOSReader::get_value_record_size(uint16_t value_format) {
    size_t size = 0;
    if (value_format & 0x0001) size += 2;
    if (value_format & 0x0002) size += 2;
    if (value_format & 0x0004) size += 2;
    if (value_format & 0x0008) size += 2;
    if (value_format & 0x0010) size += 2;
    if (value_format & 0x0020) size += 2;
    if (value_format & 0x0040) size += 2;
    if (value_format & 0x0080) size += 2;
    return size;
}

ValueRecord GPOSReader::parse_value_record(const uint8_t*& data, uint16_t value_format) {
    ValueRecord record;

    if (value_format & 0x0001) {
        record.x_placement = read_int16(data);
        data += 2;
    }
    if (value_format & 0x0002) {
        record.y_placement = read_int16(data);
        data += 2;
    }
    if (value_format & 0x0004) {
        record.x_advance = read_int16(data);
        data += 2;
    }
    if (value_format & 0x0008) {
        record.y_advance = read_int16(data);
        data += 2;
    }
    // Skip device table offsets
    if (value_format & 0x0010) data += 2;
    if (value_format & 0x0020) data += 2;
    if (value_format & 0x0040) data += 2;
    if (value_format & 0x0080) data += 2;

    return record;
}

} // namespace lvgl
