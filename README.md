
# Dalton
<img src="https://samchill.github.io/dalton/images/john_dalton.jpg" alt="John Dalton Portrait" align="right" height="100" />
Dalton is an atomic structure visualizer that supports many different rendering styles. Most notably, it implements a [path tracing] renderer that uses Monte Carlo sampling to provide photorealistic images. Path tracing is able to simulate soft shadows, ambient occlusion and depth of field. In addition to the path tracing renderer, Dalton also includes a much faster "analytic" rendering engine that is able to display approximate ambient occlusion and depth aware outlines.

[path tracing]: https://en.wikipedia.org/wiki/Path_tracing

## Path Tracing
Path tracing is a global illumination rendering algorithm. Images produced using global illumination, include the effects of both direct and indirect light. Direct light is light that only takes once bounce to travel from the light source to the camera, while indirect light is all light that reaches the camera after more than one bounce.

Path tracing uses Monte Carlo sampling to determine the color value of each pixel on the screen. During each rendering pass, a ray is shot from each pixel into the scene. If no atom is hit, then a background color is displayed. If an atom is hit, then the ray scatters in a random direction. The ray continues to scatter until it no longer hits an atom or until the maximum number of bounces is reached. Every time the ray scatters off of an atom it accumulates some of its color. The resulting color from each rendering pass is averaged together to display a final result. This means that initially the image is noisy. However, after several hundred rendering passes a correct image is produced.

Here is an example of the noise decaying as the number of samples increases:
<img src="https://samchill.github.io/dalton/images/sampling.gif" alt="monte carlo sampling" align="center" width="400" />

## Screenshots

<img src="https://samchill.github.io/dalton/images/examples.gif" alt="screenshots" align="center" width="400" />

## Installation
Compile and run like so:

    sudo apt install build-essential cmake xorg-dev libglu1-mesa-dev libboost-filesystem-dev
    git clone https://github.com/SamChill/dalton.git
    cd dalton
    make -j 4
    ./build/dalton
