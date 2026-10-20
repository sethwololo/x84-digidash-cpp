#include "view.h"

namespace x84 {

Severity severity(Notice notice)
{
    switch (notice) {
    case Notice::severe_injection_fault:
        return Severity::fault;
    case Notice::hands_free_fault:
    case Notice::card_reader_fault:
    case Notice::restricted_card_mode:
    case Notice::card_battery_low:
    case Notice::column_not_locked:
    case Notice::column_not_unlocked:
    case Notice::child_lock_fault:
    case Notice::wiper_fault:
    case Notice::injection_fault:
        return Severity::service;
    default:
        return Severity::info;
    }
}

} // namespace x84
