#pragma once

#include <fmt/format.h>

#include "apriltag.h"
#include "common/image_u8x3.h"
#include "common/unionfind.h"

struct pt {
    // Note: these represent 2*actual value.
    uint16_t x, y;
    int16_t gx, gy;

    float slope;
};

apriltag_detector_t *DefaultApriltagDetector() {
    apriltag_detector_t *td = apriltag_detector_create();
    td->quad_decimate = 1.0;
    td->quad_sigma = 0.0;
    td->nthreads = 1.0;
    td->debug = 0;
    td->refine_edges = 1;
    td->wp = workerpool_create(td->nthreads);

    return td;
}

void WriteConnectedComponents(apriltag_detector_t *td, int w, int h, unionfind_t *uf) {
    image_u8x3_t *d = image_u8x3_create(w, h);

    uint32_t *colors = (uint32_t *)calloc(w * h, sizeof(*colors));

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint32_t v = unionfind_get_representative(uf, y * w + x);

            if ((int)unionfind_get_set_size(uf, v) < td->qtp.min_cluster_pixels) continue;

            uint32_t color = colors[v];
            uint8_t r = color >> 16, g = color >> 8, b = color;

            if (color == 0) {
                const int bias = 50;
                r = bias + (random() % (200 - bias));
                g = bias + (random() % (200 - bias));
                b = bias + (random() % (200 - bias));
                colors[v] = (r << 16) | (g << 8) | b;
            }

            d->buf[y * d->stride + 3 * x + 0] = r;
            d->buf[y * d->stride + 3 * x + 1] = g;
            d->buf[y * d->stride + 3 * x + 2] = b;
        }
    }

    free(colors);

    image_u8x3_write_pnm(d, "debug_segmentation.pnm");
    image_u8x3_destroy(d);
}

void PrintGradientClusters(zarray_t *clusters) {
    for (int i = 0; i < zarray_size(clusters); i++) {
        zarray_t *cluster;
        zarray_get(clusters, i, &cluster);

        fmt::print("\n\n====={{");
        for (int j = 0; j < zarray_size(cluster); j++) {
            struct pt *p;
            zarray_get_volatile(cluster, j, &p);
            fmt::print("{{{},{},{},{}}}, ", (float)p->x / 2.0, (float)p->y / 2.0, p->gx, p->gy);
        }
        fmt::println("}}====\n\n");
    }
}

void WriteGradientClusters(int w, int h, zarray_t *clusters) {
    image_u8x3_t *d = image_u8x3_create(w, h);

    for (int i = 0; i < zarray_size(clusters); i++) {
        zarray_t *cluster;
        zarray_get(clusters, i, &cluster);

        uint32_t r, g, b;

        if (1) {
            const int bias = 50;
            r = bias + (random() % (200 - bias));
            g = bias + (random() % (200 - bias));
            b = bias + (random() % (200 - bias));
        }

        for (int j = 0; j < zarray_size(cluster); j++) {
            struct pt *p;
            zarray_get_volatile(cluster, j, &p);

            int x = p->x / 2;
            int y = p->y / 2;
            d->buf[y * d->stride + 3 * x + 0] = r;
            d->buf[y * d->stride + 3 * x + 1] = g;
            d->buf[y * d->stride + 3 * x + 2] = b;
        }
    }

    image_u8x3_write_pnm(d, "debug_clusters.pnm");
    image_u8x3_destroy(d);
}
