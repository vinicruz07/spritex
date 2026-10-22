#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include "palette.hpp"
#include "../datasets/dataset.hpp"

namespace spritex::prototype {

struct SemanticIntent {
    std::string category;       // "weapon", "humanoid", "spell"
    std::string palette_name;   // "iron", "wood", "fire"
    std::string object_name;    // "machado", "espada", "cavaleiro", "fogo"
};

class SemanticRouter {
public:
    static SemanticIntent parse(const std::string& input) {
        std::string text = input;
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        SemanticIntent intent;
        intent.category = "weapon";        // fallback padrão
        intent.palette_name = "iron";      // fallback padrão
        intent.object_name = "espada";     // fallback padrão

        // 1. Detectar Objeto e Categoria
        if (text.find("machado") != std::string::npos) {
            intent.category = "weapon";
            intent.object_name = "machado";
        } else if (text.find("espada") != std::string::npos || text.find("lamina") != std::string::npos) {
            intent.category = "weapon";
            intent.object_name = "espada";
        } else if (text.find("mulher") != std::string::npos || text.find("garota") != std::string::npos || text.find("menina") != std::string::npos || text.find("princesa") != std::string::npos || text.find("dama") != std::string::npos) {
            intent.category = "humanoid";
            intent.object_name = "mulher";
        } else if (text.find("cavaleiro") != std::string::npos || text.find("guerreiro") != std::string::npos || text.find("heroi") != std::string::npos || text.find("homem") != std::string::npos || text.find("pessoa") != std::string::npos) {
            intent.category = "humanoid";
            intent.object_name = "cavaleiro";
        } else if (text.find("fogo") != std::string::npos || text.find("chama") != std::string::npos || text.find("magia") != std::string::npos || text.find("bola") != std::string::npos) {
            intent.category = "spell";
            intent.object_name = "fogo";
        }

        // 2. Detectar Material / Paleta de Cores
        if (text.find("loira") != std::string::npos || text.find("loiro") != std::string::npos || text.find("dourado") != std::string::npos || text.find("ouro") != std::string::npos) {
            intent.palette_name = "blonde";
        } else if (text.find("madeira") != std::string::npos || text.find("natureza") != std::string::npos || text.find("floresta") != std::string::npos) {
            intent.palette_name = "wood";
        } else if (text.find("fogo") != std::string::npos || text.find("chama") != std::string::npos || text.find("brasa") != std::string::npos || text.find("magico") != std::string::npos || text.find("magica") != std::string::npos) {
            intent.palette_name = "fire";
        } else if (text.find("ferro") != std::string::npos || text.find("aco") != std::string::npos || text.find("metal") != std::string::npos || text.find("armadura") != std::string::npos) {
            intent.palette_name = "iron";
        } else {
            // Paleta padrão baseada no objeto caso o material não tenha sido explicitado
            if (intent.object_name == "mulher") intent.palette_name = "blonde";
            else if (intent.object_name == "machado") intent.palette_name = "wood";
            else if (intent.object_name == "fogo") intent.palette_name = "fire";
            else intent.palette_name = "iron";
        }

        return intent;
    }

    static SpriteSample resolve_sample(const std::string& raw_prompt) {
        SemanticIntent intent = parse(raw_prompt);
        auto samples = DatasetRepository::get_samples();

        SpriteSample chosen;

        // Selecionar a geometria base mais próxima
        for (const auto& s : samples) {
            if (intent.object_name == "mulher" && s.prompt.find("mulher") != std::string::npos) {
                chosen = s;
                break;
            } else if (intent.object_name == "machado" && s.prompt.find("machado") != std::string::npos) {
                chosen = s;
                break;
            } else if (intent.object_name == "espada" && s.prompt.find("espada") != std::string::npos) {
                chosen = s;
                break;
            } else if (intent.object_name == "cavaleiro" && s.prompt.find("cavaleiro") != std::string::npos) {
                chosen = s;
                break;
            } else if (intent.object_name == "fogo" && s.prompt.find("fogo") != std::string::npos) {
                chosen = s;
                break;
            }
        }

        if (chosen.matrix.empty()) {
            chosen = samples[0];
        }

        // Sobrescrever com o prompt do usuário e a paleta identificada
        chosen.prompt = raw_prompt;
        chosen.category = intent.category;
        chosen.default_palette = intent.palette_name;

        return chosen;
    }
};

} // namespace spritex::prototype
