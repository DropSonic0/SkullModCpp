#define _CRT_SECURE_NO_WARNINGS
#include "reader_writer.h"
#include "gbs.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;

namespace gbs {

gbs_t::gbs_t(const fs::path pathtoread) {
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
    m_file_size = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 4);
    m_data_version = reader::readBuffer_VectorUnChar_to_String(file_buffer, 8, 8);
    m_scene_id = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 16);
    m_fonts_count = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 20);
    m_textures_count = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 24);
    m_sounds_count = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 28);
    m_views_count = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 32);
    m_messages_count = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 36);
    m_fonts_offset = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 40);
    m_textures_offset = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 44);
    m_sounds_offset = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 48);
    m_view_offset = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 52);
    m_messages_offset = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, 56);

    size_t ptr = m_fonts_offset;
    for (uint32_t i = 0; i < m_fonts_count; i++) {
        m_fonts.push_back(font_t(file_buffer, ptr));
        ptr += m_fonts.back().size();
    }
    ptr = m_textures_offset;
    for (uint32_t i = 0; i < m_textures_count; i++) {
        m_textures.push_back(texture_t(file_buffer, ptr));
        ptr += m_textures.back().size();
    }
}

gbs_t::font_t::font_t(std::vector<unsigned char>& buffer, size_t ptr) {
    try {
        _read(buffer, ptr);
    }
    catch (...) {
        throw;
    }
}

void gbs_t::font_t::_read(std::vector<unsigned char>& file_buffer, size_t ptr) {
    m_gfnt_lable = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr, 4);
    m_font_lenght = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 4);
    m_font_id = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 8);
    m_font_name = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr + 12, 64);
    m_font_size = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 76);
    m_atlas_w = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 80);
    m_atlas_h = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 84);
    m_max_top = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 88);
    m_atlas_count = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 92);
    uint32_t l_chars = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 96);
    m_chars_count = l_chars;
    size_t ptr_f = ptr + 100;
    for (uint32_t i = 0; i < l_chars; i++) {
        m_chars.push_back(char_t(file_buffer, ptr_f));
        ptr_f += 0x28;
    }
}

gbs_t::char_t::char_t(std::vector <unsigned char>& file_buffer, size_t ptr_f) {
    try {
        _read(file_buffer, ptr_f);
    }
    catch (...) {
        throw;
    }
}

void gbs_t::char_t::_read(std::vector <unsigned char>& file_buffer, size_t ptr) {
    m_char_code = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr, 4);
    m_is_image_glyph = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 4);
    m_char_x_offset = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 8);
    m_char_y_offset = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 12);
    m_char_w = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 16);
    m_char_h = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 20);
    m_char_top = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 24);
    m_char_advance = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 28);
    m_char_left_bearning = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 32);
    m_char_atlas_index = reader::LE_readBuffer_VectorUnChar_to_UnInt32(file_buffer, ptr + 36);
}

gbs_t::texture_t::texture_t(std::vector<unsigned char>& buffer, size_t ptr) {
    try {
        _read(buffer, ptr);
    }
    catch (...) {
        throw;
    }
}

void gbs_t::texture_t::_read(std::vector<unsigned char>& file_buffer, size_t ptr) {
    m_id = (uint16_t)(file_buffer[ptr] + (file_buffer[ptr + 1] << 8));
    m_type = (uint16_t)(file_buffer[ptr + 2] + (file_buffer[ptr + 3] << 8));
    m_path = reader::readBuffer_VectorUnChar_to_String(file_buffer, ptr + 4, 260);
}
 
void gbs_t::write(fs::path const pathtowrite) {
    std::ofstream export_file(pathtowrite, std::ios::out | std::ios::binary);
    if (!export_file.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }
    std::vector<unsigned char> buffer;
    writer::appendString(buffer, m_gbsc_header);
    writer::appendLE32(buffer, m_file_size);
    writer::appendString(buffer, m_data_version);
    writer::appendLE32(buffer, uint32_t(m_scene_id));
    writer::appendLE32(buffer, uint32_t(m_fonts_count));
    writer::appendLE32(buffer, uint32_t(m_textures_count));
    writer::appendLE32(buffer, uint32_t(m_sounds_count));
    writer::appendLE32(buffer, uint32_t(m_views_count));
    writer::appendLE32(buffer, uint32_t(m_messages_count));
    writer::appendLE32(buffer, uint32_t(m_fonts_offset));
    writer::appendLE32(buffer, uint32_t(m_textures_offset));
    writer::appendLE32(buffer, uint32_t(m_sounds_offset));
    writer::appendLE32(buffer, uint32_t(m_view_offset));
    writer::appendLE32(buffer, uint32_t(m_messages_offset));
    for (const auto& font : m_fonts) {
        writer::appendString(buffer, font.gfnt_lable());
        writer::appendLE32(buffer, font.font_lenght());
        writer::appendLE32(buffer, font.font_id());
        writer::appendString(buffer, font.font_name());
        writer::appendLE32(buffer, font.font_size());
        writer::appendLE32(buffer, font.atlas_w());
        writer::appendLE32(buffer, font.atlas_h());
        writer::appendLE32(buffer, font.max_top());
        writer::appendLE32(buffer, font.atlas_count());
        writer::appendLE32(buffer, font.chars_count());
        for (const auto& letter : font.chars()) {
            writer::appendString(buffer, letter.char_code());
            writer::appendLE32(buffer, letter.is_image_glyph());
            writer::appendLE32(buffer, letter.char_x_offset());
            writer::appendLE32(buffer, letter.char_y_offset());
            writer::appendLE32(buffer, letter.char_w());
            writer::appendLE32(buffer, letter.char_h());
            writer::appendLE32(buffer, letter.char_top());
            writer::appendLE32(buffer, letter.char_advance());
            writer::appendLE32(buffer, letter.char_left_bearning());
            writer::appendLE32(buffer, letter.char_atlas_index());
        }
    }
    for (const auto& texture : m_textures) {
        writer::appendLE16(buffer, texture.id());
        writer::appendLE16(buffer, texture.type());
        writer::appendString(buffer, texture.path());
        writer::appendLE16(buffer, texture.id());
        writer::appendLE16(buffer, texture.type());

        writer::appendLE64(buffer, 0);

        writer::appendLE16(buffer, 0);
        writer::appendLE16(buffer, uint16_t(0x3f80));
        writer::appendLE16(buffer, 0);
        writer::appendLE16(buffer, uint16_t(0x3f80));
    }
    buffer.insert(buffer.end(), file_buffer.begin(), file_buffer.end());
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
                        existing_font.m_font_lenght += 0x28;
                        
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
