#include "Halide.h"

using namespace Halide;

extern "C" HALIDE_EXPORT_SYMBOL uint32_t __CCL_GetLabelCount(uint32_t x);

HalidePureExtern_1(uint32_t, __CCL_GetLabelCount, uint32_t);

class GradientClusters : public Halide::Generator<GradientClusters> {
   public:
    Input<Buffer<uint8_t, 3>> input{"input"};
    Input<Buffer<int32_t, 3>> labels{"labels"};
    Output<Buffer<uint64_t, 3>> gradient_clusters{"output"};
    Var x{"x"}, y{"y"}, c{"c"};

    void generate() {
        // From frc971/orin/apriltag.cc
        // We search the following 4 neighbors.
        //      ________
        //      | x | 0 |
        //  -------------
        //  | 3 | 2 | 1 |
        //  -------------
        //
        //  If connection 3 has the same IDs as the connection between blocks 0 and 2,
        //  we will have a duplicate entry.  Detect and don't add it.  This will only
        //  happen if id(x) == id(0) and id(2) == id(3),
        //         or id(x) == id(2) and id(0) == id(3).

        Func clamped_input = BoundaryConditions::constant_exterior(input, Expr(127));
        Func clamped_labels = BoundaryConditions::constant_exterior(labels, Expr(127));

#define DO_CONN(dx, dy, EXPR_NUM)                                                            \
    Expr value_##EXPR_NUM;                                                                   \
    {                                                                                        \
        Expr rep0 = cast<uint64_t>(clamped_labels(x, y, 0));                                 \
        Expr rep1 = cast<uint64_t>(clamped_labels(x + dx, y + dy, 0));                       \
        Expr v0 = cast<uint64_t>(clamped_input(x, y, 0));                                    \
        Expr v1 = cast<uint64_t>(clamped_input(x + dx, y + dy, 0));                          \
                                                                                             \
        Expr hash_value_caldidate1 = rep0 << 20 | rep1;                                      \
        Expr hash_value_caldidate2 = rep0 | rep1 << 20;                                      \
        Expr hash_value = min(hash_value_caldidate1, hash_value_caldidate2) << 24;           \
        Expr value_expr =                                                                    \
                hash_value | cast<uint64_t>(x) << 13 | cast<uint64_t>(y) << 2 | EXPR_NUM;    \
        Expr label_count = __CCL_GetLabelCount(cast<uint32_t>(rep0));                        \
        Expr label_count2 = __CCL_GetLabelCount(cast<uint32_t>(rep1));                       \
        value_##EXPR_NUM = select(v0 + v1 == 255 && label_count >= 25 && label_count2 >= 25, \
                                  value_expr, cast<uint64_t>(0));                            \
    }
        DO_CONN(1, 0, 0);
        DO_CONN(1, 1, 1);
        DO_CONN(0, 1, 2);
        DO_CONN(-1, 1, 3);
#undef DO_CONN
        gradient_clusters(x, y, c) = mux(c, {value_0, value_1, value_2, value_3});

        gradient_clusters.compute_root();
    }
};

HALIDE_REGISTER_GENERATOR(GradientClusters, halide_gradient_clusters)
