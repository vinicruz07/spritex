# 🧪 Spritex — Protótipo Isolado

Este diretório contém um **protótipo autocontido e descartável** do `spritex`.

> ⚠️ **Aviso:** Esta pasta é completamente independente do resto do repositório. Você pode testar, rodar e **deletá-la a qualquer momento** sem quebrar a estrutura principal do projeto.

---

## 📦 O que está incluído aqui:
1. **`third_party/stb_image_write.h`**: Biblioteca single-header em C para exportação direta de PNGs sem dependências externas.
2. **`datasets/dataset.hpp`**: Dataset mockado contendo matrizes 16x16 com prompts reais:
   - *"um machado de madeira"* (Arma)
   - *"um cavaleiro de armadura"* (Humanoide)
   - *"uma bola de fogo magica"* (Magia / Efeito FX)
   - *"uma espada longa de ferro"* (Arma)
3. **`src/palette.hpp`**: Sistema de paletas de 16 cores (Madeira, Ferro/Aço, Fogo).
4. **`src/rasterizer.hpp`**: Rasterizador de matrizes para imagens PNG de 256x256 (com escalonamento *nearest-neighbor* 16x).
5. **`src/generator.hpp`**: Simulador dos passos iterativos de decodificação paralela do MaskGIT.
6. **`output/`**: Pasta onde os arquivos `.png` gerados são salvos.

---

## 🚀 Como Compilar e Rodar

Abra o terminal na pasta `spritex/`:

```bash
# Compilar com g++:
g++ -std=c++17 prototype/src/main.cpp -o prototype/spritex_proto.exe

# 1. Passar um prompt direto por linha de comando:
./prototype/spritex_proto.exe "uma espada de fogo"
./prototype/spritex_proto.exe "um cavaleiro de fogo"
./prototype/spritex_proto.exe "um machado de ferro"

# 2. Ou rodar em modo interativo (digitar prompts no terminal):
./prototype/spritex_proto.exe
```

Cada imagem gerada é salva automaticamente em `prototype/output/<nome_do_prompt>.png`.
