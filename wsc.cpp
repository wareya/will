#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <string>
#include <fstream>

const char * filename;

bool trap = false;

void fread_or_die(void * a, size_t b, size_t c, FILE * d)
{
    size_t got = fread(a, b, c, d);
    if(feof(d)) {/*puts("feof"); puts(filename);*/ trap = true;}
    if(ferror(d)) {/*puts("ferror");*/ trap = true;}
    if(got != c) {trap = true;}
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

std::vector<extdata> extensions;

std::string replace(std::string in, std::string older, std::string newer, bool all = false)
{
    recurse:
    if(in.find(older) != std::string::npos)
    {
        in = in.replace(in.find(older), older.length(), newer);
        if(all)
            goto recurse;
    }
    return in;
}

int main(int argc, char ** argv)
{
    if(argc < 3) return 0;
    
    bool compiling = false;
    if (strncmp(argv[1], "-c", 2) == 0) compiling = true;
    
    if(compiling)
    {
        for(int argument = 2; argument < argc; argument++)
        {
            std::ifstream infile(argv[argument]);
            
            std::string name(argv[argument]);
            name = replace(name, ".txt", ".WSC");
            name = replace(name, ".WSC.WSC", ".WSC");
            puts(name.data());
            auto outfile = fopen(name.data(), "wb");
            
            std::string line;
            while(std::getline(infile, line))
            {
                if(line.find("op") != 0 or line.find(":") == std::string::npos or line.size() < 5)
                    continue;
                
                uint8_t op = std::stoul(line.substr(2, 2), nullptr, 16);
                fputc(op, outfile);
                
                int start = line.find(":");
                
                for(unsigned int i = start+1; i < line.length(); i++)
                {
                    if(line[i] == 0 or line[i] == '\n' or line[i] == '\r') break;
                    if(line[i] == ' ') continue;
                    if(line[i] != '"' and line.length() >= i+2)
                    {
                        uint8_t data = std::stoul(line.substr(i, 2), nullptr, 16);
                        fputc(data, outfile);
                        i++;
                    }
                    if(line[i] == '"' and line.length() >= i+2)
                    {
                        i++;
                        while(line[i] != '"' or line[i-1] == '\\')
                        {
                            if(line[i] == '\\')
                            {
                                if(line[i+1] == '\\')
                                    fputc('\\', outfile);
                                if(line[i+1] == '"')
                                    fputc('"', outfile);
                                i++;
                            }
                            else
                                fputc(line[i], outfile);
                            i++;
                        }
                        fputc(0, outfile);
                        i++;
                    }
                }
            }
            fclose(outfile);
        }
    }
    else
    {
        for(int argument = 1; argument < argc; argument++)
        {
            trap = false;
            filename = argv[argument];
            auto f = fopen(filename, "rb");
            
            std::string name(filename);
            name.append(".txt");
            auto out = fopen(name.data(), "wb");
            
            unsigned char trash[512];
            
            while(!feof(f) and !ferror(f) and !trap)
            {
                uint8_t command;
                fread_or_die(&command, 1, 1, f);
                if(trap) break;
                
                // https://sourceforge.net/p/vilevn/vilevn/ci/7d130e34ebe88970b18eecc9ddb52b3d03eae68b/tree/src/will/will.cpp
                auto FGENERIC = [&](int N)
                {
                    fprintf(out, "op%02X: ", command);
                    fread_or_die(trash, 1, (N), f);
                    for(int i = 0; i < (N); i++)
                        fprintf(out, "%02X ", trash[i]);
                    fputs("\n", out);
                };
                auto FGENERIC2 = [&](int N)
                {
                    fprintf(out, "op%02X (suspicious): ", command);
                    fread_or_die(trash, 1, (N), f);
                    for(int i = 0; i < (N); i++)
                        fprintf(out, "%02X ", trash[i]);
                    fputs("\n", out);
                };
                auto FSTRING = [&](int N)
                {
                    fprintf(out, "op%02X: ", command);
                    fread_or_die(trash, 1, (N), f);
                    for(int i = 0; i < (N); i++)
                        fprintf(out, "%02X ", trash[i]);
                    fprintf(out, "\"");
                    for(;;)
                    {
                        int c = fgetc(f);
                        if(c == 0)
                            break;
                        if(c == '\\' or c == '"')
                            putc('\\', out); // escape character if needed
                        putc(c, out);
                    }
                    fprintf(out, "\"\n");
                };
                auto FGOODSTRING = [&](int N)
                {
                    fprintf(out, "op%02X: ", command);
                    fread_or_die(trash, 1, (N), f);
                    for(int i = 0; i < (N); i++)
                        fprintf(out, "%02X ", trash[i]);
                    fprintf(out, "\"");
                    for(;;)
                    {
                        int c = fgetc(f);
                        if(c == 0)
                            break;
                        if(c == '\\' or c == '"')
                            putc('\\', out); // escape character if needed
                        putc(c, out);
                    }
                    fprintf(out, "\"\n");
                };
                auto FDOUBLESTRING = [&](int N)
                {
                    fprintf(out, "op%02X: ", command);
                    fread_or_die(trash, 1, (N), f);
                    for(int i = 0; i < (N); i++)
                        fprintf(out, "%02X ", trash[i]);
                    fprintf(out, "\"");
                    for(;;)
                    {
                        int c = fgetc(f);
                        if(c == 0)
                            break;
                        if(c == '\\' or c == '"')
                            putc('\\', out); // escape character if needed
                        putc(c, out);
                    }
                    fprintf(out, "\" \"");
                    for(;;)
                    {
                        int c = fgetc(f);
                        if(c == 0)
                            break;
                        if(c == '\\' or c == '"')
                            putc('\\', out); // escape character if needed
                        putc(c, out);
                    }
                    fprintf(out, "\"\n");
                };
                #define GENERIC(M, N) case (M): FGENERIC(N); break;
                #define GENERIC2(M, N) case (M): FGENERIC2(N); break;
                #define STRING(M, N) case (M): FSTRING(N); break;
                #define GOODSTRING(M, N) case (M): FGOODSTRING(N); break;
                #define DOUBLESTRING(M, N) case (M): FDOUBLESTRING(N); break;
                switch(command)
                {
                GENERIC(0x03, 7)
                GENERIC(0x06, 5)
                GENERIC(0x08, 2)
                GENERIC(0x0A, 1)
                GENERIC(0x0C, 3)
                GENERIC(0x22, 4)
                GENERIC(0x26, 2)
                GENERIC(0x45, 4)
                GENERIC(0x47, 2)
                GENERIC(0x49, 3)
                GENERIC(0x4F, 4)
                GENERIC(0x51, 5)
                GENERIC(0x52, 2)
                GENERIC(0x74, 2)
                GENERIC(0x82, 3)
                GENERIC(0x83, 1)
                GENERIC(0x84, 1)
                GENERIC(0x8B, 1)
                GENERIC(0xE2, 1)
                GENERIC(0xB8, 3)
                
                GENERIC2(0x30, 4)
                GENERIC2(0x4E, 4)
                GENERIC2(0x62, 1)
                GENERIC2(0x85, 2)
                GENERIC2(0x86, 2)
                GENERIC2(0x88, 3)
                GENERIC2(0x89, 1)
                GENERIC2(0x8A, 1)
                GENERIC2(0x8C, 3)
                GENERIC2(0x8E, 1)
                GENERIC2(0xBC, 4)
                GENERIC2(0xBD, 2)
                GENERIC2(0xBE, 1)
                GENERIC2(0xE5, 1)
                
                STRING(0x50, 0)
                STRING(0x54, 0)
                STRING(0x61, 0)
                STRING(0x73, 9)
                STRING(0xB6, 2) // FIXME: "text"
                
                STRING(0x09, 2)
                
                STRING(0xE0, 0) // scene title
                
                GOODSTRING(0x41, 4)
                STRING(0x43, 6)
                STRING(0x46, 9)
                
                //STRING(0x07, 9)
                // not: 0 1 2 12
                // probably not: 
                // might be: 16
                //STRING(0x07, 0)
                
                GENERIC(0x4A, 6) // 6.
                
                //GENERIC(0xB1, 24)
                GENERIC(0x4B, 16)
                //    not: ?? 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1
                //implies: B1
                // !!!! B9 is definitely a command
                GENERIC(0xB1, 5)
                // must be 0, 2, or 5
                
                GENERIC(0x68, 9) // certain
                //GENERIC(0x68, 9)
                GENERIC(0x65, 8) // 5 or 8 or 12, probably 8
                GENERIC(0x67, 8) // probably 8
                //GENERIC(0x66, 8) // probably 8
                //GENERIC(0xE8, 9)
                GENERIC(0x66, 18)
                // not 7, probably 9
                //GENERIC(0x90, 9)
                
                //GENERIC(0xE8, 9)
                
                GENERIC(0x4C, 8)
                
                STRING(0x48, 11)
                GENERIC(0x63, 3) // 3 or 7 or larger
                GENERIC(0xB9, 3) // 3 or 1
                
                GENERIC(0x64, 8)
                
                STRING(0x23, 9)
                
                STRING(0x21, 10)
                
                //DOUBLESTRING(0x42, 10)
                DOUBLESTRING(0x42, 5)
                GENERIC(0xA6, 1)
                
                GENERIC(0x19, 5)
                
                GENERIC(0x4D, 13)
                GENERIC(0xD0, 9)
                GENERIC(0x28, 5)
                
                STRING(0x07, 0) // script call
                GENERIC(0xFF, 8) // end of file
                
                STRING(0xB2, 2)
                GENERIC(0xB4, 12)
                //GENERIC(0xB5, 2)
                GENERIC(0xE4, 2)
                
                //GENERIC2(0x76, 9)
                //GENERIC(0xF4, 9)
                //GENERIC(0x02, 7) // must be 7 or 76's length is not 9, must be 5 or f4's length is not 12
                //GENERIC(0x70, 8)
                //GENERIC(0x76, 4) // not 3
                GENERIC(0x76, 17)
                //GENERIC(0xC4, 12)
                //GENERIC2(0xC8, 12)
                //GENERIC(0xF4, 12)
                
                GENERIC(0x01, 10)
                
                /*
                02 
                op
                02 00
                num choices?
                77 00
                some kind of flag?
                8B 7D 82 A2 82 C5 82 C8 82 C8 82 DD 82 CC 8F 8A 82 D6 8B EC 82 AF 82 C2 82 AF 82 E9 00
                text
                01 52 03
                ? branch ?
                07 41 30 32 5F 30 36 62 00 // is the 07 here length instead of command?
                scipt name
                78 00
                some kind of flag?
                82 AB 82 E7 82 E7 82 CC 8C A9 82 A6 82 E9 82 E0 82 CC 82 C9 82 C2 82 A2 82 C4 89 EF 98 62 00
                text
                01 53 03
                ? branch ?
                //// normal 01 looks like this:
                //// 01 (06) (F5 03) (00 00) (08 00 00 00) 00
                //// 01 (03) (B7 02) (00 00) (17 00 00 00) 00
                07 41 30 32 5F 30 36 61 00
                script name
                07 41 30 32 5F 30 36 62 00
                call @ 5047
                07 41 30 32 5F 30 36 61 00
                call @ 5050
                FF DD 00 00 00 80 00 00 00
                end of the file
                */
                /*
                {
                02
                02 00
                06 00
                93 64 98 62 82 F0 8E E6 82 E9 00
                01 52 03
                07 41 30 32 5F 31 31 61 00 // is the 07 here length instead of command?
                07 00
                93 64 98 62 82 F0 8E E6 82 E7 82 C8 82 A2 00
                01 53 03
                07 41 30 32 5F 31 31 62 00
                07 41 30 32 5F 31 31 61 00
                07 41 30 32 5F 31 31 62 00
                }
                */
                
                case 0x02:
                    fprintf(out, "op02: ");
                    uint16_t numchoices;
                    fread_or_die(&numchoices, 2, 1, f);
                    
                    fprintf(out, "%02X %02X ", numchoices&0xFF, numchoices>>8);
                    
                    for(int i = 0; i < numchoices; i++)
                    {
                        fread_or_die(trash, 1, 2, f);
                        for(int i = 0; i < 2; i++)
                            fprintf(out, "%02X ", trash[i]);
                        
                        fprintf(out, "\"");
                        for(;;)
                        {
                            int c = fgetc(f);
                            if(c == 0)
                                break;
                            fputc(c, out);
                        }
                        fprintf(out, "\" ");
                        
                        fread_or_die(trash, 1, 4, f);
                        for(int i = 0; i < 4; i++)
                            fprintf(out, "%02X ", trash[i]);
                        
                        fprintf(out, "\"");
                        for(;;)
                        {
                            int c = fgetc(f);
                            if(c == 0) break;
                            fputc(c, out);
                        }
                        fprintf(out, "\" ");
                    }
                    fputs("\n", out);
                    break;
                    
                
                /* */ GENERIC2(0x70, 8)
                
                GENERIC(0xA8, 16)
                
                STRING(0x25, 11)
                //GENERIC2(0x25, 2)
                GENERIC2(0x29, 4)
                GENERIC2(0xB5, 7)
                
                GENERIC(0xA9, 1)
                GENERIC2(0x55, 1)
                
                GOODSTRING(0xBA, 11)
                GENERIC2(0xBB, 1)
                GENERIC2(0x2C, 12)
                
                GENERIC(0x0B, 2) // 2? (was 5)
                GENERIC(0x05, 2) // 2? (was 1)
                
                GENERIC2(0x8D, 1) // CGMODE
                GENERIC2(0x04, 0) // CGMODE (????????????)
                GENERIC2(0x72, 1) // CG_WAIT
                
                GENERIC2(0xB3, 2) // S08_06
                default:
                    //fprintf(out, "Unknown opcode %02X at %08X in %s\n", command, ftell(f)-1, filename);
                    exit(0);
                }
            }
            fclose(out);
            
            fclose(f);
        }
    }
}
