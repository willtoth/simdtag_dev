#include "Halide.h"

using namespace Halide;

extern "C" HALIDE_EXPORT_SYMBOL int32_t __HashMapInsert(void* hashmap, uint32_t hash,
                                                        uint32_t value);

HalideExtern_3(int32_t, __HashMapInsert, void*, uint32_t, uint32_t);

class GradientClusters : public Halide::Generator<GradientClusters> {
   public:
    Input<Buffer<uint8_t, 3>> input{"input"};
    Input<Buffer<int32_t, 3>> labels{"labels"};
    Input<void*> hash_map{"hashmap"};
    Output<Buffer<uint32_t, 3>> gradient_clusters{"gradient_clusters"};
    // Output<Buffer<uint64_t, 1>> output{"output"};
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

        // Func clamped_input = BoundaryConditions::constant_exterior(input, Expr(127));
        // Func clamped_labels = BoundaryConditions::constant_exterior(labels, Expr(127));
        Func clamped_input = BoundaryConditions::repeat_edge(input);
        Func clamped_labels = BoundaryConditions::repeat_edge(labels);

        // {x start, number of elements, y start, number of elements}
        RDom r2{-1, 3, 0, 2};
        r2.where(!(r2.x == -1 && r2.y == 0));

        gradient_clusters(x, y, c) = cast<uint32_t>(0);

        Expr rep0 = cast<uint64_t>(clamped_labels(x, y, 0));
        Expr rep1 = cast<uint64_t>(clamped_labels(x + r2.x, y + r2.y, 0));
        Expr v0 = cast<uint32_t>(clamped_input(x, y, 0));
        Expr v1 = cast<uint32_t>(clamped_input(x + r2.x, y + r2.y, 0));

        Expr hash_value_caldidate1 = (rep0 << 32) | rep1;
        Expr hash_value_caldidate2 = rep0 | (rep1 << 32);
        Expr hash_value =
                (Expr((uint64_t)2654435761) * min(hash_value_caldidate1, hash_value_caldidate2)) >>
                32;
        Expr value =
                cast<uint32_t>(x) << 20 | cast<uint32_t>(y) << 8 | cast<uint32_t>(1 + r2.x + r2.y);
        // Expr value = select(v0 != 127 && v0 + v1 == 255, value_expr, cast<uint32_t>(0));
        r2.where(v0 != 127 && v0 + v1 == 255);
        //__HashMapInsert(hash_map, cast<uint32_t>(hash_value), value);
        //__TEST123(r.x, r.y);

        // DO_CONN(1, 0, 0);
        // DO_CONN(1, 1, 1);
        // DO_CONN(0, 1, 2);
        // DO_CONN(-1, 1, 3);

        // gradient_clusters(x, y, c) = mux(c, {value_0, value_1, value_2, value_3});
        gradient_clusters(x, y, 1 + r2.x + r2.y) =
                cast<uint32_t>(__HashMapInsert(hash_map, cast<uint32_t>(hash_value), value));
        // output(clamp(__TEST123(r.x, r.y), 1, input.width() * input.height() * 4)) = value_0;
    }

    void schedule() {
        if (using_autoscheduler()) {
            input.set_estimates({{640, 1600}, {480, 1200}, {1, 1}});
            labels.set_estimates({{640, 1600}, {480, 1200}, {1, 1}});
            gradient_clusters.set_estimates({{640, 1600}, {480, 1200}, {4, 4}});
            hash_map.set_estimate(nullptr);
            // output.set_estimates({{640 * 480 * 4, 1200 * 1600 * 4}});
        } else {
            // auto pipeline = get_pipeline();
            // Func output = pipeline.get_func(8);
            // Func constant_exterior_1 = pipeline.get_func(7);
            // Func repeat_edge_1 = pipeline.get_func(6);
            // Func lambda_1 = pipeline.get_func(5);
            // Func constant_exterior = pipeline.get_func(3);
            // Func repeat_edge = pipeline.get_func(2);
            // Func lambda_0 = pipeline.get_func(1);
            // Var _0(constant_exterior_1.get_schedule().dims()[0].var);
            // Var _0i("_0i");
            // Var _1(constant_exterior_1.get_schedule().dims()[1].var);
            // Var _2(constant_exterior_1.get_schedule().dims()[2].var);
            // Var c(output.get_schedule().dims()[2].var);
            // Var x(output.get_schedule().dims()[0].var);
            // Var xi("xi");
            // Var y(output.get_schedule().dims()[1].var);
            // Var yi("yi");
            // Var yii("yii");
            // output.split(y, y, yi, 80, TailStrategy::ShiftInwards)
            //         .split(yi, yi, yii, 10, TailStrategy::ShiftInwards)
            //         .split(x, x, xi, 64, TailStrategy::ShiftInwards)
            //         .vectorize(xi)
            //         .compute_root()
            //         .reorder({xi, x, yii, c, yi, y});
            // //.parallel(y);
            // constant_exterior_1.split(_0, _0, _0i, 16, TailStrategy::ShiftInwards)
            //         .vectorize(_0i)
            //         .compute_at(output, yi)
            //         .store_at(output, y)
            //         .reorder({_0i, _0, _1, _2});
            // repeat_edge_1.store_in(MemoryType::Stack)
            //         .split(_0, _0, _0i, 16, TailStrategy::ShiftInwards)
            //         .vectorize(_0i)
            //         .compute_at(constant_exterior_1, _1)
            //         .reorder({_0i, _0, _1, _2});
            // constant_exterior.store_in(MemoryType::Stack)
            //         .split(_0, _0, _0i, 64, TailStrategy::ShiftInwards)
            //         .vectorize(_0i)
            //         .compute_at(output, yi)
            //         .reorder({_0i, _0, _1, _2});
        }
    }
};

HALIDE_REGISTER_GENERATOR(GradientClusters, halide_gradient_clusters)
