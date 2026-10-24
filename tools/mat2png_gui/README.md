# 🖥️ spritex Studio GUI — Visualizador & Editor Interativo de Pixel Art

Interface Gráfica (GUI) nativa em **C++17 puro (Win32 GDI)**, ultrarrápida, sem dependências externas pesadas e com visual moderno *Dark Theme*.

Permite importar qualquer imagem normal (PNG/JPG), colar matrizes numéricas geradas por LLMs ou criadas manualmente, editá-las em tempo real, remover fundos com 1 clique e visualizar a arte final instantaneamente a 60+ FPS.

---

## ✨ Arquitetura em Abas (Interface Limpa e Focada)

A interface agora é dividida em duas abas dedicadas no topo esquerdo, eliminando poluição visual e separando os fluxos de trabalho:

### 📝 Aba 1: `[ Matriz -> PNG ]`
* **Editor de Matriz com Zero Lag:** Área de texto ampla com parser em tempo real (< 0.3 ms).
* **Ações de Código:** Botões `[Colar]`, `[Auto-Ajustar]` (repara linhas desalinhadas de LLMs), `[Limpar]` e `[Remover Fundo]`.
* **Presets Rápidos:** Botões de 1 clique para carregar o Paladino 32x32 e o Guerreiro 16x16.
* **Diagnóstico Integrado:** Barra de status abaixo do editor informando resolução, total de tokens, cores únicas e tempo de parse.

### 🖼️ Aba 2: `[ PNG -> Pixel Art ]`
* **Importador Universal:** Carregue qualquer imagem PNG, JPG ou BMP com 1 clique.
* **Seletores de Resolução Alvo (16x16, 32x32, 64x64):**
  * Botões dedicados com indicação do modo ativo (`[ 16x16 ]`, `[ 32x32 ]`, `[ 64x64 ]`).
  * **Re-Pixelização Instantânea:** Troque a resolução com 1 clique e veja a pixel art se transformar em tempo real (< 45 ms), preservando a fidelidade da imagem original.
* **🪄 Removedor de Fundo Externo:** Botão que aplica *Outer Perimeter Flood-Fill*, apagando o fundo externo sem danificar olhos ou detalhes internos do sprite.
* **Card de Estatísticas em Tempo Real:** Mostra arquivo, dimensões originais, tokens gerados, cores DB32 utilizadas e tempo de processamento.
* **Ações de Exportação:** `[Copiar Matriz Gerada [Tokens]]` e `[Abrir no Editor de Matriz (Aba 1)]`.

---

## 🎨 Recursos Compartilhados (Ambas as Abas)

* **Canvas Interativo a 60+ FPS:** Fundo quadriculado (*checkerboard*) estilo Aseprite para transparência (Token 0).
* **Remoção de Bolsões com Botão Direito:** Clique com o botão direito em qualquer pixel da tela para apagar uma cor específica por inundação.
* **Inspetor de Pixels:** Ao mover o cursor sobre o sprite, exibe `(X, Y)`, número do token, nome e código Hex da cor DB32.
* **Régua / Paleta Master DB32 no Rodapé:** Todos os 32 swatches clicáveis (clicar copia o número do token).
* **Seleção de Escala & Grid:** Escalas de 1x a 24x e grade de pixels ativável.
* **Exportação Profissional:** Salvar em PNG HD (16x), PNG Nativo (1x) ou Copiar Imagem diretamente para a Área de Transferência (`CF_DIB`).


---

## 🚀 Como Executar

Basta abrir o executável diretamente:

```bash
# Pelo terminal dentro da pasta:
cd "tools/mat2png_gui"
.\mat2png_gui.exe
```

Ou dar um duplo clique em `mat2png_gui.exe` no Windows Explorer!

---

## 🛠️ Compilação

### Via g++ (MinGW)
```bash
g++ -std=c++17 -O2 -I../../third_party -I../mat2png -I../img2mat -I. main.cpp -mwindows -lgdi32 -luser32 -lcomdlg32 -lcomctl32 -o mat2png_gui.exe
```

### Via CMake
```bash
cmake -B build
cmake --build build --target mat2png_gui
```
