namespace GBC::CARTRIDGE
{
    FILE*          FILE_INPUT = nullptr;
    unsigned int   FILE_SIZE  = 0;
    unsigned char* BUFFER     = new unsigned char[65536];

    unsigned const char NINTENDO_LOGO[] = {
        0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,0x03,0x73,0x00,0x83,0x00,0x0C,
        0x00,0x0D,0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,0xDC,0xCC,0x6E,0xE6,
        0xDD,0xDD,0xD9,0x99,0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,0xDD,0xDC,
        0x99,0x9F,0xBB,0xB9,0x33,0x3E};

    struct HEADER
    {
        unsigned char jump_code        [4];
        unsigned char nintendo_logo   [48];
        signed   char game_title      [16];
        unsigned char manufactor_code  [4];
        unsigned char cgb_flag;
        unsigned char new_license_code [2];
        unsigned char sgb;
        unsigned char cartridge_type;
        unsigned char rom_size;
        unsigned char ram_size;
        unsigned char destination_code;
        unsigned char old_license_code;
        unsigned char mask_rom_version;
        unsigned char header_checksum;
        unsigned char global_checksum  [2];
    } HEADER;

    void DUMP(unsigned int offset, unsigned int size)
    {
        unsigned int maxx = 16;
        unsigned int maxy = size/maxx;

        for(unsigned int y=0; y<maxy; y++, offset+=16)
        {
            printf("%04x: ", offset);
            for(unsigned int x=0; x<maxx; x++)
                printf("%02x ", BUFFER[offset+x]);

            printf(" ");
            for(unsigned int x=0; x<maxx; x++)
                printf("%c", BUFFER[offset+x]<'!'|| BUFFER[offset+x]>'~'?'.':BUFFER[offset+x]);

            printf("\n");
        }   
    }

    bool INIT(const char *FILE_NAME)
    {
        memset(BUFFER,  0, 65536);
        memset(&HEADER, 0, sizeof(HEADER));


        if(!(FILE_INPUT = fopen(FILE_NAME, "r")))
        {
            printf("Error, cannot open input file: '%s'\n", FILE_NAME);
            return false;
        }

        fseek(FILE_INPUT, 0, SEEK_END);
        FILE_SIZE = ftell(FILE_INPUT);
        fseek(FILE_INPUT, 0, SEEK_SET);

        if(FILE_SIZE > 65536)
        {
            printf("This isn't a gameBoy classic rom!: '%s'\n", FILE_NAME);
            return false;
        }

        fread(BUFFER, 1, FILE_SIZE, FILE_INPUT);            
        memcpy(&HEADER, BUFFER+0x100, sizeof(HEADER));


        for(unsigned int n=0; n<48; n++)
        if(NINTENDO_LOGO[n] != HEADER.nintendo_logo[n])
        {
            printf("This isn't a gameBoy classic rom!: '%s'\n", FILE_NAME);
            return false;
        }

        return true;
    }
}