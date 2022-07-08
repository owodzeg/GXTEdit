#include <iostream>
#include <SFML/Graphics.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>

//using namespace std;
namespace fs = std::filesystem;

void convert_file(const fs::path& path) {
    int length, width, height;
    int pal_size = 16;
    int chunk_w = 1, chunk_h = 1;
    int swizzling = 0;

    uint32_t image_param, image_info;
    uint32_t image_info_off, image_info_size;
    uint32_t pal_off = 0; // to read it later with reinterpret_cast<char*>

    std::vector<int> pixels;

    sf::Image image;

    std::ifstream gxt(path, std::ios::binary);
    if (!gxt) {
        std::cout << "Cannot access " << path << "\nConversion aborted\n\n";
        return;
    }

    // is it necessary to call c_str if gxt can be contructed from const std::string& ?

    std::cout << "Reading " << path << "\n";

    gxt.seekg(0x28);
    gxt.read(reinterpret_cast<char*>(&image_info_off), sizeof(uint32_t));
    image_info = image_info_off + 0x40;

    gxt.seekg(image_info_off + 0x4);
    gxt.read(reinterpret_cast<char*>(&image_info_size), sizeof(uint32_t));

    gxt.seekg(0x10);
    gxt.read(reinterpret_cast<char*>(&image_param), sizeof(uint32_t));

    ///reading px length
    gxt.seekg(0x44);
    gxt.read(reinterpret_cast<char*>(&length), sizeof(uint32_t));

    uint8_t sz_tmp = 0;

    gxt.seekg(image_info);
    gxt.read(reinterpret_cast<char*>(&sz_tmp), sizeof(uint8_t));
    swizzling = sz_tmp;

    uint8_t pl_tmp = 0;

    gxt.seekg(image_info + image_info_size - 0x8);
    gxt.read(reinterpret_cast<char*>(&pl_tmp), sizeof(uint8_t));

    if (pl_tmp == 0x2) {
        pal_size = 16;
    }
    if (pl_tmp == 0x20) {
        pal_size = 256;
    }

    gxt.seekg(image_info + image_info_size - 0xC);
    gxt.read(reinterpret_cast<char*> (&pal_off), 3); // this works because the 4th byte is zero

    gxt.seekg(image_param + 0x20);
    gxt.read(reinterpret_cast<char*>(&width), sizeof(uint32_t));

    gxt.seekg(image_param + 0x24);
    gxt.read(reinterpret_cast<char*>(&height), sizeof(uint32_t));

    int px_start = pal_off - 0x40 - length;

    std::cout << "Detected colors: " << pal_size << "\n";
    std::cout << "Amount of pixels: " << length << "\n\n";
    std::cout << "Image width: " << width << "   Image height: " << height << "\n";

    if (swizzling == 1) {
        if (pal_size == 16) {
            chunk_w = 32;
            chunk_h = 8;
        }

        if (pal_size == 256) {
            chunk_w = 16;
            chunk_h = 8;
        }

        std::cout << "Preset swizzling set to " << chunk_w << "x" << chunk_h << " for " << pal_size << " color textures\n";
    }

    ///reading palette

    std::vector<sf::Color> pal_color;
    pal_color.reserve(pal_size);

    gxt.seekg(pal_off);
    for (int i = 0; i < pal_size; ++i) {
        uint32_t color;

        gxt.read(reinterpret_cast<char*>(&color), sizeof(uint32_t));

        pal_color.push_back(sf::Color(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, (color >> 24) & 0xFF)); // weird
    }

    ///reading pixels
    std::vector <char> c_pixels(length);

    gxt.seekg(px_start);
    gxt.read(&(c_pixels.front()), length);

    for (unsigned int i = 0; i < c_pixels.size(); ++i) {
        unsigned char px = c_pixels[i];
        int pixel = px;

        if (pal_size == 256) {
            pixels.push_back(pixel);
        }
        else if (pal_size == 16) {
            pixels.push_back(pixel & 0xF);
            pixels.push_back((pixel >> 4) & 0xF);
        }
    }

    if ((width == 48) || (height == 48)) { // someone got Pythoned here with "or" instead of ||
        std::cout << "Applying x48 fix (for P3 items)\n";

        width = 64;
        height = 64;
    }

    if ((width == 137) || (height == 78)) {
        std::cout << "Applying rtbl fix (for P3 teamcard thumbnails)\n";

        width = 144;
        height = 80;
    }

    gxt.close();

    image.create(width, height, sf::Color(0, 0, 0, 0));

    int x = 0, y = 0;
    unsigned int p = 0;

    while (p < pixels.size()) {
        for (int ch = 0; ch < chunk_h; ++ch) {
            for (int cw = 0; cw < chunk_w; ++cw) {
                if (x + cw < width) {
                    if (y + ch < height) {
                        image.setPixel(x + cw, y + ch, pal_color[pixels[p]]);
                        ++p;
                    }
                }
            }
        }

        x += chunk_w;

        if (x >= width) {
            x = 0;
            y += chunk_h;
        }

        if (y >= height) {
            break;
        }
    }

    fs::path save_path = path;
    save_path.replace_extension(".png");
    image.saveToFile(save_path.string());
    std::cout << "Saving as " << save_path << "\n\n";
}

int main(int argc, char** argv) {
    std::cout << "GXTEdit CMD v2.2 by Owocek and Nemoumbra (8th July 2022)\n\n";

    if (argc < 2) {
        std::cout << "Usage: GXTEditCMD.exe <file1> [file2, file3...]\n" << "Check \"HOW TO USE.txt\" for more details.\n";
        return 1; // if argc < 2, the programs stops
    }

    std::vector <fs::path> paths;
    paths.reserve(argc - 1);

    // given that the program reached this part, argc is automatically "not less than 2" == "bigger than or equal to 2"

    for (int i = 1; i < argc; ++i) { // argc - 1 iterations
        paths.emplace_back(argv[i]);
    }

    for (unsigned int i = 0; i < paths.size(); ++i) {
        convert_file(paths[i]);
    }

    //return 0;
}
