#pragma once
#include <cstdint>

struct RGB {
	double r, g, b;
};

// Convert color temperature (Kelvin, 1000-7500) to
// R:G:B gamma multipliers for xrandr.
// Uses the Tanner Helland algorithm (same family as redshift/f.lux).
RGB kelvin_to_rgb(uint16_t kelvin);

// Estimate color temperature from gamma R:G:B values.
// Returns 6500 (neutral) if within 5% of 1:1:1.
uint16_t estimate_kelvin(double r, double g, double b);
