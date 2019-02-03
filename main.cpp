#include <iostream>
#include <SFML/Graphics.hpp>
#include <fstream>

using namespace std;

int main(int argc,  char** argv)
{
    cout << "GXTEdit CMD v2.0 by Owocek (30th January 2019)" << endl << endl;

    if(argc < 2)
    {
        cout << "Usage: GXTEditCMD.exe <file1> [file2, file3...] [chunk_w=, chunk_h=]" << endl << "Check \"HOW TO USE.txt\" for more details." << endl;
        return 1;
    }

    vector<std::string> files;

    int cus_w=1,cus_h=1;
    bool custom_chunk = false;

    if(argc >= 2)
    {
        for(int i=1; i<argc; i++)
        {
            string param = argv[i];

            if(param.find("chunk_w=") != std::string::npos)
            {
                string val = param.substr(param.find_first_of("=")+1);
                cus_w = atoi(val.c_str());
                custom_chunk = true;
            }
            else if(param.find("chunk_h=") != std::string::npos)
            {
                string val = param.substr(param.find_first_of("=")+1);
                cus_h = atoi(val.c_str());
                custom_chunk = true;
            }
            else
            {
                files.push_back(param);
            }
        }
    }

    for(int i=0; i<files.size(); i++)
    {
        string arg = files[i];

        string name_a = arg.substr(arg.find_last_of("\\")+1);
        string name_b = name_a.substr(0,name_a.find_last_of("."));

        string filename = files[i];

        int length;
        int image_param;
        int image_info;
        int pal_size = 16;
        int width,height;
        int chunk_w=cus_w,chunk_h=cus_h;

        unsigned int swizzling,pal_off;

        vector<int> pixels;

        vector<int> pal_r;
        vector<int> pal_g;
        vector<int> pal_b;
        vector<int> pal_a;

        sf::Image image;

        ifstream gxt(filename.c_str(), std::ios::binary);

        cout << "Reading " << filename << endl;

        int image_info_off;
        int image_info_size;

        gxt.seekg(0x28);
        gxt.read(reinterpret_cast<char*>(&image_info_off), sizeof(uint32_t));
        image_info = image_info_off + 0x40;

        cout << "Image info offset: 0x" << std::hex << image_info_off << std::dec << endl;
        cout << "Image info: 0x" << std::hex << image_info << std::dec << endl;

        gxt.seekg(image_info_off+0x4);
        gxt.read(reinterpret_cast<char*>(&image_info_size), sizeof(uint32_t));

        cout << "Image info size: 0x" << std::hex << image_info_size << std::dec << endl;

        gxt.seekg(0x10);
        gxt.read(reinterpret_cast<char*>(&image_param), sizeof(uint32_t));

        cout << "Image param offset: 0x" << std::hex << image_param << std::dec << endl;

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

        cout << "Palette offset: 0x" << std::hex << pal_off << std::dec << endl;
        cout << "Pixel data offset: 0x" << std::hex << px_start << std::dec << endl << endl;
        cout << "Detected colors: " << pal_size << endl;
        cout << "Amount of pixels: " << length << endl;
        cout << endl << "Image width: " << width << "   Image height: " << height << endl;

        if(swizzling == 1)
        {
            cout << "SWIZZLING ENABLED! You might want to adjust the chunks with arrow keys." << endl;

            if(!custom_chunk)
            {
                if(pal_size == 16)
                {
                    cout << "Preset swizzling set to 32x8 for 16 color textures" << endl;
                    chunk_w = 32;
                    chunk_h = 8;
                }

                if(pal_size == 256)
                {
                    cout << "Preset swizzling set to 16x8 for 256 color textures" << endl;
                    chunk_w = 16;
                    chunk_h = 8;
                }
            }
        }

        ///reading palette
        for(int i=0; i<pal_size; i++)
        {
            uint8_t u_R;
            uint8_t u_G;
            uint8_t u_B;
            uint8_t u_A;

            int R;
            int G;
            int B;
            int A;

            gxt.seekg(pal_off+(i*4));
            gxt.read(reinterpret_cast<char*>(&u_R), sizeof(uint8_t));
            R = static_cast<int>(u_R);

            gxt.seekg(pal_off+(i*4)+1);
            gxt.read(reinterpret_cast<char*>(&u_G), sizeof(uint8_t));
            G = static_cast<int>(u_G);

            gxt.seekg(pal_off+(i*4)+2);
            gxt.read(reinterpret_cast<char*>(&u_B), sizeof(uint8_t));
            B = static_cast<int>(u_B);

            gxt.seekg(pal_off+(i*4)+3);
            gxt.read(reinterpret_cast<char*>(&u_A), sizeof(uint8_t));
            A = static_cast<int>(u_A);

            pal_r.push_back(R);
            pal_g.push_back(G);
            pal_b.push_back(B);
            pal_a.push_back(A);
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

            if(pal_size == 16)
            {
                int pixel_1,pixel_2;

                pixel_1 = floor(pixel/16);
                pixel_2 = pixel - pixel_1*16;

                pixels.push_back(pixel_2);
                pixels.push_back(pixel_1);
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
                        image.setPixel(x+cw,y+ch,sf::Color(pal_r[pixels[p]],pal_g[pixels[p]],pal_b[pixels[p]],pal_a[pixels[p]]));
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
        cout << "Done." << endl;
    }

    return 0;
}
