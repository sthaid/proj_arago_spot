# Arago Spot

In this project I investigated the Arago Spot, 
using a software simulation and optical apparatus.

[Wikipedia Arago Spot](https://en.wikipedia.org/wiki/Arago_spot)

## History

Prior to the early 1800s light was considered to be a particle, called a corpuscle.

In 1807 Thomas Young published his double slit experiment, which showed a diffraction pattern
that provided evidence that light was actually a wave. 

In 1818 the Arago Spot experiment was performed.
The intent of the experiment was to show that the Arago spot did not exist, and thus
refute the theory that light is a wave. However, the Arago spot was observed, which
convinced scientists that light is a wave and not a particle.

In 1905 Einstein's showed that light is both a wave and a particle in the Photoelectric experiment.
It wasn't until 1926 that the word Photon was first used.

## Simulation Overview

A few years ago I wrote a program to simulate diffraction in an interferometer or aperture
such as a double slit. This program ray traced photons emitted from an aperture.
This program worked well for apertures with small areas.
When I tried this program using a ring aperture, to simulate the Arago spot, 
the program generated the correct result; but took 100 hours to generate a good image.
 
![ifsim arago spot](/assets/ifsim_arago_spot_4_8.png)

When reading about the Arago spot I found the
[Angular Spectrum Method](https://en.wikipedia.org/wiki/Angular_spectrum_method)
is an efficient technique for modeling the propagation a wave field. The steps are:
* Generate the wave field for the aperture.
* Take 2D FFT of the wave field.
* Multiply each point of the 2D FFT by a propagation term.
* Take 2D inverse FFT, this is the propagated wave field.

The following references were used:
* https://rafael-fuente.github.io/simulating-diffraction-patterns-with-the-angular-spectrum-method-and-python.html
* https://github.com/rafael-fuente/diffractsim
* https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.674.3630&rep=rep1&type=pdf

The core code of this simulation program is in angular_spectrum_method.c file.
This code computes the propagation of a 5000x5000 array in about 5 seconds.
```
void angular_spectrum_method_execute(double wavelen, double z)
{
    fftw_execute(fwd);
    multiply_by_propagation_term(wavelen, z);
    fftw_execute(back);
}

static void multiply_by_propagation_term(double wavelen, double z)
{
    double kx, ky, kz;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            kx = (2*M_PI/TOTAL_SIZE) * (i <= N/2-1 ? i : i-N);
            ky = (2*M_PI/TOTAL_SIZE) * (j <= N/2-1 ? j : j-N);
            kz = csqrt( square(2*M_PI/wavelen) - square(kx) - square(ky) );
            buff[i][j] = buff[i][j] * cexp(I * kz * z);
        }
    }
}
```

I had some difficulty getting this to work because I did not understand the 
sample frequency of an FFT. This python code demonstrates the sample frequencies:
```
  >>> x = np.fft.fftfreq(10,1)
  >>> x
  array([ 0. ,  0.1,  0.2,  0.3,  0.4, -0.5, -0.4, -0.3, -0.2, -0.1])
```
Screenshot from simulation:
![screenshot.png](/assets/sim.png)

## Simulation Program Usage

Apertures are defined in file aperture_defs.

Controls:
* Select aperture by clicking on its name at the top right.
* Use left and right arrows to adjust the distance to the screen (Z).
* Use up and down arrows to adjust wavelength.
* Clicking AUTO_INIT_START will cause the program to compute the propagation for each aperture at Z distances ranging from 0 to 6000 mm, and for wavelengths ranging from 400 nm to 750 nm (purple to red). The results are stored in the saved_results directory, and uses 6.9G of disk space. 3660 files are created. This takes several hours to complete.
* Clicking ALG selects the algorithm used to convert from wave amplitude to pixel intensity; 0=logarithmic 1=linear.

## Optical Apparatus

The apparatus consists of a laser pointer, a double convex lens and a circular disk. 
The distance from the circular disk to the screen is 3 meters.
The circular disk is 6 mm diameter map tack pin.
The lens is double convex 50mm focal length.

3D printed mounts were used to hold the laser pointer, the lens and the map pin.
These rest on top of a 2 inch by 2 inch by 1/8 inch mill finish aluminum angle.

Links to parts:
* [Map Tacks](https://www.amazon.com/dp/B06W56RVT3?psc=1&ref=ppx_yo2_dt_b_product_details)
* [50mm Focal Length Lens](https://www.amazon.com/dp/B01F9KXRX2?psc=1&ref=ppx_yo2_dt_b_product_details)
* [Aluminum Angle](https://www.amazon.com/dp/B000EUGY24?psc=1&ref=ppx_yo2_dt_b_product_details)

Photos of apparatus:
![apparatus](/assets/apparatus.jpg)
![disk](/assets/apparatus_disk.jpg)
![screen](/assets/apparatus_screen_3000mm.jpg)

## Mounts CAD

[mounts](https://www.tinkercad.com/things/am9T5TBHxlz-copy-of-sizzling-wolt/edit)

![mounts](/assets/tinkercad.png)

