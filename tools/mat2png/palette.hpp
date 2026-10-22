#pragma once
#include <cstdint>
#include <array>
#include <string>

namespace spritex::tools {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    constexpr Color() = default;
    constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

class Palette32 {
public:
    static constexpr size_t SIZE = 32;

    static Color get(uint8_t index) {
        if (index >= SIZE) return Color(0, 0, 0, 0);
        return DB32_COLORS[index];
    }

    static const char* get_name(uint8_t index) {
        if (index >= SIZE) return "Unknown";
        return COLOR_NAMES[index];
    }

private:
    static constexpr std::array<Color, SIZE> DB32_COLORS = {{
        Color(0, 0, 0, 0),          // 0: Transparente
        Color(0, 0, 0, 255),        // 1: Preto Puro (#000000)
        Color(34, 32, 52, 255),     // 2: Obsidiana Sombria (#222034)
        Color(69, 40, 60, 255),     // 3: Vinho Profundo (#45283c)
        Color(102, 57, 49, 255),    // 4: Marrom Escuro (#663931)
        Color(143, 86, 59, 255),    // 5: Marrom Madeira (#8f563b)
        Color(223, 113, 38, 255),   // 6: Ferrugem / Cobre (#df7126)
        Color(217, 160, 102, 255),  // 7: Pele Morena / Sombra (#d9a066)
        Color(238, 195, 154, 255),  // 8: Pele Clara / Bege (#eec39a)
        Color(251, 242, 54, 255),   // 9: Amarelo Ouro (#fbf236)
        Color(153, 229, 80, 255),   // 10: Verde Limão (#99e550)
        Color(106, 190, 48, 255),   // 11: Verde Folha (#6abe30)
        Color(55, 148, 110, 255),   // 12: Verde Musgo (#37946e)
        Color(75, 105, 47, 255),    // 13: Verde Pântano (#4b692f)
        Color(50, 60, 57, 255),     // 14: Verde Ardósia (#323c39)
        Color(63, 63, 116, 255),    // 15: Azul Meia-Noite (#3f3f74)
        Color(48, 96, 130, 255),    // 16: Azul Marinho (#306082)
        Color(91, 110, 225, 255),   // 17: Azul Safira (#5b6ee1)
        Color(99, 155, 255, 255),   // 18: Azul Céu / Arcano (#639bff)
        Color(95, 205, 228, 255),   // 19: Ciano / Gelo (#5fcde4)
        Color(203, 219, 252, 255),  // 20: Prata Reflexo (#cbdbfc)
        Color(255, 255, 255, 255),  // 21: Branco Puro (#ffffff)
        Color(155, 173, 183, 255),  // 22: Aço Prateado Claro (#9badb7)
        Color(132, 126, 135, 255),  // 23: Cinza Pedra Médio (#847e87)
        Color(105, 106, 106, 255),  // 24: Chumbo / Sombra Aço (#696a6a)
        Color(89, 86, 82, 255),     // 25: Grafite Profundo (#595652)
        Color(118, 66, 138, 255),   // 26: Púrpura / Magia (#76428a)
        Color(172, 50, 50, 255),    // 27: Vermelho Sangue (#ac3232)
        Color(247, 118, 34, 255),   // 28: Laranja Fogo (#f77622)
        Color(217, 87, 99, 255),    // 29: Rosa / Pele Suave (#d95763)
        Color(215, 123, 186, 255),  // 30: Magenta Mágico (#d77bba)
        Color(143, 151, 74, 255)    // 31: Oliva Dourado (#8f974a)
    }};

    static constexpr const char* COLOR_NAMES[SIZE] = {
        "Transparente", "Preto", "Obsidiana", "Vinho", "Marrom Escuro", "Madeira",
        "Ferrugem", "Pele Sombra", "Pele Clara", "Ouro", "Verde Limao", "Verde Folha",
        "Verde Musgo", "Verde Pantano", "Verde Ardosia", "Azul Meia-Noite", "Azul Marinho",
        "Azul Safira", "Azul Ceu", "Ciano", "Prata Reflexo", "Branco Puro",
        "Aco Claro", "Cinza Pedra", "Chumbo", "Grafite", "Purpura",
        "Vermelho Sangue", "Laranja Fogo", "Rosa", "Magenta", "Oliva Dourado"
    };
};

} // namespace spritex::tools
