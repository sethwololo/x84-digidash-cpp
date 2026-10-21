#include "frame.h"

#include <string>

#include "base.h"
#include "images.h"
#include "theme.h"
#include "widgets/decorations.h"

namespace ui {

namespace {

using theme::CORNER_INSET;
using theme::MARGIN;

// The header's texts are centered on the wordmark, whose bottom sits 25 px above the header rule.
constexpr int32_t WORDMARK_Y = theme::HEADER_RULE - 25;
constexpr int32_t FOOTER_Y = theme::FOOTER_RULE + 11;

// The top of a text box in `font` centered on the wordmark.
int32_t centered_on_wordmark(const theme::Font &font)
{
    return WORDMARK_Y + (img_wordmark.header.h - static_cast<int32_t>(theme::LINE_HEIGHT * font.size)) / 2;
}

const char *notice_text(x84::Notice notice)
{
    switch (notice) {
    case x84::Notice::welcome: return "BEM-VINDO";
    case x84::Notice::insert_card: return "INSIRA O CARTÃO";
    case x84::Notice::card_not_detected: return "CARTÃO NÃO DETECTADO";
    case x84::Notice::hands_free_fault: return "FALHA MÃOS LIVRES";
    case x84::Notice::press_stop_twice: return "PRESSIONE STOP 2 VEZES";
    case x84::Notice::card_reader_fault: return "FALHA NO LEITOR DO CARTÃO";
    case x84::Notice::brake_and_start: return "FREIO + START";
    case x84::Notice::remove_card: return "RETIRE O CARTÃO";
    case x84::Notice::restricted_card_mode: return "MODO CARTÃO RESTRITO";
    case x84::Notice::brake_and_start_or_clutch: return "FREIO + START OU EMBREAGEM";
    case x84::Notice::neutral_or_clutch: return "PONTO MORTO OU EMBREAGEM";
    case x84::Notice::trunk_open_insert_card: return "PORTA-MALAS ABERTO - INSIRA O CARTÃO";
    case x84::Notice::confirm_engine_stop: return "CONFIRME DESLIGAR O MOTOR";
    case x84::Notice::starting: return "PARTIDA EM CURSO";
    case x84::Notice::pedal_not_held: return "PEDAL NÃO MANTIDO";
    case x84::Notice::card_battery_low: return "PILHA DO CARTÃO FRACA";
    case x84::Notice::column_not_locked: return "COLUNA NÃO TRAVADA";
    case x84::Notice::column_not_unlocked: return "COLUNA NÃO DESTRAVADA";
    case x84::Notice::child_lock_fault: return "FALHA TRAVA DE CRIANÇA";
    case x84::Notice::wiper_fault: return "FALHA LIMPADOR";
    case x84::Notice::injection_fault: return "VERIFICAR INJEÇÃO";
    case x84::Notice::severe_injection_fault: return "FALHA GRAVE INJEÇÃO";
    case x84::Notice::check_steering: return "VERIFICAR DIREÇÃO";
    case x84::Notice::steering_fault: return "FALHA NA DIREÇÃO";
    case x84::Notice::electronic_fault: return "FALHA ELETRÔNICA";
    }
    return "";
}

lv_color_t notice_color(x84::Severity severity)
{
    switch (severity) {
    case x84::Severity::fault: return theme::palette().fault;
    case x84::Severity::service: return theme::palette().service;
    case x84::Severity::info: return theme::palette().value;
    }
    return theme::palette().value;
}

} // namespace

Frame::Frame(lv_obj_t *screen)
{
    const int32_t right = theme::SCREEN_WIDTH - CORNER_INSET - 24;
    const int32_t bottom = theme::SCREEN_HEIGHT - CORNER_INSET - 24;
    corner(screen, CORNER_INSET, CORNER_INSET, false, false);
    corner(screen, right, CORNER_INSET, true, false);
    corner(screen, CORNER_INSET, bottom, false, true);
    corner(screen, right, bottom, true, true);

    // Header
    lv_obj_t *wordmark = image(screen, &img_wordmark, theme::palette().value, MARGIN, WORDMARK_Y);
    spaced(text(screen, theme::TEXT_SMALL, theme::palette().label, MARGIN + img_wordmark.header.w + 13,
                centered_on_wordmark(theme::TEXT_SMALL), "// K4M 16V"),
           theme::SPACING_LABEL);
    const int32_t status_y = text_y(theme::TEXT_LABEL_BOLD, centered_on_wordmark(theme::TEXT_LABEL_BOLD));
    notice_ = spaced(text(screen, theme::TEXT_LABEL_BOLD, theme::palette().value, 0, 0), theme::SPACING_LABEL);
    lv_obj_align(notice_, LV_ALIGN_TOP_MID, 0, status_y);
    status_ = spaced(text(screen, theme::TEXT_LABEL_BOLD, theme::palette().label, 0, 0, "SISTEMA OK"), theme::SPACING_LABEL);
    lv_obj_align(status_, LV_ALIGN_TOP_RIGHT, -MARGIN, status_y);

    const int32_t rule_width = theme::SCREEN_WIDTH - 2 * MARGIN;
    fill(screen, MARGIN, theme::HEADER_RULE, rule_width, 1, theme::palette().border);
    fill(screen, MARGIN, theme::FOOTER_RULE, rule_width, 1, theme::palette().border);

    // Footer
    spaced(text(screen, theme::TEXT_SMALL, theme::palette().border, MARGIN, FOOTER_Y, "SN // MG-2026-0313"), theme::SPACING_CAPTION);
    can_ = spaced(text(screen, theme::TEXT_SMALL, theme::palette().border, 0, 0), theme::SPACING_CAPTION);
    lv_obj_align(can_, LV_ALIGN_TOP_MID, 0, text_y(theme::TEXT_SMALL, FOOTER_Y));
    firmware_ = spaced(text(screen, theme::TEXT_SMALL, theme::palette().border, 0, 0), theme::SPACING_CAPTION);
    lv_obj_align(firmware_, LV_ALIGN_TOP_RIGHT, -MARGIN, text_y(theme::TEXT_SMALL, FOOTER_Y));
}

void Frame::show(const Screen &screen, bool alert)
{
    const x84::View &view = screen.view;

    set_text(notice_, view.notice ? notice_text(*view.notice) : "");
    if (view.notice) set_text_color(notice_, notice_color(x84::severity(*view.notice)));

    set_text(status_, !view.online ? "SEM CAN" : alert ? "ALERTA" : "SISTEMA OK");
    set_text_color(status_, alert || !view.online ? theme::palette().fault : theme::palette().label);

    switch (screen.can_state) {
    case CanState::ok: set_text(can_, ("CAN // " + std::to_string(screen.can_rate) + " FR/S").c_str()); break;
    case CanState::errors: set_text(can_, "CAN // ERROS"); break;
    case CanState::bus_off: set_text(can_, "CAN // BUS-OFF"); break;
    case CanState::silent: set_text(can_, "CAN // SEM SINAL"); break;
    }
    set_text_color(can_, screen.can_state == CanState::ok ? theme::palette().border : theme::palette().fault);

    set_text(firmware_, (std::string("SLN // FIRMWARE ") + screen.firmware_version).c_str());
}

} // namespace ui
