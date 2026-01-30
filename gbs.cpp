#define _CRT_SECURE_NO_WARNINGS
#include "reader_writer.h"
#include "gbs.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;

namespace gbs {

    uint32_t readU32(std::vector<unsigned char>& buffer, size_t offset, bool be) {
        return be ? reader::BE_readBuffer_VectorUnChar_to_UnInt32(buffer, offset) : reader::LE_readBuffer_VectorUnChar_to_UnInt32(buffer, offset);
    }
    uint16_t readU16(std::vector<unsigned char>& buffer, size_t offset, bool be) {
        if (be) return (uint16_t)((buffer[offset] << 8) | buffer[offset + 1]);
        else return (uint16_t)(buffer[offset] | (buffer[offset + 1] << 8));
    }

    void writeU32(std::vector<unsigned char>& buffer, uint32_t value, bool be) {
        if (be) writer::appendBE32(buffer, value);
        else writer::appendLE32(buffer, value);
    }
    void writeU16(std::vector<unsigned char>& buffer, uint16_t value, bool be) {
        if (be) writer::appendBE16(buffer, value);
        else writer::appendLE16(buffer, value);
    }
    void appendStringPadded(std::vector<unsigned char>& buffer, const std::string& str, size_t total_size) {
        size_t to_copy = std::min(str.size(), total_size);
        buffer.insert(buffer.end(), str.begin(), str.begin() + to_copy);
        if (total_size > to_copy) {
            buffer.insert(buffer.end(), total_size - to_copy, 0);
        }
    }

gbs_t::gbs_t(const fs::path pathtoread, bool is_ps3) {
    m_is_ps3 = is_ps3;
    try {
        std::fstream File;
        File.open(pathtoread, std::ios::out | std::ios::in | std::ios::binary | std::ios::ate);
        if (File.is_open()) {
            auto size = File.tellg();
            File.seekg(0);
            file_buffer.resize((size_t)size);
            File.read(reinterpret_cast<char*>(file_buffer.data()), size);
            _read();
        }
        else throw std::runtime_error("Could not open file");
    }
    catch (...) {
        throw;
    }
}

void gbs_t::_read() {
    m_gbsc_header = reader::readBuffer_VectorUnChar_to_String(file_buffer, 0, 4);

    // Auto-detect endianness
    bool be = false;
    if (file_buffer.size() >= 8) {
        uint32_t size_le = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 4);
        uint32_t size_be = reader::BE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 4);
        if (size_be == file_buffer.size() || size_be == file_buffer.size() - 4) be = true;
        else if (size_le == file_buffer.size() || size_le == file_buffer.size() - 4) be = false;
        else {
            // Fallback to checking footer
            if (file_buffer.size() >= 4) {
                uint32_t footer_be = reader::BE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, file_buffer.size() - 4);
                if (footer_be == 0x12344321) be = true;
            }
        }
    }

    m_file_size = readU32(file_buffer, 4, be);
    m_data_version = reader::readBuffer_VectorUnChar_to_String(file_buffer, 8, 8);
    m_scene_id = readU32(file_buffer, 16, be);
    m_textures_count = readU32(file_buffer, 20, be);
    m_fonts_count = readU32(file_buffer, 24, be);
    m_sounds_count = readU32(file_buffer, 28, be);
    m_views_count = readU32(file_buffer, 32, be);
    m_messages_count = readU32(file_buffer, 36, be);
    m_textures_offset = readU32(file_buffer, 40, be);
    m_fonts_offset = readU32(file_buffer, 44, be);
    m_sounds_offset = readU32(file_buffer, 48, be);
    m_view_offset = readU32(file_buffer, 52, be);
    m_messages_offset = readU32(file_buffer, 56, be);

    size_t high_water_mark = 0x3C;

    size_t ptr = m_fonts_offset + 0x3C;
    for (uint32_t i = 0; i < m_fonts_count; i++) {
        m_fonts.push_back(font_t(file_buffer, ptr, be));
        ptr += m_fonts.back().size();
    }
    if (m_fonts_count > 0) high_water_mark = std::max(high_water_mark, ptr);

    ptr = m_textures_offset + 0x3C;
    for (uint32_t i = 0; i < m_textures_count; i++) {
        m_textures.push_back(texture_t(file_buffer, ptr, be));
        ptr += m_textures.back().size();
    }
    if (m_textures_count > 0) high_water_mark = std::max(high_water_mark, ptr);

    auto get_section_end = [&](uint32_t offset) -> size_t {
        if (offset == 0) return 0;
        uint32_t next_offset = 0xFFFFFFFF;
        auto check = [&](uint32_t o) { if (o > offset && o < next_offset) next_offset = o; };
        check(m_textures_offset);
        check(m_fonts_offset);
        check(m_sounds_offset);
        check(m_view_offset);
        check(m_messages_offset);
        if (next_offset == 0xFFFFFFFF) {
            // Check for footer
            if (file_buffer.size() >= 4) {
                uint32_t footer_le = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, file_buffer.size() - 4);
                uint32_t footer_be = reader::BE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, file_buffer.size() - 4);
                if (footer_le == 0x12344321 || footer_be == 0x12344321) {
                    return file_buffer.size() - 4 - 0x3C;
                }
            }
            return file_buffer.size() - 0x3C;
        }
        return next_offset;
    };

    if (m_sounds_count > 0 && m_sounds_offset > 0) {
        size_t start = m_sounds_offset + 0x3C;
        size_t end = get_section_end(m_sounds_offset) + 0x3C;
        if (end > start && end <= file_buffer.size())
            m_sounds_raw.assign(file_buffer.begin() + start, file_buffer.begin() + end);
    }
    if (m_views_count > 0 && m_view_offset > 0) {
        size_t start = m_view_offset + 0x3C;
        size_t end = get_section_end(m_view_offset) + 0x3C;
        if (end > start && end <= file_buffer.size())
            m_views_raw.assign(file_buffer.begin() + start, file_buffer.begin() + end);
    }
    if (m_messages_count > 0 && m_messages_offset > 0) {
        size_t start = m_messages_offset + 0x3C;
        size_t end = get_section_end(m_messages_offset) + 0x3C;
        if (end > start && end <= file_buffer.size()) {
            m_messages_raw.assign(file_buffer.begin() + start, file_buffer.begin() + end);
            high_water_mark = std::max(high_water_mark, end);
        }
    }
    if (m_sounds_count > 0 && m_sounds_offset > 0) high_water_mark = std::max(high_water_mark, m_sounds_offset + 0x3C + m_sounds_raw.size());
    if (m_views_count > 0 && m_view_offset > 0) high_water_mark = std::max(high_water_mark, m_view_offset + 0x3C + m_views_raw.size());

    if (file_buffer.size() >= 4) {
        uint32_t footer_le = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, file_buffer.size() - 4);
        uint32_t footer_be = reader::BE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, file_buffer.size() - 4);
        if (footer_le == 0x12344321 || footer_be == 0x12344321) {
            m_footer = 0x12344321;
            if (file_buffer.size() - 4 > high_water_mark) {
                m_tail_raw.assign(file_buffer.begin() + high_water_mark, file_buffer.begin() + file_buffer.size() - 4);
            }
        }
        else {
            if (file_buffer.size() > high_water_mark) {
                m_tail_raw.assign(file_buffer.begin() + high_water_mark, file_buffer.end());
            }
        }
    }
}

