
# Dalton
<img src="https://samchill.github.io/dalton/images/john_dalton.jpg" alt="John Dalton Portrait" align="right" height="100" />
Dalton is an atomic structure visualizer that support many different rendering styles. Most notably, it implements a [path tracing][1] render that uses Monte Carlo sampling to provide global illumination. Path tracing is able to simulate soft shadows, ambient occlusion and depth of field. In addition to the path tracing renderer Dalton also includes a much faster "analytic" rendering engine that is able to display approximate ambient occlusion.

[1]: https://en.wikipedia.org/wiki/Path_tracing

## Screenshots

![screenshots](https://samchill.github.io/dalton/images/examples.gif)

## Installation
Compile and run like so:

    sudo apt install build-essential cmake xorg-dev libglu1-mesa-dev libboost-filesystem-dev
    git clone https://github.com/SamChill/dalton.git
    cd dalton
    make -j 4
    ./build/dalton
