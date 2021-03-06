/*
 * FastFourierTransform.h
 *
 *  Created on: 26.03.2012
 *      Author: michi
 */

#ifndef FASTFOURIERTRANSFORM_H_
#define FASTFOURIERTRANSFORM_H_

#include "../lib/file/file.h"
#include "../lib/math/math.h"

namespace FastFourierTransform
{
	void _cdecl fft_c2c(Array<complex> &in, Array<complex> &out, bool inverse);
	void _cdecl fft_c2c_michi(Array<complex> &in, Array<complex> &out, bool inverse);
	void _cdecl fft_r2c(Array<float> &in, Array<complex> &out);
	void _cdecl fft_c2r_inv(Array<complex> &in, Array<float> &out);
}

#endif /* FASTFOURIERTRANSFORM_H_ */
