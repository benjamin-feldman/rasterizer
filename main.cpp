#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>
#include <utility>


struct vec2 {
    int x, y;
};

struct Pixel {
    uint8_t r, g, b;
};

struct Canvas {
    int width, height;

    // Contiguous list of pixels
    // pixel (x, y) is at pixels[y*width + x]
    std::vector<Pixel> pixels;

    Canvas(int w, int h) : width(w), height(h), pixels(w * h){}

    void putPixelRaw(int x, int y, Pixel pixel){
        // (x, y) in screen coordinates
        assert(x >= 0 && x < width);
        assert(y >= 0 && y < height);
        pixels[y*width + x] = pixel;
    }

    void putPixel(int x, int y, Pixel pixel){
        // (x, y) in math coordinates
        int sx = width / 2 + x;
        int sy = height / 2 - y;
        putPixelRaw(sx, sy, pixel);
    }

    void save() {
        FILE* f = fopen("out.ppm", "wb");
        fprintf(f, "P6\n%d %d\n255\n", width, height);
        fwrite(pixels.data(), sizeof(Pixel), width * height, f);
        fclose(f);
    }
};

std::vector<float> interpolate(int i0, float d0, int i1, float d1){

    if (i0 == i1){
        return {d0};
    }

    std::vector<float> values = {};

    float a = (float) (d1 - d0) / (i1 - i0);
    float d = d0;

    for (int i = i0; i < i1; i++){
        values.push_back(d);
        d = d + a;
    }

    return values;
}

void drawLine(Canvas& c, vec2 p0, vec2 p1, Pixel color){
    int dx = abs(p1.x - p0.x);
    int dy = abs(p1.y - p0.y);

    if (dx > dy){
        if (p0.x > p1.x){
            std::swap(p0, p1);
        }

        auto ys = interpolate(p0.x, p0.y, p1.x, p1.y);

        for (int x = p0.x; x < p1.x; x++){
            c.putPixel(x, ys[x - p0.x], color);
        }
    }
    else {
        if (p0.y > p1.y){
            std::swap(p0, p1);
        }

        auto xs = interpolate(p0.y, p0.x, p1.y, p1.x);

        for (int y = p0.y; y < p1.y; y++){
            c.putPixel(xs[y - p0.y], y, color);
        }
    }


}

int main(){
    auto c = Canvas(201, 201);

    Pixel color = {255, 255, 255};
    drawLine(c, vec2 {-30, -30}, vec2 {30, 30}, color);
    drawLine(c, vec2 {-30, 30}, vec2 {30, -30}, color);
    drawLine(c, vec2 {0, -100}, vec2 {0, 100}, color);


    c.save();
    return 0;
}
