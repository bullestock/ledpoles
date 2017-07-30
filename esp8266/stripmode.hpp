#pragma once

enum class StripMode
{
    // Use the whole strip for the animation
    WholeStrip,
    First = WholeStrip,
    // Use one pole for the animation, and copy to remaining poles
    OnePoleCopy,
    // Use one pole for the animation, and shift-copy to remaining poles
    OnePoleShiftCopy,
    Last
};

StripMode get_strip_mode();

void set_strip_mode(StripMode mode);
