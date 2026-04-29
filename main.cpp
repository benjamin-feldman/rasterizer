#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>

struct Pixel {
    uint8_t r, g, b;
};

struct Canvas {
    int width, height;

    // Contiguous list of pixels
    // pixel (x, y) is at pixels[y*width + x]
    std::vector<Pixel> pixels;

    Canvas(int w, int h) : width(w), height(h), pixels(w * h){}

    void putPixel(int x, int y, Pixel pixel){
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        pixels[y*width + x] = pixel;
    }

    void save() {
        FILE* f = fopen("out.ppm", "wb");
        fprintf(f, "P6\n%d %d\n255\n", width, height);
        fwrite(pixels.data(), sizeof(Pixel), width * height, f);
        fclose(f);
    }
};

int main(){
    auto c = Canvas(1280, 720);
    c.save();

    return 0;
}