gbs_t::font_t::font_t(std::vector<unsigned char>& buffer, size_t ptr, bool be) {
    try {
        _read(buffer, ptr, be);
    }
    catch (...) {
        throw;
    }
}

void gbs_t::font_t::_read(std::vector<unsigned char>& file_buffer, size_t ptr, bool be) {
    m_gfnt_label = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr, 4);
    m_font_length = readU32(file_buffer, ptr + 4, be);
    m_font_id = readU32(file_buffer, ptr + 8, be);
    m_font_name = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr + 12, 64);
    m_font_size = readU32(file_buffer, ptr + 76, be);
    m_atlas_w = readU32(file_buffer, ptr + 80, be);
    m_atlas_h = readU32(file_buffer, ptr + 84, be);
    m_max_top = readU32(file_buffer, ptr + 88, be);
    m_atlas_count = readU32(file_buffer, ptr + 92, be);
    uint32_t l_chars = readU32(file_buffer, ptr + 96, be);
    m_chars_count = l_chars;
    size_t ptr_f = ptr + 100;
    for (uint32_t i = 0; i < l_chars; i++) {
        m_chars.push_back(char_t(file_buffer, ptr_f, be));
        ptr_f += 0x28;
    }
}

gbs_t::char_t::char_t(std::vector <unsigned char>& file_buffer, size_t ptr_f, bool be) {
    try {
        _read(file_buffer, ptr_f, be);
    }
    catch (...) {
        throw;
    }
}

