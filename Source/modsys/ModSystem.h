#pragma once

#include "Parameter.h"
#include "Destination.h"
#include "Modulator.h"
#include "MacroModulator.h"
#include "EnvelopeFollowerModulator.h"
#include "LFOModulator.h"
#include "RandomModulator.h"
#include "PerlinModulator.h"
#include "Matrix.h"
	
/* to do:

implement spline editor modulator
implement formularparser modulator
implement fourier modulator (pitchtracking)
implement midi modulator
	velocity
	note value
	cc
	pitchbend
	modwheel

temposync / free phase mod
	add phase offset parameter (rand)

perlin noise temposync
	make noise buffer fully procedural (deterministic)

dryWetMix needs sqrt(x) and sqrt(1 - x) all the time
currently in vibrato for each channel and sample (bad!)
	solution 1:
		rewrite modulation system to convert to sqrt before smoothing
	solution 2:
		make sqrt lookup table, so modSys can stay the way it is
	solution 3:
		make buffers for both calculations. (2)
		only calculate once per block

rewrite parameter so has functions to give back min and max of range
	then implement maxOctaves in processBlock of perlinMod to get from there

envelopeFollowerModulator
	rewrite db to gain calculation so that it happens before parameter smoothing instead
	add lookahead parameter

lfoModulator
	add pump curve wavetable

randModulator
	try rewrite with spline interpolator instead lowpass

if intellisense is messed up, do:
https://stackoverflow.com/questions/18289936/refreshing-the-auto-complete-intellisense-database-in-visual-studio


*/