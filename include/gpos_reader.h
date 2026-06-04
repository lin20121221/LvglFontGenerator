#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <map>
#include <set>
#include <cstdint>

namespace lvgl {

// Coverage table
struct Coverage {
    std::set<uint16_t> glyphs;  // Set of glyph IDs covered

    // For Coverage Format 1: list of glyph IDs
    // For Coverage Format 2: ranges
    bool contains(uint16_t glyph_id) const {
        return glyphs.find(glyph_id) != glyphs.end();
    }

    int get_coverage_index(uint16_t glyph_id) const {
        if (!contains(glyph_id)) return -1;
        int index = 0;
        for (uint16_t g : glyphs) {
            if (g == glyph_id) return index;
            index++;
        }
        return -1;
    }
};

// Class Definition table
struct ClassDef {
    std::map<uint16_t, uint16_t> glyph_to_class;  // glyph_id -> class_id

    uint16_t get_class(uint16_t glyph_id) const {
        auto it = glyph_to_class.find(glyph_id);
        return (it != glyph_to_class.end()) ? it->second : 0;
    }
};

// Value Record (for GPOS adjustments)
struct ValueRecord {
    int16_t x_placement = 0;
    int16_t y_placement = 0;
    int16_t x_advance = 0;
    int16_t y_advance = 0;
};

class GPOSReader {
public:
    // Extract kerning data from GPOS table using query-based approach
    // This mimics Node.js opentype.js behavior
    static bool extract_kerning(FT_Face face,
                               const std::vector<uint32_t>& codes,
                               std::map<uint32_t, std::map<uint32_t, int8_t>>& kerning_map);

private:
    struct PairPosFormat1Data {
        Coverage coverage;
        uint16_t value_format1;
        uint16_t value_format2;
        std::vector<uint16_t> pair_set_offsets;
        const uint8_t* subtable_base;
    };

    struct PairPosFormat2Data {
        Coverage coverage;
        ClassDef class_def1;
        ClassDef class_def2;
        uint16_t value_format1;
        uint16_t value_format2;
        uint16_t class1_count;
        uint16_t class2_count;
        const uint8_t* class_records;
        size_t class_record_size;
    };

    // Internal parsed GPOS structures (with pre-parsed subtables for performance)
    struct ParsedGPOS {
        std::vector<uint8_t> data;
        uint16_t lookup_list_offset;
        uint16_t lookup_count;
        int units_per_em;
        int pixel_size;

        // Pre-parsed subtables (cached for fast queries)
        std::vector<PairPosFormat1Data> format1_subtables;
        std::vector<PairPosFormat2Data> format2_subtables;
    };

    // Parse GPOS table and prepare for queries
    static bool parse_gpos_table(FT_Face face, ParsedGPOS& gpos);

    // Query kerning value between two glyph IDs
    static int16_t query_kerning_value(const ParsedGPOS& gpos, FT_UInt left_gid, FT_UInt right_gid);

    // Query individual subtable
    static int16_t query_pair_pos_format1(const PairPosFormat1Data& data,
                                         FT_UInt left_gid, FT_UInt right_gid);
    static int16_t query_pair_pos_format2(const PairPosFormat2Data& data,
                                         FT_UInt left_gid, FT_UInt right_gid);

    // Helper to read big-endian integers
    static inline uint16_t read_uint16(const uint8_t* data) {
        return (data[0] << 8) | data[1];
    }

    static inline int16_t read_int16(const uint8_t* data) {
        return static_cast<int16_t>((data[0] << 8) | data[1]);
    }

    static inline uint32_t read_uint32(const uint8_t* data) {
        return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    }

    // Parse Coverage table
    static bool parse_coverage(const uint8_t* data, size_t max_size, Coverage& coverage);

    // Parse ClassDef table
    static bool parse_classdef(const uint8_t* data, size_t max_size, ClassDef& classdef);

    // Parse Value Record
    static ValueRecord parse_value_record(const uint8_t*& data, uint16_t value_format);

    // Get size of value record based on format flags
    static size_t get_value_record_size(uint16_t value_format);

    // Parse PairPos Format 1
    static void parse_pair_pos_format1(const uint8_t* subtable,
                                      const uint8_t* base_data,
                                      size_t max_size,
                                      const std::map<uint32_t, FT_UInt>& code_to_gid,
                                      const std::map<FT_UInt, uint32_t>& gid_to_code,
                                      int units_per_em,
                                      int pixel_size,
                                      std::map<uint32_t, std::map<uint32_t, int8_t>>& kerning_map);

    // Parse PairPos Format 2
    static void parse_pair_pos_format2(const uint8_t* subtable,
                                      const uint8_t* base_data,
                                      size_t max_size,
                                      const std::map<uint32_t, FT_UInt>& code_to_gid,
                                      const std::map<FT_UInt, uint32_t>& gid_to_code,
                                      int units_per_em,
                                      int pixel_size,
                                      std::map<uint32_t, std::map<uint32_t, int8_t>>& kerning_map);
};

} // namespace lvgl
