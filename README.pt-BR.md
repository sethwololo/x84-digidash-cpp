# x84-digidash-cpp

[English](README.md)

Painel de instrumentos digital para o Renault Mégane II (plataforma X84), em C++ com LVGL. Roda numa placa
Waveshare ESP32-P4 com tela DSI de 8,8" (480x1920, retrato) usada em paisagem (1920x480) e lê o CAN do carro em modo
listen-only: nunca transmite.

É a versão C++ do projeto privado Rust + Slint `x84-digidash`: mesma tela, mesma leitura do CAN, cerca do dobro de quadros
por segundo.

![À noite](docs/night.png)
![De dia, combustível na reserva](docs/day.png)

## Recursos

- Rotação com barra e faixa vermelha, velocidade, marcha (com debounce), piloto automático e limitador.
- Temperatura do motor, tensão da bateria e temperatura externa; nível de combustível com alerta de reserva; nível
  do óleo.
- Portas e porta-malas abertos numa vista de cima do carro; setas.
- Doze luzes de aviso (farol, cinto, neblina dianteira e traseira, freio, óleo, motor, bateria, ABS, STOP, revisão,
  airbag), fixas ou piscando como o carro manda. O STOP e a revisão (SERVICE) acendem junto com os outros sistemas,
  como no painel original, e um frame que some acende os alertas que o original acenderia
  ([docs/decisoes.md](docs/decisoes.md)).
- As mensagens de texto do painel original no cabeçalho, com a gravidade de cada uma.
- Distância e consumo do trip, guardados entre reinícios (NVS), e o hodômetro.
- Dia e noite: com farol baixo ou alto aceso, tema escuro com o backlight pelo reostato do
  painel (entre 10 e 70 %); senão, tema claro em 100 %.
- Estado do CAN no rodapé: frames por segundo, erros no barramento, bus-off ou silêncio.
- 50 a 61 fps no pior caso (todos os dígitos mudando a cada quadro); firmware de cerca de 0,9 MB.

## Hardware

