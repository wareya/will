#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <map>

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
    if(argc < 3)
        return 0;
    
    std::string outfname(argv[argc-1]);
    
    if(outfname.find_last_of(".") == std::string::npos)
        return 0;
    if(outfname.substr(outfname.find_last_of(".")+1, 3) != "arc")
        return 0;
    
    puts("Working");
    
    std::map<std::string, std::vector<std::string>*> extlists;
    for(int i = 1; i < argc-1; i++)
    {
        std::string name(argv[i]);
        if(name.find_last_of(".") == std::string::npos) continue;
        
        std::string ext(name.substr(name.find_last_of(".")+1, 3));
        std::string justname(name.substr(0, name.find_last_of(".")));
        
        if(extlists.find(ext) != extlists.end())
            extlists.find(ext)->second->push_back(justname);
        else
        {
            std::vector<std::string>* vect = new std::vector<std::string>();
            vect->push_back(justname);
            extlists.insert({ext, vect});
        }
    }
    
    auto out = fopen(outfname.data(), "wb");
    
    uint32_t numext = extlists.size();
    fwrite(&numext, 4, 1, out);
    
    int header_length = 4; // size of ext header
    for(auto pair : extlists)
        header_length += 12; // size of each ext data entry
    
    int numfiles = 0;
    for(auto pair : extlists)
    {
        char ext[4];
        memset(ext, 0, 4);
        strncpy(ext, pair.first.data(), 3);
        
        fwrite(ext, 1, 4, out);
        
        uint32_t start = header_length + numfiles*0x15;
        uint32_t count = pair.second->size();
        
        fwrite(&count, 4, 1, out);
        fwrite(&start, 4, 1, out);
        
        numfiles += count;
    }
    
    for(int i = 0; i < numfiles; i++)
    {
        unsigned char junk[0x15];
        fwrite(junk, 1, 0x15, out);
    }
    
    int filecounter = 0;
    for(auto pair : extlists)
    {
        std::string ext = pair.first;
        for(auto fname : *pair.second)
        {
            std::string tablename = std::string(fname);
            if(tablename.find_last_of("/") != std::string::npos)
                tablename = tablename.substr(tablename.find_last_of("/")+1, std::string::npos);
            
            fseek(out, 0, SEEK_END);
            
            uint32_t pos = ftell(out);
            
            auto in = fopen((fname+"."+ext).data(), "rb");
            
            if(in == 0)
                continue;
            
            uint32_t len = 0;
            fseek(in, 0, SEEK_END);
            len = ftell(in);
            fseek(in, 0, SEEK_SET);
            
            if(1)
            {
                unsigned char * data = (unsigned char *)malloc(len);
                fread(data, 1, len, in);
                if(strncmp(ext.data(), "WSC", 3) == 0)
                {
                    for(uint32_t i = 0; i < len; i++)
                        data[i] = rotl_8(data[i], 2);
                }
                fwrite(data, 1, len, out);
                
                fseek(out, header_length + filecounter*0x15, SEEK_SET);
                char name[13];
                memset(name, 0, 13);
                strncpy(name, tablename.data(), 12);
                
                fwrite(name, 1, 13, out);
                fwrite(&len, 4, 1, out);
                fwrite(&pos, 4, 1, out);
                free(data);
            }
            
            fclose(in);
            
            filecounter++;
        }
    }
}
