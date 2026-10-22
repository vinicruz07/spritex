# 🛠️ mat2png — Matrix to Pixel Art Rasterizer

Ferramenta do ecossistema **`spritex`** para converter matrizes numéricas (de qualquer dimensão: 16x16, 32x32, 64x64) em imagens PNG nítidas com paleta de 32 cores master (DB32).

---

## 🎨 Paleta Padrão de 32 Cores
Consulte os arquivos desta pasta para mapeamento:
* [`PALETTE_32.md`](PALETTE_32.md) — Tabela visual de Índices (0 a 31), Nomes, Hex e Famílias.
* [`palette_32.json`](palette_32.json) — Base de dados em JSON para integração com scripts e frontends.
* [`palette.hpp`](palette.hpp) — Implementação em C++17 com `constexpr array` de 32 cores.

---

## 🔮 Roadmap Futuro
- [ ] Implementar a CLI nativa em C++ (`mat2png.exe`).
- [ ] Criar interface web/GUI dinâmica onde o usuário possa **colar qualquer matriz de texto** e visualizar a pixel art sendo renderizada em tempo real.
