#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "Cartridge.hpp"
#include "Cpu.hpp"

namespace GBC
{
    bool INIT(const char *file_name)
    {
        CARTRIDGE::INIT(file_name);
        CPU::INIT();

        return true;
    }
};



int main(int argc, const char *argv[])
{

    GBC::INIT(argv[1]);

    for(unsigned int a=0; a<100; a++)
    {
        GBC::CPU::CLOCK();
    }

    GBC::CPU::REG.DUMP();
    GBC::CARTRIDGE::DUMP(0, 0xFFFF);

    return 0;
}



































































    /*FILE *o = fopen("out.bin", "w");
    for(unsigned short n=0,v=0; n<256; n++)
    switch(GBC::CPU::INSTRUCTIONS[n].Length)
    {
        case 1:
            fputc(n, o);
            break;
        case 2:
            fputc(n, o);
            fputc(v, o);
            v++;
            break;
        case 3:
            fputc(n, o);
            fputc( v&0xFF, o);
            fputc((v&0xFF00)>>8, o);
            v++;
            break;
        default:
            printf("ERROR\n");
            break;
    }
    char *f = new char[65536];
    memset(f, 0, 65536);
    fwrite(f, 1, 65536, o);    
    fclose(o);*/

