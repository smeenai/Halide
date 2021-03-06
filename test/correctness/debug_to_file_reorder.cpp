#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main(int argc, char **argv) {
    const int size_x = 766;
    const int size_y = 311;

    {
        Func f, g, h, j;
        Var x, y;
        f(x, y) = x + y;
        g(x, y) = cast<float>(f(x, y) + f(x+1, y));
        h(x, y) = f(x, y) + g(x, y);

        Target target = get_jit_target_from_environment();
        if (target.has_gpu_feature()) {
            f.compute_root().gpu_tile(x, y, 1, 1).reorder_storage(y, x).debug_to_file("f2.tmp");
            g.compute_root().gpu_tile(x, y, 1, 1).reorder_storage(y, x).debug_to_file("g2.tmp");
            h.compute_root().gpu_tile(x, y, 1, 1).debug_to_file("h2.tmp");
        } else {
            f.compute_root().reorder_storage(y, x).debug_to_file("f2.tmp");
            g.compute_root().reorder_storage(y, x).debug_to_file("g2.tmp");
            h.compute_root().debug_to_file("h2.tmp");
        }

        Image<float> im = h.realize(size_x, size_y, target);
    }

    FILE *f = fopen("f2.tmp", "rb");
    FILE *g = fopen("g2.tmp", "rb");
    FILE *h = fopen("h2.tmp", "rb");
    assert(f && g && h);

    int header[5];
    assert(fread((void *)(&header[0]), 4, 5, f) == 5);
    assert(header[0] == size_x+1);
    assert(header[1] == size_y);
    assert(header[2] == 1);
    assert(header[3] == 1);
    assert(header[4] == 7);

    std::vector<int32_t> f_data((size_x+1)*size_y);
    assert(fread((void *)(&f_data[0]), 4, (size_x+1)*size_y, f) == (size_x+1)*size_y);
    for (int y = 0; y < size_y; y++) {
        for (int x = 0; x < size_x+1; x++) {
            int32_t val = f_data[y*(size_x+1)+x];
            if (val != x+y) {
                printf("f_data[%d, %d] = %d instead of %d\n", x, y, val, x+y);
                return -1;
            }
        }
    }
    fclose(f);

    assert(fread((void *)(&header[0]), 4, 5, g) == 5);
    assert(header[0] == size_x);
    assert(header[1] == size_y);
    assert(header[2] == 1);
    assert(header[3] == 1);
    assert(header[4] == 0);

    std::vector<float> g_data(size_x*size_y);
    assert(fread((void *)(&g_data[0]), 4, size_x*size_y, g) == size_x*size_y);
    for (int y = 0; y < size_y; y++) {
        for (int x = 0; x < size_x; x++) {
            float val = g_data[y*size_x+x];
            float correct = (float)(f_data[y*(size_x+1)+x] + f_data[y*(size_x+1)+x+1]);
            if (val != correct) {
                printf("g_data[%d, %d] = %f instead of %f\n", x, y, val, correct);
                return -1;
            }
        }
    }
    fclose(g);

    assert(fread((void *)(&header[0]), 4, 5, h) == 5);
    assert(header[0] == size_x);
    assert(header[1] == size_y);
    assert(header[2] == 1);
    assert(header[3] == 1);
    assert(header[4] == 0);

    std::vector<float> h_data(size_x*size_y);
    assert(fread((void *)(&h_data[0]), 4, size_x*size_y, h) == size_x*size_y);
    for (int y = 0; y < size_y; y++) {
        for (int x = 0; x < size_x; x++) {
            float val = h_data[y*size_x+x];
            float correct = f_data[y*(size_x+1)+x] + g_data[y*size_x+x];
            if (val != correct) {
                printf("h_data[%d, %d] = %f instead of %f\n", x, y, val, correct);
                return -1;
            }
        }
    }
    fclose(h);

    printf("Success!\n");
    return 0;

}
