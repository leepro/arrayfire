/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <arrayfire.h>
#include <cstdlib>

#include "input.h"
using namespace af;

// The following function is a modified version of http://www.johndcook.com/blog/cpp_phi/
// The example above references Handbook of Mathematical Functions by Abramowitz and Stegun

array cnd(array x)
{
    // constants
    const float a1 =  0.254829592;
    const float a2 = -0.284496736;
    const float a3 =  1.421413741;
    const float a4 = -1.453152027;
    const float a5 =  1.061405429;
    const float p  =  0.3275911;
    const float sqrt2 = sqrt(2.0);

    // Save the sign of x
    array xSign = sign(x);

    x = abs(x) / sqrt2;

    // A&S formula 7.1.26
    array t = 1.0f / (1.0f + p*x);
    array y = 1.0f + 0.5f * (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x*x);

    return xSign * y + !xSign * (1 - y); // equivalent of (x >= 0) ? y : (1 - y);
}

static void black_scholes(array& C, array& P,
                          const array& S, const array& X,
                          const array& R, const array& V,
                          const array& T)
{
    // This function computes the call and put option prices based on
    // Black-Scholes Model

    // S = Underlying stock price
    // X = Strike Price
    // R = Risk free rate of interest
    // V = Volatility
    // T = Time to maturity

    array d1 = log(S / X);
    d1 = d1 + (R + (V*V)*0.5) * T;
    d1 = d1 / (V*sqrt(T));

    array d2 = d1 - (V*sqrt(T));

    array cnd_d1 = cnd(d1);
    array cnd_d2 = cnd(d2);

    C = S * cnd_d1  - (X * exp((-R)*T) * cnd_d2);
    P = X * exp((-R)*T) * (1 - cnd_d2) - (S * (1 - cnd_d1));
}

int main(int argc, char **argv)
{

    try {
        int device = argc > 1 ? atoi(argv[1]) : 0;
        setDevice(device);
        info();

        printf("** ArrayFire Black-Scholes Example **\n"
               "**          by AccelerEyes         **\n\n");

        array GC1(4000, 1, C1);
        array GC2(4000, 1, C2);
        array GC3(4000, 1, C3);
        array GC4(4000, 1, C4);
        array GC5(4000, 1, C5);


        // Compile kernels
        // Create GPU copies of the data
        array Sg = GC1;
        array Xg = GC2;
        array Rg = GC3;
        array Vg = GC4;
        array Tg = GC5;
        array Cg, Pg;

        // Warm up black scholes example
        black_scholes(Cg, Pg, Sg,Xg,Rg,Vg,Tg);
        eval(Cg, Pg);
        printf("Warming up done\n");
        af::sync();


        int iter = 1000;
        for (int n = 50; n <= 500; n += 50) {

            // Create GPU copies of the data
            Sg = tile(GC1, n, 1);
            Xg = tile(GC2, n, 1);
            Rg = tile(GC3, n, 1);
            Vg = tile(GC4, n, 1);
            Tg = tile(GC5, n, 1);
            af::eval(Sg, Xg, Rg, Vg, Tg);

            dim4 dims = Xg.dims();
            // Force compute on the GPU
            af::sync();

            timer::start();
            for (int i = 0; i < iter; i++) {
                black_scholes(Cg, Pg, Sg,Xg,Rg,Vg,Tg);
                eval(Cg, Pg);
            }
            af::sync();

            double t = timer::stop() / iter;
            printf("Input Data Size = %8d. Mean GPU Time: %0.6f ms\n", (int)dims[0], 1000 * t);
        }
    } catch (af::exception& e){
        fprintf(stderr, "%s\n", e.what());
        throw;
    }

    return 0;
}