void gbs_t::char_t::_read(std::vector <unsigned char>& file_buffer, size_t ptr, bool be) {
    m_char_code = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr, 4);
    m_is_image_glyph = readU32(file_buffer, ptr + 4, be);
    m_char_x_offset = readU32(file_buffer, ptr + 8, be);
    m_char_y_offset = readU32(file_buffer, ptr + 12, be);
    m_char_w = readU32(file_buffer, ptr + 16, be);
    m_char_h = readU32(file_buffer, ptr + 20, be);
    m_char_top = readU32(file_buffer, ptr + 24, be);
    m_char_advance = readU32(file_buffer, ptr + 28, be);
    m_char_left_bearning = readU32(file_buffer, ptr + 32, be);
    m_char_atlas_index = readU32(file_buffer, ptr + 36, be);
}

gbs_t::texture_t::texture_t(std::vector<unsigned char>& buffer, size_t ptr, bool be) {
    try {
        _read(buffer, ptr, be);
    }
    catch (...) {
        throw;
    }
}

void gbs_t::texture_t::_read(std::vector<unsigned char>& file_buffer, size_t ptr, bool be) {
    m_id = readU16(file_buffer, ptr, be);
    m_type = readU16(file_buffer, ptr + 2, be);
    m_path = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr + 4, 260);
}
 
void gbs_t::write(fs::path const pathtowrite) {
    std::ofstream export_file(pathtowrite, std::ios::out | std::ios::binary);
    if (!export_file.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }
    bool be = m_is_ps3;
    std::vector<unsigned char> buffer;
    buffer.resize(0x3C); // Header placeholder

    // Write Textures
    if (m_textures_count > 0) {
        m_textures_offset = (uint32_t)(buffer.size() - 0x3C);
        for (const auto& texture : m_textures) {
            writeU16(buffer, texture.id(), be);
            writeU16(buffer, texture.type(), be);
            appendStringPadded(buffer, texture.path(), 260);
            writeU16(buffer, texture.id(), be);
            writeU16(buffer, texture.type(), be);

            if (be) writer::appendBE64(buffer, 0);
            else writer::appendLE64(buffer, 0);

            writeU16(buffer, 0, be);
            writeU16(buffer, uint16_t(0x3f80), be);
            writeU16(buffer, 0, be);
            writeU16(buffer, uint16_t(0x3f80), be);
        }
    }
    else m_textures_offset = 0;

    // Write Fonts
    if (m_fonts_count > 0) {
        m_fonts_offset = (uint32_t)(buffer.size() - 0x3C);
        for (const auto& font : m_fonts) {
            writer::appendString(buffer, font.gfnt_label());
            writeU32(buffer, font.font_length(), be);
            writeU32(buffer, font.font_id(), be);
            appendStringPadded(buffer, font.font_name(), 64);
            writeU32(buffer, font.font_size(), be);
            writeU32(buffer, font.atlas_w(), be);
            writeU32(buffer, font.atlas_h(), be);
            writeU32(buffer, font.max_top(), be);
            writeU32(buffer, font.atlas_count(), be);
            writeU32(buffer, font.chars_count(), be);
            for (const auto& letter : font.chars()) {
                writer::appendString(buffer, letter.char_code());
                writeU32(buffer, letter.is_image_glyph(), be);
                writeU32(buffer, letter.char_x_offset(), be);
                writeU32(buffer, letter.char_y_offset(), be);
                writeU32(buffer, letter.char_w(), be);
                writeU32(buffer, letter.char_h(), be);
                writeU32(buffer, letter.char_top(), be);
                writeU32(buffer, letter.char_advance(), be);
                writeU32(buffer, letter.char_left_bearning(), be);
                writeU32(buffer, letter.char_atlas_index(), be);
            }
        }
    }
    else m_fonts_offset = 0;

    // Write Raw sections
    auto write_raw = [&](uint32_t& offset, uint32_t count, const std::vector<unsigned char>& raw) {
        if (count > 0 && !raw.empty()) {
            offset = (uint32_t)(buffer.size() - 0x3C);
            buffer.insert(buffer.end(), raw.begin(), raw.end());
        }
        else offset = 0;
    };

    write_raw(m_sounds_offset, m_sounds_count, m_sounds_raw);
    write_raw(m_view_offset, m_views_count, m_views_raw);
    write_raw(m_messages_offset, m_messages_count, m_messages_raw);

    // Write Tail
    if (!m_tail_raw.empty()) {
        buffer.insert(buffer.end(), m_tail_raw.begin(), m_tail_raw.end());
    }

    // Write Footer
    if (m_footer != 0) {
        if (be) writer::appendBE32(buffer, m_footer);
        else writer::appendLE32(buffer, m_footer);
    }

    m_file_size = (uint32_t)buffer.size();

    // Fill Header
    std::vector<unsigned char> header;
    writer::appendString(header, m_gbsc_header);
    writeU32(header, m_file_size, be);
    appendStringPadded(header, m_data_version, 8);
    writeU32(header, uint32_t(m_scene_id), be);
    writeU32(header, uint32_t(m_textures_count), be);
    writeU32(header, uint32_t(m_fonts_count), be);
    writeU32(header, uint32_t(m_sounds_count), be);
    writeU32(header, uint32_t(m_views_count), be);
    writeU32(header, uint32_t(m_messages_count), be);
    writeU32(header, uint32_t(m_textures_offset), be);
    writeU32(header, uint32_t(m_fonts_offset), be);
    writeU32(header, uint32_t(m_sounds_offset), be);
    writeU32(header, uint32_t(m_view_offset), be);
    writeU32(header, uint32_t(m_messages_offset), be);

    std::memcpy(buffer.data(), header.data(), 0x3C);

    export_file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
}

