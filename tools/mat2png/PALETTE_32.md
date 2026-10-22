# 🎨 Paleta Master Oficial de 32 Cores (DB32 Adaptada)

Esta é a tabela de referência de cores padrão para o **`spritex`** e o utilitário **`mat2png`**.
São **32 índices numéricos (`0` a `31`)** que cabem em 5 bits (`uint8_t`), cobrindo todo o espectro necessário para pixel art profissional.

---

## 📋 Tabela de Mapeamento (Índice, Cor, Hex e Função)

| Índice | Nome da Cor | Hex | RGBA | Família / Uso Sugerido |
|:---:|---|:---:|---|---|
| **`0`** | **Transparente** | `#000000` | `(0, 0, 0, 0)` | Fundo / Vazio transparente |
| **`1`** | **Preto Absoluto** | `#000000` | `(0, 0, 0, 255)` | Contornos externos, fendas profundas |
| **`2`** | **Obsidiana Sombria** | `#222034` | `(34, 32, 52, 255)` | Sombra de contorno, cabelo escuro |
| **`3`** | **Vinho Profundo** | `#45283c` | `(69, 40, 60, 255)` | Sombra de tecido, sangue seco |
| **`4`** | **Marrom Escuro** | `#663931` | `(102, 57, 49, 255)` | Madeira escura, couro gasto, botas |
| **`5`** | **Marrom Madeira** | `#8f563b` | `(143, 86, 59, 255)` | Cabos de armas, troncos, pelo |
| **`6`** | **Ferrugem / Cobre** | `#df7126` | `(223, 113, 38, 255)` | Couro claro, cerâmica, metal rústico |
| **`7`** | **Pele Morena / Sombra** | `#d9a066` | `(217, 160, 102, 255)` | Sombra facial, pele bronzeada |
| **`8`** | **Pele Clara / Bege** | `#eec39a` | `(238, 195, 154, 255)` | Rosto, mãos, luz na pele |
| **`9`** | **Amarelo Ouro** | `#fbf236` | `(251, 242, 54, 255)` | Ouro nobre, cabelo loiro, faíscas |
| **`10`** | **Verde Limão** | `#99e550` | `(153, 229, 80, 255)` | Veneno, ácido, luz de folhagem |
| **`11`** | **Verde Folha** | `#6abe30` | `(106, 190, 48, 255)` | Grama, folhas vivas, roupas de elfo |
| **`12`** | **Verde Musgo** | `#37946e` | `(55, 148, 110, 255)` | Floresta, escamas de réptil / orc |
| **`13`** | **Verde Pântano** | `#4b692f` | `(75, 105, 47, 255)` | Sombra de folhagem, camuflagem |
| **`14`** | **Verde Ardósia** | `#323c39` | `(50, 60, 57, 255)` | Pedra musgosa, sombra fria |
| **`15`** | **Azul Meia-Noite** | `#3f3f74` | `(63, 63, 116, 255)` | Sombra de tecido azul, capa noturna |
| **`16`** | **Azul Marinho** | `#306082` | `(48, 96, 130, 255)` | Armadura azul, aço frio sombra |
| **`17`** | **Azul Safira** | `#5b6ee1` | `(91, 110, 225, 255)` | Vestes reais, capas nobres |
| **`18`** | **Azul Céu / Arcano** | `#639bff` | `(99, 155, 255, 255)` | Magia arcana, energia, asas |
| **`19`** | **Ciano / Gelo** | `#5fcde4` | `(95, 205, 228, 255)` | Cristais de gelo, visores de elmo |
| **`20`** | **Prata Reflexo** | `#cbdbfc` | `(203, 219, 252, 255)` | Brilho intenso no metal azulado |
| **`21`** | **Branco Puro** | `#ffffff` | `(255, 255, 255, 255)` | Ponto de luz máxima, olhos, reflexos |
| **`22`** | **Aço Prateado Claro** | `#9badb7` | `(155, 173, 183, 255)` | Armadura de ferro / lâminas de espada |
| **`23`** | **Cinza Pedra Médio** | `#847e87` | `(132, 126, 135, 255)` | Rochas, cota de malha, cinzas |
| **`24`** | **Chumbo / Sombra Aço** | `#696a6a` | `(105, 106, 106, 255)` | Sombra de lâminas, metal escurecido |
| **`25`** | **Grafite Profundo** | `#595652` | `(89, 86, 82, 255)` | Armadura negra, ferro fundido |
| **`26`** | **Púrpura / Magia** | `#76428a` | `(118, 66, 138, 255)` | Necromancia, gemas de ametista |
| **`27`** | **Vermelho Sangue** | `#ac3232` | `(172, 50, 50, 255)` | Penachos de elmo, rubis, sangue |
| **`28`** | **Laranja Fogo** | `#f77622` | `(247, 118, 34, 255)` | Chamas vivas, brasas |
| **`29`** | **Rosa / Pele Suave** | `#d95763` | `(217, 87, 99, 255)` | Bochechas, lábios, tecido rosa |
| **`30`** | **Magenta Mágico** | `#d77bba` | `(215, 123, 186, 255)` | Poções, faíscas mágicas |
| **`31`** | **Oliva Dourado** | `#8f974a` | `(143, 151, 74, 255)` | Ouro velho, pátina de bronze, líquen |

---

## 💡 Como Usar na Matriz
Ao gerar ou colar matrizes de 32x32 ou 16x16:
* O número `0` sempre deixa o fundo transparente.
* Use `1` para contornos limpos.
* Use `22, 23, 24` para armaduras de ferro.
* Use `7, 8` para peles e rostos.
* Use `27, 28, 9` para labaredas de fogo e explosões.
* Use `17, 18, 19` para magias de água/gelo e visores brilhantes.
