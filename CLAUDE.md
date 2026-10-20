# x84-digidash-cpp

Painel digital do Renault Mégane II (X84) em C++20 + LVGL 9.5 sobre ESP-IDF 6.0.2, numa Waveshare ESP32-P4 com
tela DSI de 8,8" (480x1920, OTA7290B) usada em paisagem. Port do projeto Rust + Slint em `~/dev/rust/x84-digidash`,
que é a referência de layout (`ui/*.slint`) e de regras do CAN (`x84-can/`, `docs/can-plan.md`). Comandos e
estrutura: `README.pt-BR.md` (em inglês: `README.md`).

## Convenções

- Textos da UI em português; identificadores e comentários em inglês. Vírgula decimal: o CAN entrega décimos e a
  UI formata (`ui::tenths`).
- Valores que o carro pode não mandar são `std::optional`: vazio mostra `--`.
- Só `components/ui` chama o LVGL; o firmware entrega um `ui::Screen` ao `Dashboard::show` a cada 10 ms, dentro da
  task do LVGL (um `lv_timer`). Fora dela, segure `display::Lock`.
- Commits pequenos e coesos, um assunto cada, com o porquê na mensagem.
- O README avisa que o código foi feito com assistência de IA; mantenha o aviso nas duas línguas.

## Armadilhas

- **Setters que não mudam nada:** o dashboard escreve todos os valores a cada atualização; `base.h` só toca o LVGL
  quando o valor muda (cada escrita repinta o objeto). Use sempre `ui::set_*`, não `lv_obj_set_*`, nos `show`.
- **Texto pela linha de base:** o `lv_font_conv` calcula a altura da linha só com os glifos da fonte (a de 80 px,
  só dígitos, tem 58 px de linha). `ui::text` recebe o topo da caixa de 1,3 em do design (as coordenadas do
  Slint) e posiciona pela linha de base lida da fonte; `ui::center_text` idem para texto centralizado.
- **Filhos são recortados pelo pai,** mesmo com `LV_OBJ_FLAG_OVERFLOW_VISIBLE` (só estende ao `ext_draw_size`): o
  que sai de um contorno (as portas do carro) é desenhado ao lado, não dentro. As listras de perigo (10x10) ficam
  dentro da borda da moldura de alerta, e o rótulo dela (RESERVA) termina antes delas.
- **Fontes por tamanho e conjunto de glifos:** `tools/fonts.sh` (Nordwand Mono). Texto com ASCII + maiúsculas
  acentuadas + `°` (a Nordwand não tem `·`); 80/190 px só dígitos (o de 190 px completo custaria ~1,5 MB).
- **Imagens A8** (`tools/images.mjs`): só opacidade, a cor vem de `image_recolor` (`ui::image`).
- **Desenho só por software:** o PPA do LVGL (`CONFIG_LV_USE_PPA`) roda ao lado da unidade de software e
  sincroniza o cache em linhas inteiras de 128 B, apagando pixels que ela desenhava ao lado (pedaços de letras
  sumiam na placa, nunca no simulador): desligado. O do `esp_lvgl_adapter` (`enable_ppa_accel`) desalinha buffers
  e aborta: desligado. Só a rotação de 90° usa o PPA, sobre buffers prontos (`LV_DRAW_BUF_ALIGN=128`).
- **Versões:** LVGL fixo em 9.5 (`main/idf_component.yml`); a 9.6 deprecia headers que o adapter 0.6 inclui.
- **Display:** o BSP só cria o painel (`bsp_display_new`); `display.cpp` registra no adapter (o
  `bsp_display_start` desliga o PPA). Modo `TRIPLE_PARTIAL` com rotação: 3 framebuffers
  (`CONFIG_BSP_LCD_DPI_BUFFER_NUMS=3`).
- **CAN:** driver novo (`esp_twai_onchip`), que calcula o bitrate a partir do clock real (o `B500K` do esp-idf-hal
  errava na P4). A interrupção só copia o último frame de cada ID; `vehicle.cpp` decodifica, e um frame curto
  demais mantém o anterior. As chaves de NVS do trip são as do firmware Rust.
- **Serial:** a porta da UART (`/dev/cu.usbmodem5B61…`) aceita o reset do esptool; não force DTR/RTS na porta
  USB-JTAG da P4 (prende o chip no modo de download).

## Dia e noite

- `View::night` = farol baixo ou alto aceso (o estado real, de `0x625`, não o do autoteste das lâmpadas). Noite:
  backlight 70 % e paleta escura; dia (farol desligado ou só lanternas): 100 % e paleta clara. O reostato do
  painel original (`0x645`) não acompanhou no carro e não é usado.
- `theme.h` tem as duas paletas (`DARK`, `LIGHT`) com os mesmos papéis de cor; os widgets leem
  `theme::palette()`. Na troca, `Dashboard` reconstrói a tela inteira na outra paleta.
- Paleta clara: fundo cinza-claro (`#e6e6e6`) e acentos saturados (um canal cheio, outro zerado). Tons escuros, que
  dão mais contraste no papel, viraram um marrom indistinto no painel; o contraste de cada um está anotado.
- A simulação alterna noite e dia a cada volta pelos três cenários; `--screenshot out.png N light` captura de dia.

## Desempenho

- Simulação de pior caso (RPM e velocidade mudando a cada quadro): 51 a 61 fps, 10 a 16 ms por quadro (render +
  rotação + cópia). O Rust/Slint fazia cerca de 30 fps no mesmo cenário.
- Firmware: cerca de 0,9 MB (89 % da partição do app livre).
- Medido sem ganho (não repetir): buffers de desenho na RAM interna em vez da PSRAM (os de 50 linhas já cabem no
  cache L2) e `LV_ATTRIBUTE_FAST_MEM_USE_IRAM` (não compila no LVGL 9.5 e, contornado, não cabe na IRAM).

## Próximos passos

1. Conferido no carro: mensagens do cabeçalho, marcha, cinto, trip (incluindo a gravação). Falta: polaridade da luz
   de óleo, MOTOR e REG/LIM contra o painel original, o brilho e o tema pelos faróis, a legibilidade do tema
   claro sob o sol.
2. O log de FPS só aparece quando há o que redesenhar (tela parada não gera quadros).
