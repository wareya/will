#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>

#include <experimental/filesystem>

void fread_or_die(void * a, size_t b, size_t c, FILE * d)
{
    size_t got = fread(a, b, c, d);
    if(feof(d)) {puts("feof"); exit(0);}
    if(ferror(d)) {puts("ferror"); exit(0);}
    if(got != c) {exit(0);}
}

inline uint8_t rotl_8 (int c, int n)
{
    return (c<<n|c>>(8-n))&0xFF;
}

struct filedata
{
    char name[0xD];
    uint32_t len;
    uint32_t start;
};

struct extdata
{
    char ext[0x4];
    uint32_t count;
    uint32_t start;
    std::vector<filedata> files;
};

int main(int argc, char ** argv)
{
    if(argc < 2)
        return 0;
    for(int i = 1; i < argc; i++)
    {
        std::vector<extdata> extensions;
        
        puts(argv[i]);
        auto f = fopen(argv[i], "rb");
        uint32_t num_extensions;
        
        fread_or_die(&num_extensions, 1, 4, f);
        
        for(unsigned int i = 0; i < num_extensions; i++)
        {
            extdata ext;
            fread_or_die(ext.ext, 1, 4, f);
            fread_or_die(&ext.count, 4, 1, f);
            fread_or_die(&ext.start, 4, 1, f);
            extensions.push_back(ext);
        }
        
        std::string arc_name(argv[i]);
        arc_name = arc_name.substr(0, arc_name.size()-4);
        std::experimental::filesystem::v1::create_directory("arc");
        arc_name.insert(0, "arc/");
        std::experimental::filesystem::v1::create_directory(arc_name);
        
        for(auto & ext : extensions)
        {
            fseek(f, ext.start, SEEK_SET);
            for(unsigned int i = 0; i < ext.count; i++)
            {
                filedata file;
                fread_or_die(file.name, 1, 0xD, f); // name
                fread_or_die(&file.len, 4, 1, f);
                fread_or_die(&file.start, 4, 1, f);
                ext.files.push_back(file);
            }
            for(auto & file : ext.files)
            {
                //puts("--- File ---");
                //puts(file.name);
                //puts("");
                //uint32_t idk;
                //fread_or_die(&idk, 4, 1, f);
                //fgetc(f);
                //for(;;)
                //{
                //    int c = fgetc(f);
                //    if(c == 0) break;
                //    putc(rotl_8(c, 6), stdout);
                //}
                std::string filename(arc_name + "/" + file.name);
                filename += ".";
                filename += ext.ext;
                //printf("  %s\n", filename.data());
                auto f2 = fopen(filename.data(), "wb");
                
                printf("%X\n", file.start);
                
                fseek(f, file.start, SEEK_SET);
                
                unsigned char * data = (unsigned char*)malloc(file.len);
                fread_or_die(data, 1, file.len, f);
                if(strncmp(ext.ext, "WSC", 3) == 0)
                {
                    for(uint32_t i = 0; i < file.len; i++)
                        data[i] = rotl_8(data[i], 6);
                }
                fwrite(data, 1, file.len, f2);
                
                free(data);
                
                fclose(f2);
                
                /*
                while(ftell(f)-file.start < file.len)
                {
                    //fputc(rotl_8(fgetc(f), 6), f2);
                    if(strncmp(ext.ext, "WSC", 3) == 0)
                        fputc(rotl_8(fgetc(f), 6), f2);
                    else
                        fputc((unsigned char)fgetc(f), f2);
                    //uint16_t command;
                    //fread_or_die(&command, 2, 1, f);
                    //printf("%04X\n", command);
                    #if 0
                    if(command == 0x0500)
                    {
                        uint16_t unknown1;
                        uint16_t unknown2;
                        fread_or_die(&unknown1, 1, 2, f);
                        fread_or_die(&unknown2, 1, 2, f);
                        if(unknown2 != 0x3C00) continue;
                        //printf("%04X %04X\n", unknown1, unknown2);
                        //printf("%08X ", ftell(f));
                        for(;;)
                        {
                            int c = fgetc(f);
                            if(c == 0) break;
                            if(unknown2 != 0x3C00) continue;
                            putc(rotl_8(c, 6), stdout);
                        }
                        if(unknown2 == 0x3C00)
                            puts("");
                    }
                    #endif
                }
                */
            }
        }
    }
}
