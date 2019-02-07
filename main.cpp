#include <iostream>
#include <SFML/Graphics.hpp>
#include <fstream>

using namespace std;

int main(int argc,  char** argv)
{
    cout << "GXTEdit CMD v2.0 by Owocek (30th January 2019)" << endl << endl;

    if(argc < 2)
    {
        cout << "Usage: GXTEditCMD.exe <file1> [file2, file3...]" << endl << "Check \"HOW TO USE.txt\" for more details." << endl;
        return 1;
    }

    vector<std::string> files;

    if(argc >= 2)
    {
        for(int i=1; i<argc; i++)
        {
            files.push_back(string(argv[i]));
        }
    }

    for(unsigned int i=0; i<files.size(); i++)
    {
        string name_a = files[i].substr(files[i].find_last_of("\\")+1);
        string name_b = name_a.substr(0,name_a.find_last_of("."));

        int length;
        int image_param;
        int image_info;
        int pal_size = 16;
        int width,height;
        int chunk_w=1,chunk_h=1;

        unsigned int swizzling,pal_off;

        vector<int> pixels;
        vector<sf::Color> pal_color;

        sf::Image image;

        ifstream gxt(files[i].c_str(), std::ios::binary);

        cout << "Reading " << files[i] << endl;

        int image_info_off;
        int image_info_size;

        gxt.seekg(0x28);
        gxt.read(reinterpret_cast<char*>(&image_info_off), sizeof(uint32_t));
        image_info = image_info_off + 0x40;

        gxt.seekg(image_info_off+0x4);
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

        if(pl_tmp == 0x2)
        {
            pal_size = 16;
        }
        if(pl_tmp == 0x20)
        {
            pal_size = 256;
        }

        unsigned char c_pal_off[3];

        gxt.seekg(image_info + image_info_size - 0xC);
        gxt.read(reinterpret_cast<char*>(&c_pal_off), 3);
        pal_off = (c_pal_off[2] << 16) + (c_pal_off[1] << 8) + c_pal_off[0];

        gxt.seekg(image_param+0x20);
        gxt.read(reinterpret_cast<char*>(&width), sizeof(uint32_t));

        gxt.seekg(image_param+0x24);
        gxt.read(reinterpret_cast<char*>(&height), sizeof(uint32_t));

        int px_start = pal_off - 0x40 - length;

        cout << "Detected colors: " << pal_size << endl;
        cout << "Amount of pixels: " << length << endl;
        cout << endl << "Image width: " << width << "   Image height: " << height << endl;

        if(swizzling == 1)
        {
            if(pal_size == 16)
            {
                chunk_w = 32;
                chunk_h = 8;
            }

            if(pal_size == 256)
            {
                chunk_w = 16;
                chunk_h = 8;
            }

            cout << "Preset swizzling set to " << chunk_w << "x" << chunk_h << " for " << pal_size << " color textures" << endl;
        }

        ///reading palette
        for(int i=0; i<pal_size; i++)
        {
            uint32_t color;

            gxt.seekg(pal_off+(i*4));
            gxt.read(reinterpret_cast<char*>(&color), sizeof(uint32_t));

            pal_color.push_back(sf::Color(color&0xFF,(color>>8)&0xFF,(color>>16)&0xFF,(color>>24)&0xFF));
        }

        ///reading pixels
        char c_pixels[length];

        gxt.seekg(px_start);
        gxt.read(c_pixels,length);

        for(unsigned int i=0; i<sizeof(c_pixels); i++)
        {
            unsigned char px = c_pixels[i];
            int pixel = px;

            if(pal_size == 256)
            {
                pixels.push_back(pixel);
            }
            else if(pal_size == 16)
            {
                pixels.push_back(pixel&0xF);
                pixels.push_back((pixel>>4)&0xF);
            }
        }

        if((width == 48) or (height == 48))
        {
            cout << "Applying x48 fix (for P3 items)" << endl;

            width = 64;
            height = 64;
        }

        gxt.close();

        image.create(width,height,sf::Color(0,0,0,0));

        int x=0,y=0;
        unsigned int p=0;

        while(p < pixels.size())
        {
            for(int ch=0; ch<chunk_h; ch++)
            {
                for(int cw=0; cw<chunk_w; cw++)
                {
                    if(x+cw < width)
                    if(y+ch < height)
                    {
                        image.setPixel(x+cw,y+ch,pal_color[pixels[p]]);
                        p++;
                    }
                }
            }

            x+=chunk_w;

            if(x >= width)
            {
                x=0;
                y+=chunk_h;
            }

            if(y >= height)
            {
                break;
            }
        }

        image.saveToFile(name_b+".png");
        cout << "Saving as " << name_b << ".png" << endl;
    }

    return 0;
}
