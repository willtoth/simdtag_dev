#include "Halide.h"

using namespace Halide;

class AdaptiveThreshold : public Halide::Generator<AdaptiveThreshold> {
   public:
    Input<Buffer<uint8_t, 3>> input{"input"};
    Output<Buffer<uint8_t, 3>> adaptive_threshold{"output"};
    GeneratorParam<int> min_diff{"min_diff", 5};
    GeneratorParam<int> tilesize{"tilesize", 4};

    void generate() {
        Func minMaxTile("minMaxTile");
        Func minMax("minMax");
        Func blur("blur");
        Var x("x"), y("y"), c("c");
        Var xMinMaxTile("xMinMaxTile"), yMinMaxTile("yMinMaxTile"), cMinMaxTile("cMinMaxTile");

        // TODO: Investigate the right boundary condition, this one does not _quite_
        // match the apriltag output.
        Func clamped = BoundaryConditions::repeat_edge(input);

        /******** Min max funtion ********/
        RDom tile(0, tilesize, 0, tilesize, "minMaxTileDomain");
        Expr tileSpace =
                clamped(xMinMaxTile * tilesize + tile.x, yMinMaxTile * tilesize + tile.y, 0);
        RDom blurtile(-1, 3, -1, 3, "blurTile");

        minMaxTile(xMinMaxTile, yMinMaxTile, cMinMaxTile) = {minimum(tileSpace, "myMin"),
                                                             maximum(tileSpace, "myMax")};
        blur(xMinMaxTile, yMinMaxTile, cMinMaxTile) = {
                minimum(minMaxTile(xMinMaxTile + blurtile.x, yMinMaxTile + blurtile.y, 0)[0]),
                maximum(minMaxTile(xMinMaxTile + blurtile.x, yMinMaxTile + blurtile.y, 0)[1])};

        /******** Threshold ************/
        Expr min = blur(x / tilesize, y / tilesize, 0)[0];
        Expr max = blur(x / tilesize, y / tilesize, 0)[1];
        adaptive_threshold(x, y, c) = Halide::cast<uint8_t>(select(
                max - min < min_diff, 127, select(input(x, y, c) > min + (max - min) / 2, 255, 0)));

        /******** Schedule *************/
        // TODO: Try auto-schedule
        Var x_outer, y_outer, x_inner, y_inner;
        adaptive_threshold.tile(x, y, x_outer, y_outer, x_inner, y_inner, 128, 64)
                .vectorize(x_inner, 32);

        // Vectorize by size of reduction domain
        int tmp = tilesize;
        minMaxTile.compute_root().vectorize(xMinMaxTile, tmp * tmp);
        blur.store_root().compute_at(adaptive_threshold, x_outer).vectorize(xMinMaxTile, 9);
    }
};

HALIDE_REGISTER_GENERATOR(AdaptiveThreshold, adaptive_threshold)
