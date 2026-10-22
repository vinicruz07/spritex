#pragma once
#include <cstdint>
#include <string>
#include <array>

namespace spritex::prototype {

struct Color {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};

    Color() = default;
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

class Palette {
public:
    std::string name;
    std::array<Color, 16> colors;

    Palette(const std::string& pal_name = "default") : name(pal_name) {
        // Token 0 is always transparent
        colors[0] = Color(0, 0, 0, 0);
    }

    void set(int token, Color c) {
        if (token >= 0 && token < 16) colors[token] = c;
    }

    Color get(int token) const {
        if (token < 0 || token >= 16) return Color(0, 0, 0, 0);
        return colors[token];
    }

    // --- Paletas Pre-fabricadas ---

    // Paleta de Ferro / Aço (Armaduras metálicas, lâminas)
    static Palette make_iron() {
        Palette p("Ferro/Aço");
        p.set(0,  Color(0, 0, 0, 0));       // Transparente
        p.set(1,  Color(24, 20, 37));       // Contorno escuro azulado
        p.set(2,  Color(38, 43, 68));       // Contorno secundário
        // Rampa Primária (Armadura / Metal)
        p.set(3,  Color(68, 77, 98));       // Sombra metal
        p.set(4,  Color(139, 155, 180));   // Tom médio metal
        p.set(5,  Color(203, 219, 252));   // Brilho metal
        // Rampa Secundária (Pele / Detalhe)
        p.set(6,  Color(143, 86, 59));      // Sombra pele
        p.set(7,  Color(217, 160, 102));   // Tom médio pele
        p.set(8,  Color(240, 200, 160));   // Luz pele
        // Detalhes & Acessórios
        p.set(9,  Color(100, 30, 30));      // Capa / Tecido escuro
        p.set(10, Color(180, 50, 50));      // Capa vermelha viva
        p.set(11, Color(220, 180, 50));     // Ouro / Fivela
        // Efeitos & Luzes
        p.set(12, Color(80, 140, 230));     // Reflexo azulado
        p.set(13, Color(140, 200, 255));    // Energia
        p.set(14, Color(220, 240, 255));    // Centelha
        p.set(15, Color(255, 255, 255));    // Highlight puro
        return p;
    }

    // Paleta de Madeira (Machados, arcos, cajados rústicos)
    static Palette make_wood() {
        Palette p("Madeira");
        p.set(0,  Color(0, 0, 0, 0));
        p.set(1,  Color(30, 18, 10));       // Contorno marrom profundo
        p.set(2,  Color(50, 30, 15));       // Sombra profunda
        // Rampa Primária (Madeira / Tronco)
        p.set(3,  Color(89, 47, 23));       // Sombra madeira
        p.set(4,  Color(134, 76, 38));      // Madeira base
        p.set(5,  Color(189, 126, 74));     // Madeira iluminada
        // Rampa Secundária (Lâmina / Metal amarrado)
        p.set(6,  Color(55, 60, 70));       // Metal escuro
        p.set(7,  Color(110, 120, 135));    // Metal médio
        p.set(8,  Color(180, 195, 210));    // Fio afiado
        // Detalhes (Musgo / Amarras de couro)
        p.set(9,  Color(52, 78, 38));       // Musgo verde escuro
        p.set(10, Color(98, 140, 58));      // Folhagem viva
        p.set(11, Color(180, 130, 80));     // Corda/Couro
        // Efeitos
        p.set(12, Color(220, 180, 120));    // Lasca de madeira
        p.set(13, Color(240, 220, 180));    // Poeira
        p.set(14, Color(220, 240, 250));    // Brilho do fio
        p.set(15, Color(255, 255, 255));    // Ponto de luz máxima
        return p;
    }

    // Paleta de Fogo & Magia Incandescente
    static Palette make_fire() {
        Palette p("Fogo/Magia");
        p.set(0,  Color(0, 0, 0, 0));
        p.set(1,  Color(40, 10, 10));       // Sombra de fumaça / Contorno avermelhado
        p.set(2,  Color(80, 15, 10));       // Brasa escura
        // Rampa Primária (Chamas externas)
        p.set(3,  Color(160, 25, 15));      // Vermelho profundo
        p.set(4,  Color(220, 60, 20));      // Laranja fogo
        p.set(5,  Color(255, 130, 25));     // Laranja quente
        // Rampa Secundária (Núcleo incandescente)
        p.set(6,  Color(255, 190, 40));     // Amarelo fogo
        p.set(7,  Color(255, 235, 100));    // Amarelo claro intenso
        p.set(8,  Color(255, 255, 180));    // Quase branco
        // Detalhes & Brasas voadoras
        p.set(9,  Color(70, 40, 50));       // Fumaça cinza
        p.set(10, Color(120, 70, 70));      // Fumaça clara
        p.set(11, Color(255, 80, 30));      // Centelha viva
        // Efeitos de Luz alta
        p.set(12, Color(255, 170, 50));     // Glow difuso
        p.set(13, Color(255, 220, 120));    // Glow intenso
        p.set(14, Color(255, 250, 210));    // Calor puro
        p.set(15, Color(255, 255, 255));    // Ponto central de calor máximo
        return p;
    }

    // Paleta Humanoide Loiro / Roupas Nobres (Cabelo Loiro Dourado + Pele Clara)
    static Palette make_blonde() {
        Palette p("Humanoide/Loira");
        p.set(0,  Color(0, 0, 0, 0));
        p.set(1,  Color(34, 28, 48));       // Contorno escuro elegante
        p.set(2,  Color(58, 40, 32));       // Sombra profunda
        // Rampa Primária (Roupa / Vestido Azul Safira Real)
        p.set(3,  Color(30, 55, 110));      // Sombra vestido
        p.set(4,  Color(45, 95, 180));      // Azul safira base
        p.set(5,  Color(90, 150, 235));     // Luz tecido
        // Rampa Secundária (Pele Humana Suave)
        p.set(6,  Color(185, 115, 85));     // Sombra pele pêssego
        p.set(7,  Color(235, 175, 135));    // Pele clara base
        p.set(8,  Color(255, 215, 185));    // Luz na pele
        // Rampa de Cabelo Loiro Dourado (9, 10, 11)
        p.set(9,  Color(175, 125, 45));     // Sombra cabelo loiro
        p.set(10, Color(235, 195, 75));     // Cabelo loiro dourado vivo
        p.set(11, Color(255, 235, 135));    // Brilho intenso nos fios
        // Detalhes (Olhos azuis, fivela)
        p.set(12, Color(30, 130, 190));     // Olho azul
        p.set(13, Color(220, 180, 50));     // Ouro / Adorno
        p.set(14, Color(255, 240, 190));    // Reflexo dourado
        p.set(15, Color(255, 255, 255));    // Brilho especular puro
        return p;
    }
};

} // namespace spritex::prototype