gbs_t merge(gbs_t& first_gbs, gbs_t& second_gbs, config config) {
    gbs_t merged_gbs = first_gbs;

    double divider = 1.5;
    double divider2 = 1.45;

    for (const auto& font : second_gbs.m_fonts) {
        bool font_exists = false;

        for (auto& existing_font : merged_gbs.m_fonts) {
            uint32_t old_atlas_count = existing_font.m_atlas_count;
            if (existing_font.m_font_id == font.m_font_id) {
                font_exists = true;

                for (const auto& letter : font.m_chars) {
                    bool letter_exists = false;
                    if (existing_font.m_max_top < (uint32_t)round(double(font.m_max_top) / divider) && has_flag(config, divide_coords)) {
                        existing_font.m_max_top = (uint32_t)round(double(font.m_max_top) / divider);
                    }
                    for (const auto& existing_letter : existing_font.m_chars) {
                        if (existing_letter.m_char_code == letter.m_char_code) {
                            letter_exists = true;
                            break;
                        }
                    }

                    if (!letter_exists) {
                        gbs_t::char_t current = letter;
                        if (has_flag(config, divide_coords)) {
                            current.m_char_x_offset = (uint32_t)round(double(current.m_char_x_offset) / divider);
                            current.m_char_y_offset = (uint32_t)round(double(current.m_char_y_offset) / divider);
                            current.m_char_w = (uint32_t)round(double(current.m_char_w) / divider);
                            current.m_char_h = (uint32_t)round(double(current.m_char_h) / divider);
                            current.m_char_top = (uint32_t)round(double(current.m_char_top) / divider);
                            current.m_char_advance = (uint32_t)round(double(current.m_char_advance) / divider2);
                        }
                        current.m_char_atlas_index += old_atlas_count;

                        existing_font.m_chars.push_back(current);
                        if (current.m_char_atlas_index + 1 > existing_font.m_atlas_count) {
                            existing_font.m_atlas_count = current.m_char_atlas_index + 1;
                        }
                        existing_font.m_chars_count++;
                        existing_font.m_font_length += 0x28;
                        
                        merged_gbs.m_file_size += 0x28;
                        merged_gbs.m_textures_offset += 0x28;
                        merged_gbs.m_sounds_offset += 0x28;
                        merged_gbs.m_view_offset += 0x28;
                        merged_gbs.m_messages_offset += 0x28;
                    }
                }
                break;
            }
        }

        if (!font_exists && has_flag(config, add_new_fonts)) {
            merged_gbs.m_fonts.push_back(font);
            merged_gbs.m_fonts_count++;
        }
    }

    for (const auto& texture : second_gbs.m_textures) {
        bool texture_exists = false;
        for (const auto& existing_texture : merged_gbs.m_textures) {
            if (texture.m_id == existing_texture.m_id) {
                texture_exists = true;
                break;
            }
        }
        if (!texture_exists) {
            gbs_t::texture_t current = texture;
            if (has_flag(config, calculate_texture_id)) {
                current.m_id = (uint16_t)(merged_gbs.m_textures.back().m_id + 1);
            }
            merged_gbs.m_textures.push_back(current);
            merged_gbs.m_textures_count++;

            merged_gbs.m_file_size += 0x11C;
            merged_gbs.m_sounds_offset += 0x11C;
            merged_gbs.m_view_offset += 0x11C;
            merged_gbs.m_messages_offset += 0x11C;
        }
    }
    
    return merged_gbs;
}

} 
