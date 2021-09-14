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
	* 
	* signalsmith:
	* "if you're slowing down (a.k.a. upsampling) you need to lowpass at your original signal's Nyquist
	* but when you're speeding up, you need to lowpass at the new Nyquist."
	* 
	* temposync / free phase mod
	*	add phase offset parameter (rand)
	* 
	* perlin noise temposync
	*	even needed?
	*	only phasors atm
	* 
	* envelope follower mod makes no sense on [-1,1] range
	*	current solution: it goes [0,1] only
	* 
	* perlin noise modulator doesn't compensate for spline overshoot anymore (* .8)
	* 
	* dryWetMix needs sqrt(x) and sqrt(1 - x) all the time
	* currently in vibrato for each channel and sample (bad!)
	*	solution 1:
	*		rewrite modulation system to convert to sqrt before smoothing
	*	solution 2:
	*		make sqrt lookup table, so modSys can stay the way it is
	*	solution 3:
	*		make buffers for both calculations. (2)
			only calculate once per block
	* 
	* all destinations have: bias / weight
	*
	* rewrite parameter so has functions to give back min and max of range
	*	then implement maxOctaves in processBlock of perlinMod to get from there
	* 
	* envelopeFollowerModulator
	*	rewrite db to gain calculation so that it happens before parameter smoothing instead
	*	add lookahead parameter
	*
	* lfoModulator
	*	add pump curve wavetable
	*
	* randModulator
	*	try rewrite with spline interpolator instead lowpass
	* 
	* perlinModulator
	*	add method for calculating autogain spline overshoot
	* 
	* all wavy mods (especially temposync ones)
	*	phase parameter
	* 
	* if intellisense is messed up, do:
	* https://stackoverflow.com/questions/18289936/refreshing-the-auto-complete-intellisense-database-in-visual-studio
	*/