- [Placa Waveshare ESP32-P4](https://www.waveshare.com/esp32-p4-module-dev-kit.htm) com a tela MIPI-DSI de 8,8" (controlador OTA7290B).
- Um transceptor CAN de 3,3 V ligado ao GPIO3 (TX) e ao GPIO2 (RX), no barramento de 500 kbit/s do carro. O
  controlador fica em modo listen-only.

## Compilação

Precisa do [ESP-IDF 6.0.2](https://docs.espressif.com/projects/esp-idf/en/v6.0.2/esp32p4/get-started/). O
`./idf.sh` carrega o ESP-IDF e chama o `idf.py` (ajuste os caminhos dentro dele para a sua instalação):

```sh
./idf.sh build                         # firmware lendo o CAN do carro
./idf.sh -p /dev/cu.usbmodem… flash    # grava (a porta muda ao trocar de cabo: ls /dev/cu.usbmodem*)
./idf.sh -p /dev/cu.usbmodem… monitor  # log serial (Ctrl+] sai)
```

Na bancada, sem o carro, a tela mostra `--` e "SEM CAN". Para ver dados simulados (os três cenários do design,
alternando a cada 12 s; a cada volta pelos três, troca entre noite e dia), use a variante de simulação, que compila
em `build-sim/`:

```sh
./idf.sh -B build-sim -D SDKCONFIG=build-sim/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.simulation" build flash
```

## No computador

Um simulador de desktop (precisa do SDL2: `brew install sdl2` no macOS) e os testes do CAN, sem a placa:

```sh
cmake -S sim -B sim/build && cmake --build sim/build
sim/build/dashboard-sim                                    # janela com a simulação
sim/build/dashboard-sim --screenshot out.png 1             # cenário 1 (0 normal, 1 alerta, 2 combustível) em PNG
sim/build/dashboard-sim --screenshot out.png 1 light       # o mesmo de dia, no tema claro

cmake -S components/x84_can/test -B components/x84_can/test/build
cmake --build components/x84_can/test/build && components/x84_can/test/build/x84_can_test
```

## Onde está cada coisa

- `components/ui/`: a tela, em LVGL. Só ela desenha; o resto do firmware entrega um `Screen` (`screen.h`) ao
  `Dashboard::show` (`dashboard.h`).
  - `theme.h`: cores (paletas escura e clara), fontes (escala tipográfica), escalas dos medidores e as réguas do
    layout. É onde se muda a aparência.
  - `base.h`: os blocos de montar (caixa, contorno, texto, imagem) e os setters que só mexem no LVGL quando o
    valor muda.
  - `widgets/`: peças reutilizáveis (barras, caixas de valor, luzes de aviso, moldura de alerta, portas, setas).
  - `panels/`: o quadro (cantos, cabeçalho, rodapé) e as três colunas, montados com os widgets.
  - `simulation.cpp`: dados falsos para a bancada e o simulador.
  - `fonts/` e `images/`: gerados por `tools/` (não editar à mão).
- `components/x84_can/`: o CAN do carro, em C++ puro (compila também no computador, para os testes).
  - `frames.h`: posição de cada sinal e um decodificador por frame. `view.cpp`: o que a tela mostra a partir dos
    frames (lâmpadas, avisos, marcha). `trip.h`: distância e combustível do trip.
  - `test/`: os testes, conferidos contra `can/signals-x84.json`.
- `main/`: a placa. `display.cpp` (tela e LVGL), `can.cpp` (TWAI em listen-only), `vehicle.cpp` (frames →
  `Screen`, trip no NVS), `intro.cpp` (fade do backlight), `main.cpp` (liga tudo).
- `sim/`: o simulador de desktop. `tools/`: conversores de fontes e imagens.

## Mudanças comuns

- **Uma cor ou um tamanho de texto:** `components/ui/theme.h`.
- **Um tamanho de fonte novo:** uma linha em `tools/fonts.sh`, rodar o script, declarar em `components/ui/fonts.h`
  e criar o `theme::Font` em `theme.h`.
- **Uma imagem nova:** o SVG em `assets/images/`, depois `cd tools && npm install && node images.mjs`, e declarar em
  `components/ui/images.h`.
- **Mover algo na tela:** as posições estão no topo de cada arquivo de `panels/`, como constantes com nome.
- **Arquivo novo** (`.cpp`, fonte ou imagem): os `CMakeLists.txt` acham os arquivos por pasta, mas só ao configurar.
  Depois de criar um, rode `./idf.sh reconfigure build` (e `cmake -S sim -B sim/build` para o simulador).
- Antes de gravar, confira no simulador: `cmake --build sim/build && sim/build/dashboard-sim --screenshot out.png 2`.

## Editor

O `.clangd` já remove os flags do GCC da placa que o clang não conhece. Para o clangd achar os headers do
compilador da placa, no VS Code acrescente às configurações do usuário
`"clangd.arguments": ["--query-driver=**/riscv32-esp-elf-*"]` e rode um `./idf.sh build` (ele gera
`build/compile_commands.json`).

## Créditos

- [LVGL](https://lvgl.io/) (MIT), [ESP-IDF](https://github.com/espressif/esp-idf) e o pacote de suporte da placa
  Waveshare (Apache 2.0).
- [Nordwand Mono](https://github.com/tywr/Nordwand-Mono) (SIL Open Font License 1.1, `assets/fonts/OFL.txt`); o
  logotipo "MEGANE" é desenhado a partir da [Michroma](https://fonts.google.com/specimen/Michroma) (SIL Open Font
  License).
- Testes: [doctest](https://github.com/doctest/doctest) e [nlohmann/json](https://github.com/nlohmann/json) (MIT).
  Capturas do simulador: [stb](https://github.com/nothings/stb) (domínio público / MIT).

Renault e Mégane são marcas da Renault. Este é um projeto independente, sem vínculo com a Renault nem aprovação
dela.

## Assistência de IA

Este projeto foi codificado com assistência de IA (Claude, da Anthropic). O autor dirigiu o trabalho e o testa no
carro, mas nem toda linha foi escrita ou revisada à mão: leia o código antes de confiar nele e, ao dirigir, não
deixe de lado os avisos do painel original.

## Licença

O código está sob a [licença MIT](LICENSE). Ficam fora dela:

- `can/signals-x84.json`, a lista de sinais do CAN usada como referência nos testes: é derivada de dados de
  terceiros e está aqui só como referência, sem licença concedida.
- As fontes em `assets/fonts/` (e as fontes em bitmap geradas a partir delas) e o logotipo desenhado a partir da
  Michroma: SIL Open Font License 1.1.
