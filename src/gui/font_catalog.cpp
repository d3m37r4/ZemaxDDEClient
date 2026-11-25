#include "gui/font_catalog.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <vector>

namespace gui::fonts {
    namespace {
        std::filesystem::path fontFolder() {
            wchar_t buffer[MAX_PATH]{};
            const UINT length = GetWindowsDirectoryW(buffer, MAX_PATH);
            const std::filesystem::path windows =
                length > 0 ? std::filesystem::path(buffer) : std::filesystem::path(L"C:\\Windows");
            return windows / L"Fonts";
        }

        bool isFontFile(const std::filesystem::path& path) {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return extension == ".ttf" || extension == ".otf" || extension == ".ttc";
        }

        // Minimal cmap reader: enough to answer "does this file map U+042F to a
        // real glyph". Formats 4 (BMP) and 12 (UCS-4) cover every font Windows
        // ships; anything else is treated as "no Cyrillic", which only means the
        // font is skipped instead of risking the atlas build.
        constexpr uint32_t kCyrillicProbe = 0x042F;   // 'Я'

        uint16_t readU16(const std::vector<uint8_t>& data, size_t offset) {
            return static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
        }

        uint32_t readU32(const std::vector<uint8_t>& data, size_t offset) {
            return (static_cast<uint32_t>(data[offset]) << 24) |
                   (static_cast<uint32_t>(data[offset + 1]) << 16) |
                   (static_cast<uint32_t>(data[offset + 2]) << 8) |
                   static_cast<uint32_t>(data[offset + 3]);
        }

        /// First font of a file: font collections (.ttc) start with 'ttcf'.
        size_t firstFontOffset(const std::vector<uint8_t>& data) {
            if (data.size() < 16 || readU32(data, 0) != 0x74746366u /* 'ttcf' */) {
                return 0;
            }
            return readU32(data, 12);
        }

        bool cmapMapsCyrillic(const std::vector<uint8_t>& data, uint32_t cmap, uint32_t codepoint) {
            if (cmap + 4 > data.size()) {
                return false;
            }
            const uint16_t records = readU16(data, cmap + 2);
            for (uint16_t i = 0; i < records; ++i) {
                const size_t record = cmap + 4 + static_cast<size_t>(i) * 8;
                if (record + 8 > data.size()) {
                    return false;
                }
                const size_t subtable = cmap + readU32(data, record + 4);
                if (subtable + 2 > data.size()) {
                    continue;
                }
                const uint16_t format = readU16(data, subtable);
                if (format == 12) {
                    if (subtable + 16 > data.size()) continue;
                    const uint32_t groups = readU32(data, subtable + 12);
                    for (uint32_t g = 0; g < groups; ++g) {
                        const size_t group = subtable + 16 + static_cast<size_t>(g) * 12;
                        if (group + 12 > data.size()) return false;
                        const uint32_t start = readU32(data, group);
                        const uint32_t end = readU32(data, group + 4);
                        if (codepoint < start || codepoint > end) continue;
                        const uint32_t firstGlyph = readU32(data, group + 8);
                        return firstGlyph + (codepoint - start) != 0;
                    }
                } else if (format == 4) {
                    if (subtable + 14 > data.size()) continue;
                    const uint16_t segCountX2 = readU16(data, subtable + 6);
                    const size_t endCodes = subtable + 14;
                    const size_t startCodes = endCodes + segCountX2 + 2;
                    const size_t idDeltas = startCodes + segCountX2;
                    const size_t idRangeOffsets = idDeltas + segCountX2;
                    if (idRangeOffsets + segCountX2 > data.size()) continue;
                    for (uint16_t s = 0; s < segCountX2 / 2; ++s) {
                        const uint16_t end = readU16(data, endCodes + s * 2);
                        if (codepoint > end) continue;
                        const uint16_t start = readU16(data, startCodes + s * 2);
                        if (codepoint < start) return false;
                        const uint16_t delta = readU16(data, idDeltas + s * 2);
                        const uint16_t rangeOffset = readU16(data, idRangeOffsets + s * 2);
                        if (rangeOffset == 0) {
                            return static_cast<uint16_t>(codepoint + delta) != 0;
                        }
                        const size_t glyphAddress = idRangeOffsets + s * 2 + rangeOffset +
                                                    (codepoint - start) * 2;
                        if (glyphAddress + 2 > data.size()) return false;
                        uint16_t glyph = readU16(data, glyphAddress);
                        if (glyph != 0) glyph = static_cast<uint16_t>(glyph + delta);
                        return glyph != 0;
                    }
                }
            }
            return false;
        }
    } // namespace

    std::string defaultFontPath() {
        return (fontFolder() / L"segoeui.ttf").string();
    }

    bool hasCyrillic(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        const std::vector<uint8_t> data{std::istreambuf_iterator<char>(file),
                                        std::istreambuf_iterator<char>()};
        const size_t base = firstFontOffset(data);
        if (base + 12 > data.size()) {
            return false;
        }
        const uint16_t tables = readU16(data, base + 4);
        if (base + 12 + static_cast<size_t>(tables) * 16 > data.size()) {
            return false;
        }
        uint32_t cmap = 0;
        for (uint16_t t = 0; t < tables; ++t) {
            const size_t record = base + 12 + static_cast<size_t>(t) * 16;
            if (readU32(data, record) == 0x636D6170u /* 'cmap' */) {
                cmap = readU32(data, record + 8);
                break;
            }
        }
        if (cmap == 0 || cmap >= data.size()) {
            return false;
        }
        return cmapMapsCyrillic(data, cmap, kCyrillicProbe);
    }

    std::vector<FontEntry> listSystemFonts() {
        std::vector<FontEntry> fonts;
        std::error_code ec;
        const std::filesystem::path folder = fontFolder();
        for (std::filesystem::directory_iterator it(folder, ec), end; !ec && it != end; it.increment(ec)) {
            const std::filesystem::path& path = it->path();
            if (!isFontFile(path)) {
                continue;
            }
            fonts.push_back({path.string(), path.stem().string()});
        }
        std::sort(fonts.begin(), fonts.end(),
            [](const FontEntry& a, const FontEntry& b) { return a.name < b.name; });
        return fonts;
    }
} // namespace gui::fonts
