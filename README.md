# Simdtag

In its current form, this project is an exploritory effort to speed up [apriltags](https://github.com/AprilRobotics/apriltag), specifically for lower cost hardware. For a higher performance CUDA based implementation, look at the approach taken by [frc971](https://github.com/frc971/971-Robot-Code/blob/main/frc971/orin/apriltag.cc).

Apriltag is chosen over cv::aruco as it seems to perform better for pose stability, especially at longer distances.

Three strategies for speeding things up:
1. Use [Halide](https://halide-lang.org/) for some operations. Unfortunately there is a lot that cannot be representated in Halide, but it is good for operations such as thresholding. It also has built in support for threading and vectorization.
2. Find opportunities to use different algorithms in a few spots. The first one is Connected Components Labeling (ccl) which uses [Bit-Merge-Run Scan](https://iris.unimore.it/retrieve/handle/11380/1247510/360133/2021_IVPR_Fast_Run_Based_Connected_Components_Labeling_for_Bitonal_Images.pdf) (BMRS) from [YACCLAB](https://github.com/prittt/YACCLAB) instead of the apriltag union find implementation. BMRS is a good fit for vectorization, and shows considerable performance even compared to spahgetti labeling, since the image can be turned into a binary image efficiently with vector operations. Another is changing out sort with a vector sort, we will see how this one pans out, but it is [claimed](https://opensource.googleblog.com/2022/06/Vectorized%20and%20performance%20portable%20Quicksort.html) to be about 10x faster than `std::sort`, though AVX-512 provides the best speedup.
3. Explicit vectorization (SIMD). Since SIMD is unique to every specific chip, to attempt portability [Google Highway](https://github.com/google/highway) is used. This library support dynamic dispatch of SIMD instructions based on platform. This is not enabled at this point.

Near term goals:

- Speed up _single threaded_ performance of the Apriltag alorithm by at least 4x on platforms with support for AVX2 instructions (basically all modern x86). Target platform is Intel N100, and will be tested on the [Radxa X4](https://radxa.com/products/x/x4/) which is less than $100.
- Export apriltag API as a C layer to allow LD_PRELOAD against existing solutions today, i.e. [PhotonVision](https://photonvision.org/)
- Length agnostic SIMD width to enable more platforms
- Basic test support against AVX2 instructions
- Basic experiements on ARM just to see what happens

Medium term goals:

- (Re)Introduce threading to further improve performance.
- Enable some mechanism to allow multi-target builds. Highway has support for this, just need to enable it and test. Any large performance hits will not be accepted.
- Replace OpenCV with Highway's image library?
- Organize things better and refactor a bit

Current Non-goals:

- Decimation. High quality calibraiton plus high resolution allows detection at greater ranges. Our goal is to use 1600x1200 full resolution with no decimation at 60FPS. If someone really wants it, it would be easy to add.
- GPU acceleration. There are some lower cost GPUs that could perhaps benefit the overall algorithm.
- Direct integration into any particular project. Not that it shouldn't be done, but it currently will require compiling on the target system, which is a no-go other than pre-compiled hosts like Orange Pi 5 or Raspberry Pi. It also hard codes things and is not stable.

Some other technical notes:

- Compilers are able to generate and auto-vectorize portions of code, handle loop unrolling etc. It would be nice to be able to write normal C/C++ code and let the compiler deal with it. Unfortunately its not so simple all of the time. One example is an iteration of some of the algorithm, which used templates to define the width and height. The compiler did a great job of loop unrolling and some vectorization. However it was not always the case, and upon inspection of generated assembly, it was clear that some sections I had expected to be vectorized were not. So explicit vectorization was chosen. Another advantage of that approach is the tendency to write the code specifically for SIMD. This includes things like extending to be aligned to the SIMD width, or doing more conditional logic as mask computations instead.
- There are a few locations where memory locality plays a much more critical role than pure compute. There is already a decent amount of work that was already done by apriltag to optimize here. That means some possible performance gains are more difficult in these areas.

## Building

### Requirements

- cmake
- clang (18.0 used in tests)
- opencv
- highway
- halide

`sudo apt install cmake build-essential clang clang-18 libopencv-dev`

```
git clone https://github.com/google/highway.git
git checkout 1.2.0
cd highway
mkdir build && cd build
cmake -DHWY_ENABLE_TESTS=OFF -DHWY_ENABLE_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

# Spots that could likely use more attention

- gradient clusters - hash + vector could likely use tuning?
- Array + sort vs hash --> Hash seems faster on narrower targets, haven't done full profiling on array as a result
- Fit_Quads step for sorting allocates an array, copies data over, then sorts, is this faster than other options? Look at either keeping it in 64-bit from the start, or x86-simd-sort has a 2 array sort

## Credit

[Apriltag]()
[YACCLAB]()
[FRC971]()
