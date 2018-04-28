#include "chasecolours.hpp"

int ChaseColours::idx = 0;
const CRGB ChaseColours::chase_colours[] = {
    CRGB::Yellow, CRGB::Green, CRGB::HotPink, CRGB::Blue, CRGB::Red, CRGB::White
};

void ChaseColours::next()
{
    ++idx;
    if (idx >= sizeof(chase_colours)/sizeof(CRGB))
        idx = 0;
}
