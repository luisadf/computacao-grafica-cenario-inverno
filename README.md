# Cenário de Inverno 3D

Um pequeno cenário interativo feito em **C**, usando **OpenGL**, **GLU** (parte
do conjunto de bibliotecas OpenGL) e **FreeGLUT**. Não há C++ nem bibliotecas
adicionais para instalar.

## Compilar e executar

Esta configuração foi preparada para o GCC do MinGW e o FreeGLUT disponíveis
neste computador. O compilador, os cabeçalhos, a biblioteca de ligação e a
DLL do FreeGLUT precisam estar acessíveis. Em particular, `C:\MinGW\bin` deve
estar no `PATH` para o Windows encontrar `libfreeglut.dll` ao executar.

1. Abra esta pasta no VS Code.
2. Pressione **Ctrl+Shift+B** e escolha **Compilar Cenario de Inverno**, se solicitado.
3. No terminal do VS Code, execute:

   ```powershell
   .\cenario_inverno.exe
   ```

Para compilar pelo terminal, estando na pasta do projeto:

```powershell
gcc -std=c99 -Wall -Wextra -O2 src\main.c -o cenario_inverno.exe -lfreeglut -lopengl32 -lglu32 -lm
```

Se o Windows informar que não encontrou a DLL, confira se `C:\MinGW\bin`
está no `PATH` do terminal e reinicie o VS Code depois de alterar o `PATH`.
Em outro computador, instale ou configure um GCC e um FreeGLUT compatíveis
entre si, incluindo a mesma arquitetura de 32 ou 64 bits.

## Controles

| Tecla | Ação |
|---|---|
| D | Alternar entre dia e noite |
| N | Ligar ou desligar a neve |
| + / - | Aumentar ou diminuir a quantidade de flocos, de 50 a 700 |
| L | Acender ou apagar as janelas das três casas |
| T | Mostrar ou esconder o pisca-pisca e a estrela |
| Setas esquerda / direita | Girar a câmera ao redor do cenário |
| Setas cima / baixo | Alterar a altura da câmera |
| Page Up / Page Down | Aproximar / afastar a câmera |
| Esc | Fechar o programa |

O programa começa de dia, com 250 flocos, neve, luzes das casas e enfeites
ligados. A quantidade de neve pode ser alterada mesmo quando ela está
desligada; a nova quantidade aparece quando for ligada novamente. Os postes
acendem automaticamente à noite, independentemente das luzes das casas.

## Entendendo o código por etapas

Todo o programa está em `src\main.c`, organizado em funções para facilitar a
leitura:

1. **Estado e flocos:** as variáveis no início guardam as opções do cenário e
   da câmera. A estrutura `Snowflake` armazena posição, velocidade e fase de
   cada floco. `reset_snowflake` coloca um floco em uma posição nova.
2. **Formas auxiliares:** `box`, `sphere`, `cone` e `cylinder` desenham formas
   reutilizadas. `glPushMatrix` e `glPopMatrix` isolam as transformações para
   que o movimento ou a escala de um objeto não afetem os outros.
3. **Construção da aldeia:** `draw_ground` desenha um terreno grande em pequenas
   partes; `draw_path` acrescenta os caminhos. `draw_house` reutiliza a mesma
   casa em três posições e tamanhos. Bancos, postes, cercas e presentes
   completam a aldeia. Os caminhos usam transparência nas bordas para se
   misturar à neve.
4. **Montanhas e árvore:** `draw_mountain` constrói anéis irregulares para
   formar montanhas arredondadas e colori-las de neve no alto. `draw_pine`
   preserva o formato da árvore e usa o tempo de animação para alternar as
   luzes coloridas. À noite, uma esfera transparente acrescenta um leve brilho.
5. **Iluminação:** `display` escolhe a cor do céu e a iluminação global
   conforme o horário. As janelas iluminadas usam emissão de material; durante
   a noite, fontes de luz quentes iluminam suas proximidades. Uma névoa gradual
   mistura o terreno distante ao céu, escondendo as bordas. A iluminação é
   uma aproximação visual: não calcula sombras.
6. **Câmera:** `gluLookAt` posiciona a câmera em torno do cenário. Ângulo,
   altura e distância são ajustados nas funções de teclado. `reshape` mantém
   a perspectiva correta quando a janela muda de tamanho.
7. **Neve animada:** `timer` atualiza a posição dos flocos usando o tempo
   decorrido; ao chegar ao chão, cada floco reaparece no alto. `draw_snow`
   desenha a quantidade selecionada como pontos brancos. Ao desligar a neve,
   os flocos também ficam pausados, mas o pisca-pisca continua funcionando.
   `draw_chimney_smoke` usa o mesmo tempo de animação para fazer a fumaça
   clara subir e desaparecer, mesmo quando a neve está desligada.
8. **Interação:** `keyboard` trata letras, intensidade e saída; `special_keys`
   trata setas e zoom. `draw_hud` mostra um resumo dos comandos na janela.
9. **Inicialização:** `main` configura a janela, a profundidade, a iluminação
   e os callbacks do FreeGLUT. `glutMainLoop` mantém o programa recebendo
   eventos, enquanto `display` redesenha a cena.

Para apresentar, uma sequência possível é mostrar primeiro o cenário de dia,
alternar para a noite, mudar as luzes e os enfeites, ajustar a neve e, por fim,
girar e aproximar a câmera.
