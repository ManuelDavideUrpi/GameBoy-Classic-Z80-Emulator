namespace GBC::CPU
{
    struct
    {
        union //AF
        {
            struct {unsigned char F, A;};
            unsigned short AF;
        };
        union //BC
        {
            struct {unsigned char C, B;};
            unsigned short BC;
        };    
        union //DE
        {
            struct {unsigned char E, D;};
            unsigned short DE;
        };   
        union //HL
        {
            struct {unsigned char L, H;};
            unsigned short HL;
        };         

        unsigned short SP=0;     
        unsigned short PC=0;

        const char           *reg_name[6]  = {"AF","BC","DE","HL","SP","PC"};
        const unsigned short *reg_addr[6]  = {&AF, &BC, &DE, &HL, &SP, &PC};

        bool FLAG_Z(){return F&128;}
        bool FLAG_N(){return F&64;}
        bool FLAG_H(){return F&32;}
        bool FLAG_C(){return F&16;}

        bool FLAG_Z(bool v){v?(F|=128):(F&=~128);return FLAG_Z();}
        bool FLAG_N(bool v){v?(F|=64): (F&=~64); return FLAG_N();}
        bool FLAG_H(bool v){v?(F|=32): (F&=~32); return FLAG_H();}
        bool FLAG_C(bool v){v?(F|=16): (F&=~16); return FLAG_C();}


        void DUMP()
        {
            printf("_____________________");

            for(unsigned int thisReg=0; thisReg<6; thisReg++)
            {
                printf("\n%2s: ", reg_name[thisReg]);
                for(unsigned int mask=1<<15; mask !=0; mask>>=1)
                    printf("%d%c", (**(reg_addr+thisReg))&mask?1:0, mask==256?' ':0);
            }
            printf("\n");
        }            
    } REG;

    struct INSTRUCTION
    {
        char            LABEL[50];
        unsigned char   LENGTH = 1;
        void            (*EXEC)();
    };

    unsigned char  READ8()    
    {
        return CARTRIDGE::BUFFER[REG.PC++];
    }
    unsigned short READ16()
    {
        unsigned short ret = CARTRIDGE::BUFFER [REG.PC] | (CARTRIDGE::BUFFER [REG.PC+1]<<8);
        REG.PC+=2;
        return ret;
    }
        
    void ERROR()
    {
        printf("   This instruction not exist!!!\n");
        exit(0);
    }

    void NOP(){}        
    void LD_r_n  (unsigned char *r,  unsigned char   n)     {*r  =   n;} 
    void LD_r_r  (unsigned char *r1, unsigned char *r2)     {*r1 = *r2;}
    void LD_r_mm (unsigned char *r, unsigned short mm)      {*r  = CARTRIDGE::BUFFER[mm];}

    void LD_mm_r (unsigned short mm, unsigned char *r)      {CARTRIDGE::BUFFER[mm] = *r;}
    void LD_mm_n (unsigned short mm, unsigned char n)       {CARTRIDGE::BUFFER[mm] = n;}

    void LD_rr_mm(unsigned short *rr,  unsigned short mm)   {*rr  = CARTRIDGE::BUFFER[mm];}
    void LD_rr_rr(unsigned short *rr1, unsigned short *rr2) {*rr1 = *rr2;}
    void LD_rr_nn(unsigned short *rr,  unsigned short nn)   {*rr  = nn;}
    void LD_mm_SP(unsigned short mm)
    {
        CARTRIDGE::BUFFER[mm]   = REG.SP&0xFF;
        CARTRIDGE::BUFFER[mm+1] = (REG.SP>>8)&0xFF;
    }
    void LDHL_SP_d(char d)
    {
        LD_rr_nn(&REG.HL, ((char)REG.SP)+d);

        REG.FLAG_Z(0);
        REG.FLAG_N(0);

        bool H = (REG.SP&0b0000111111111111) + (d&0b0000111111111111) > 0b0000111111111111;
        bool C = (REG.SP&0b1111111111111111) + (d&0b1111111111111111) > 0b1111111111111111;

        REG.FLAG_H(H);
        REG.FLAG_C(C);
    }
    void LD_mm_rr(unsigned short mm, unsigned short *rr)  {CARTRIDGE::BUFFER[mm] = *rr;}

    void PUSH_rr(unsigned short *rr)
    {
        *((unsigned short*)&CARTRIDGE::BUFFER[REG.SP-2]) = *rr;
        REG.SP -= 2;
    }        
    void POP_rr(unsigned short *rr)
    {
        *rr = *((unsigned short*)&CARTRIDGE::BUFFER[REG.SP]);
        REG.SP += 2;
    }
    
    void ADD_r_r(unsigned char *r1, unsigned char *r2)
    {
        bool c3 = ((*r1&0b00001111) + (*r2&0b00001111)) > 0b00001111;
        bool c7 = ((*r1&0b11111111) + (*r2&0b11111111)) > 0b11111111;

        *r1 += *r2;
        REG.FLAG_Z(!*r1);
        REG.FLAG_N(0);
        REG.FLAG_H(c3);
        REG.FLAG_C(c7);
    }
    void ADD_r_mm(unsigned char *r, unsigned short mm) {ADD_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void ADD_r_n (unsigned char *r,  unsigned char  n) {ADD_r_r(r, &n);}

    void ADC_r_r (unsigned char *r1, unsigned char *r2)
    {
        bool c3 = ((*r1&0b00001111) + (*r2&0b00001111)) + REG.FLAG_C() > 0b00001111;
        bool c7 = ((*r1&0b11111111) + (*r2&0b11111111)) + REG.FLAG_C() > 0b11111111;

        *r1 += *r2 + REG.FLAG_C();
        REG.FLAG_Z(!*r1);
        REG.FLAG_N(0);
        REG.FLAG_H(c3);
        REG.FLAG_C(c7);
    }
    void ADC_r_mm(unsigned char *r, unsigned short mm){ADC_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void ADC_r_n (unsigned char *r,  unsigned char n) {ADC_r_r(r, &n);}

    void SUB_r_r (unsigned char *r1, unsigned char *r2)
    {
        bool c4 = (((*r1-*r2)&0b00001111) + ((*r2)&0b00001111)) > 0b00001111;
        bool c  = (((~*r1)&0b11111111) & ((*r2)&0b11111111)) > 0b00000000;
        *r1 -= *r2;
        REG.FLAG_Z(!*r1);
        REG.FLAG_N(1);
        REG.FLAG_H(!c4);
        REG.FLAG_C(!c);
    }
    void SUB_r_mm(unsigned char *r, unsigned short mm) {SUB_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void SUB_r_n (unsigned char *r, unsigned char n)   {SUB_r_r(r, &n);}

    void SBC_r_r (unsigned char *r1, unsigned char *r2)
    {
        bool c4 = (((*r1-(*r2+1))&0b00001111) + ((*r2+1)&0b00001111)) > 0b00001111;
        bool c  = (((~*r1)&0b11111111) & ((*r2+1)&0b11111111)) > 0b00000000;
        *r1 -= (*r2+1);
        REG.FLAG_Z(!*r1);
        REG.FLAG_N(1);
        REG.FLAG_H(!c4);
        REG.FLAG_C(!c);
    }
    void SBC_r_mm(unsigned char *r, unsigned short mm) {SBC_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void SBC_r_n (unsigned char *r,  unsigned char  n) {SBC_r_r(r, &n);}

    void AND_r_r(unsigned char *r1, unsigned char *r2)
    {
        *r1 &= *r2;
        REG.FLAG_Z(!*r1);
        REG.FLAG_N(0);
        REG.FLAG_H(1);
        REG.FLAG_C(0);
    }
    void AND_r_mm(unsigned char *r, unsigned short mm) {AND_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void AND_r_n (unsigned char *r,  unsigned char  n) {AND_r_r(r, &n);}

    void OR_r_r(unsigned char *r1, unsigned char *r2)
    {
        *r1 |= *r2;
        REG.FLAG_Z(!*r1);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
        REG.FLAG_C(0);
    }
    void OR_r_mm(unsigned char *r, unsigned short mm)  {OR_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void OR_r_n (unsigned char *r, unsigned char   n)  {OR_r_r(r, &n);}

    void XOR_r_r(unsigned char *r1, unsigned char *r2)
    {
        *r1 ^= *r2;
        REG.FLAG_Z(!*r1);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
        REG.FLAG_C(0);
    }
    void XOR_r_mm(unsigned char *r, unsigned short mm){XOR_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void XOR_r_n (unsigned char *r, unsigned char n)  {XOR_r_r(r, &n);}

    void CP_r_r(unsigned char *r1, unsigned char *r2)
    {
        bool c4 = (((*r1-*r2)&0b00001111) + ((*r2)&0b00001111)) > 0b00001111;
        bool c  = (((~*r1)&0b11111111) & ((*r2)&0b11111111)) > 0b00000000;
        
        REG.FLAG_Z(!(*r1-*r2));
        REG.FLAG_N(1);
        REG.FLAG_H(!c4);
        REG.FLAG_C(!c);
    }
    void CP_r_mm(unsigned char *r, unsigned short mm){CP_r_r(r, &CARTRIDGE::BUFFER[mm]);}
    void CP_r_n (unsigned char *r, unsigned char n)  {CP_r_r(r, &n);}

    void INC_r(unsigned char *r1)
    {
        bool c3 = ((*r1&0b00001111) + 1) > 0b00001111;

        *r1 += 1;
        REG.FLAG_Z(!(*r1));
        REG.FLAG_N(0);
        REG.FLAG_H(c3);
    }
    void INC_mm(unsigned short mm) {INC_r(&CARTRIDGE::BUFFER[mm]);}

    void DEC_r(unsigned char *r1)
    {
        bool c4 = (((*r1-1)&0b00001111) + 1) > 0b00001111;

        *r1 -= 1;
        REG.FLAG_Z(!(*r1));
        REG.FLAG_N(1);
        REG.FLAG_H(c4);
    }
    void DEC_mm(unsigned short mm) {DEC_r(&CARTRIDGE::BUFFER[mm]);}

    void ADD_rr_rr(unsigned short *rr1, unsigned short *rr2)
    {
        bool c11 = ((*rr1&0b0000111111111111) + (*rr2&0b0000111111111111)) > 0b0000111111111111;
        bool c15 = ((*rr1&0b1111111111111111) + (*rr2&0b1111111111111111)) > 0b1111111111111111;

        *rr1 += *rr2;
        REG.FLAG_N(0);
        REG.FLAG_H(c11);
        REG.FLAG_C(c15);
    }
    void ADD_rr_d(unsigned short *rr, char n){*rr += n;}
    
    void INC_rr (unsigned short *rr) {*rr += 1;}
    void DEC_rr (unsigned short *rr) {*rr-=1;}

    void SWAP_r (unsigned char *r)   {*r = ((*r&0b11110000)>>4) | ((*r&0b00001111)<<4);}
    void SWAP_mm(unsigned short mm)  {SWAP_r(&CARTRIDGE::BUFFER[mm]);}

    void DAA()
    {
        unsigned char hn = REG.A>>4;
        unsigned char ln = REG.A&0b1111;

        if(ln > 9)
        {                
            if((ln&0b1111) + (6&0b1111) > 0b1111) hn++;
            ln += 6;
        }
        if(hn > 9)
        {                
            if((hn&0b1111) + (6&0b1111) > 0b1111) NULL;
            hn += 6;
        }

        REG.A = (hn<<4)|(ln&0b1111);

        REG.FLAG_Z(!REG.A);
        REG.FLAG_H(0);
    }
    void CPL()
    {
        REG.A = ~REG.A;
        REG.FLAG_N(1);
        REG.FLAG_H(1);
    }
    void CCF()
    {
        REG.FLAG_N(0);
        REG.FLAG_H(0);
        REG.FLAG_C(~REG.FLAG_C());
    }
    void SCF()
    {
        REG.FLAG_N(0);
        REG.FLAG_H(0);
        REG.FLAG_C(1);
    }

    void HALT(){}
    void STOP(){}
    void DI(){}
    void EI(){}

    void RLCA()
    {
        REG.FLAG_C(REG.A&0b10000000);
        REG.A <<= 1;
        REG.A |= REG.FLAG_C();
        REG.FLAG_Z(!REG.A);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void RLA()
    {
        unsigned char c = REG.FLAG_C();
        REG.FLAG_C(REG.A&0b10000000);
        REG.A <<= 1;
        REG.A |= c;
        REG.FLAG_Z(!REG.A);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void RRCA()
    {
        REG.FLAG_C(REG.A&0b00000001);
        REG.A >>= 1;
        REG.A |= REG.FLAG_C()<<7;
        REG.FLAG_Z(!REG.A);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void RRA()
    {
        unsigned char c = REG.FLAG_C();
        REG.FLAG_C(REG.A&0b00000001);
        REG.A >>= 1;
        REG.A |= c<<7;
        REG.FLAG_Z(!REG.A);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    
    void RLC_r(unsigned char *r)
    {
        REG.FLAG_C(*r&0b10000000);
        *r <<= 1;
        *r |= REG.FLAG_C();
        REG.FLAG_Z(!*r);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void RLC_mm(unsigned short mm){RLC_r(&CARTRIDGE::BUFFER[mm]);}

    void RL_r(unsigned char *r)
    {
        unsigned char c = REG.FLAG_C();
        REG.FLAG_C(*r&0b10000000);
        *r <<= 1;
        *r |= c;
        REG.FLAG_Z(!*r);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void RL_mm(unsigned short mm)  {RL_r(&CARTRIDGE::BUFFER[mm]);}

    void RRC_r(unsigned char *r)
    {
        REG.FLAG_C(*r&0b00000001);
        *r >>= 1;
        *r |= REG.FLAG_C()<<7;
        REG.FLAG_Z(!*r);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void RRC_mm(unsigned short mm) {RLC_r(&CARTRIDGE::BUFFER[mm]);}

    void RR_r(unsigned char *r)
    {
        unsigned char c = REG.FLAG_C();
        REG.FLAG_C(*r&0b00000001);
        *r >>= 1;
        *r |= c<<7;
        REG.FLAG_Z(!*r);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void RR_mm(unsigned short mm)  {RR_r(&CARTRIDGE::BUFFER[mm]);}        

    void SLA_r(unsigned char *r)
    {
        REG.FLAG_C(*r&0b10000000);
        *r <<= 1;
        REG.FLAG_Z(!*r);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void SLA_mm(unsigned short mm) {SLA_r(&CARTRIDGE::BUFFER[mm]);}

    void SRA_r(unsigned char *r)
    {
        REG.FLAG_C(*r&0b00000001);
        *r >>= 1;
        *r |= ((*r&0b01000000)<<1);
        REG.FLAG_Z(!*r);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void SRA_mm(unsigned short mm) {SRA_r(&CARTRIDGE::BUFFER[mm]);}

    void SRL_r(unsigned char *r)
    {
        REG.FLAG_C(*r&0b00000001);
        *r >>= 1;
        REG.FLAG_Z(!*r);
        REG.FLAG_N(0);
        REG.FLAG_H(0);
    }
    void SRL_mm(unsigned short mm) {SRL_r(&CARTRIDGE::BUFFER[mm]);}

    void BIT_b_r(unsigned char bit, unsigned char reg)
    {
        REG.FLAG_Z(! (reg&(1<<bit)) );
        REG.FLAG_N(0);
        REG.FLAG_H(1);
    }
    void BIT_b_mm(unsigned char bit, unsigned short mm)
    {
        REG.FLAG_Z(! CARTRIDGE::BUFFER[mm]&(1<<bit) );
        REG.FLAG_N(0);
        REG.FLAG_H(1);
    }

    void SET_b_r (unsigned char bit, unsigned char *reg) {*reg = *reg | (1<<bit);}
    void SET_b_mm(unsigned char bit, unsigned short mm)  {CARTRIDGE::BUFFER[mm] |= (1<<bit);}

    void RES_b_r (unsigned char bit, unsigned char *reg) {*reg = *reg & ~(1<<bit);}
    void RES_b_mm(unsigned char bit, unsigned short mm)  {CARTRIDGE::BUFFER[mm] &= ~(1<<bit);}

    void JP_mm   (unsigned short mm) {REG.PC = mm;}
    void JP_Z_mm (unsigned short mm) {if(REG.FLAG_Z())  JP_mm(mm);}
    void JP_NZ_mm(unsigned short mm) {if(!REG.FLAG_Z()) JP_mm(mm);}
    void JP_C_mm (unsigned short mm) {if(REG.FLAG_C())  JP_mm(mm); }
    void JP_NC_mm(unsigned short mm){if(!REG.FLAG_C())JP_mm(mm);}        

    void JR_n   (char n) {REG.PC += n;}  
    void JR_Z_n (char n) {if(REG.FLAG_Z())  JR_n(n);}
    void JR_NZ_n(char n) {if(!REG.FLAG_Z()) JR_n(n);}
    void JR_C_n (char n) {if(REG.FLAG_C())JR_n(n);  }
    void JR_NC_n(char n){if(!REG.FLAG_C()) JR_n(n); }       

    void CALL_mm(unsigned short nn)
    {
        PUSH_rr(&REG.PC);
        JP_mm(nn);
    }
    void CALL_Z_mm(unsigned short  nn) {if(REG.FLAG_Z()) {PUSH_rr(&REG.PC);JP_Z_mm(nn); }}
    void CALL_NZ_mm(unsigned short nn) {if(!REG.FLAG_Z()){PUSH_rr(&REG.PC);JP_NZ_mm(nn);}}
    void CALL_C_mm(unsigned short nn)  {if(REG.FLAG_C()) {PUSH_rr(&REG.PC);JP_C_mm(nn); }}
    void CALL_NC_mm(unsigned short nn) {if(!REG.FLAG_C()){PUSH_rr(&REG.PC);JP_NC_mm(nn);}}                

    void RET()
    {
        unsigned short mm;
        POP_rr(&mm);
        JP_mm(mm);
    }
    void RET_Z()  {if(REG.FLAG_Z()) RET();}
    void RET_NZ() {if(!REG.FLAG_Z())RET();}
    void RET_C()  {if(REG.FLAG_C()) RET();}
    void RET_NC() {if(!REG.FLAG_C())RET();}                
    
    void RETI()
    {
        EI();
        RET();
    }   
    void RST_n(unsigned char n)
    {
        PUSH_rr(&REG.PC);
        JP_mm(n);
    }
    
    /*
     * Check if the instructions and parameters are correct!
    */
    const struct INSTRUCTION INSTRUCTIONS[512] = 
    {
        {"NOP", 1,          [](){  NOP();                        }},
        {"LD BC,%04x", 3,   [](){  LD_rr_nn(&REG.BC, READ16());  }},
        {"LD (BC),A", 1,    [](){  LD_mm_r(REG.BC, &REG.A);      }},
        {"INC BC", 1,       [](){  INC_rr(&REG.BC);              }},
        {"INC B", 1,        [](){  INC_r(&REG.B);                }},
        {"DEC B", 1,        [](){  DEC_r(&REG.B);                }},
        {"LD B,%02x", 2,    [](){  LD_r_n(&REG.B, READ8());      }},
        {"RLC A", 1,        [](){  RLC_r(&REG.A);                }},
        {"LD (%04x),SP", 3, [](){  LD_mm_SP(READ16());           }},
        {"ADD HL,BC", 1,    [](){  ADD_rr_rr(&REG.HL, &REG.BC);  }},
        {"LD A,(BC)", 1,    [](){  LD_r_mm(&REG.A, REG.BC);      }},
        {"DEC BC", 1,       [](){  DEC_rr(&REG.BC);              }},
        {"INC C", 1,        [](){  INC_r(&REG.C);                }},
        {"DEC C", 1,        [](){  DEC_r(&REG.C);                }},
        {"LD C,%02x", 2,    [](){  LD_r_n(&REG.C, READ8());      }},
        {"RRC A", 1,        [](){  RRC_r(&REG.A);                }},
        {"STOP", 1,         [](){  STOP();                       }},
        {"LD DE,%04x", 3,   [](){  LD_rr_nn(&REG.DE, READ16());  }},
        {"LD (DE),A", 1,    [](){  LD_mm_r(REG.DE, &REG.A);      }},
        {"INC DE", 1,       [](){  INC_rr(&REG.DE);              }},
        {"INC D", 1,        [](){  INC_r(&REG.D);                }},
        {"DEC D", 1,        [](){  DEC_r(&REG.D);                }},
        {"LD D,%02x", 2,    [](){  LD_r_n(&REG.D, READ8());      }},
        {"RL A", 1,         [](){  RL_r(&REG.A);                 }},
        {"JR %03d", 2,      [](){  JR_n(READ8());                }},
        {"ADD HL,DE", 1,    [](){  ADD_rr_rr(&REG.HL, &REG.DE);  }},
        {"LD A,(DE)", 1,    [](){  LD_r_mm(&REG.A, REG.DE);      }},
        {"DEC DE", 1,       [](){  DEC_rr(&REG.DE);              }},
        {"INC E", 1,        [](){  INC_r(&REG.E);                }},
        {"DEC E", 1,        [](){  DEC_r(&REG.E);                }},
        {"LD E,%03d", 2,    [](){  LD_r_n(&REG.E, READ8());      }},
        {"RR A", 1,         [](){  RR_r(&REG.A);                 }},
        {"JR NZ,%03d", 2,   [](){  JR_NZ_n(READ8());             }},
        {"LD HL,%04x", 3,   [](){  LD_rr_nn(&REG.HL, READ16());  }},
        {"LDI (HL),A", 1,   [](){  LD_mm_r(REG.HL, &REG.A);REG.HL++;}},
        {"INC HL", 1,       [](){  INC_rr(&REG.HL);              }},
        {"INC H", 1,        [](){  INC_r(&REG.H);                }},
        {"DEC H", 1,        [](){  DEC_r(&REG.H);                }},
        {"LD H,%02x", 2,    [](){  LD_r_n(&REG.H, READ8());      }},
        {"DAA", 1,          [](){  DAA();                        }},
        {"JR Z,%03d", 2,    [](){  JR_Z_n(READ8());              }},
        {"ADD HL,HL", 1,    [](){  ADD_rr_rr(&REG.HL, &REG.HL);  }},
        {"LDI A,(HL)", 1,   [](){  LD_r_mm(&REG.A, REG.HL);REG.HL++;}},
        {"DEC HL", 1,       [](){  DEC_rr(&REG.HL);              }},
        {"INC L", 1,        [](){  INC_r(&REG.L);                }},
        {"DEC L", 1,        [](){  DEC_r(&REG.L);                }},
        {"LD L,%02x", 2,    [](){  LD_r_n(&REG.L, READ8());      }},
        {"CPL", 1,          [](){  CPL();                        }},
        {"JR NC,%03d", 2,   [](){  JR_NC_n(READ8());             }},
        {"LD SP,%04x", 3,   [](){  LD_rr_nn(&REG.SP, READ16());  }},
        {"LDD (HL),A", 1,   [](){  LD_mm_r(REG.HL, &REG.A); REG.HL--;}},
        {"INC SP", 1,       [](){  INC_rr(&REG.SP);              }},
        {"INC (HL)", 1,     [](){  INC_mm(REG.HL);               }},
        {"DEC (HL)", 1,     [](){  DEC_mm(REG.HL);               }},
        {"LD (HL),%02x", 2, [](){  LD_mm_n(REG.HL, READ8());     }},
        {"SCF", 1,          [](){  SCF();                        }},
        {"JR C,%03d", 2,    [](){  JR_C_n(READ8());              }},
        {"ADD HL,SP", 1,    [](){  ADD_rr_rr(&REG.HL, &REG.SP);  }},
        {"LDD A,(HL)", 1,   [](){  LD_r_mm(&REG.A, REG.HL); REG.HL--;}},
        {"DEC SP", 1,       [](){  DEC_rr(&REG.SP);              }},
        {"INC A", 1,        [](){  INC_r(&REG.A);                }},
        {"DEC A", 1,        [](){  DEC_r(&REG.A);                }},
        {"LD A,%02x", 2,    [](){  LD_r_n(&REG.A, READ8());      }},
        {"CCF", 1,          [](){  CCF();                        }},

        {"LD B,B", 1,       [](){  LD_r_r(&REG.B, &REG.B);       }},
        {"LD B,C", 1,       [](){  LD_r_r(&REG.B, &REG.C);       }},
        {"LD B,D", 1,       [](){  LD_r_r(&REG.B, &REG.D);       }},
        {"LD B,E", 1,       [](){  LD_r_r(&REG.B, &REG.E);       }},
        {"LD B,H", 1,       [](){  LD_r_r(&REG.B, &REG.H);       }},
        {"LD B,L", 1,       [](){  LD_r_r(&REG.B, &REG.L);       }},
        {"LD B,(HL)", 1,    [](){  LD_r_mm(&REG.B, REG.HL);      }},
        {"LD B,A", 1,       [](){  LD_r_r(&REG.B, &REG.A);       }},

        {"LD C,B", 1,       [](){  LD_r_r(&REG.C, &REG.B);       }},
        {"LD C,C", 1,       [](){  LD_r_r(&REG.C, &REG.C);       }},
        {"LD C,D", 1,       [](){  LD_r_r(&REG.C, &REG.D);       }},
        {"LD C,E", 1,       [](){  LD_r_r(&REG.C, &REG.E);       }},
        {"LD C,H", 1,       [](){  LD_r_r(&REG.C, &REG.H);       }},
        {"LD C,L", 1,       [](){  LD_r_r(&REG.C, &REG.L);       }},
        {"LD C,(HL)", 1,    [](){  LD_r_mm(&REG.C, REG.HL);      }},
        {"LD C,A", 1,       [](){  LD_r_r(&REG.C, &REG.A);       }},

        {"LD D,B", 1,       [](){  LD_r_r(&REG.D, &REG.B);       }},
        {"LD D,C", 1,       [](){  LD_r_r(&REG.D, &REG.C);       }},
        {"LD D,D", 1,       [](){  LD_r_r(&REG.D, &REG.D);       }},
        {"LD D,E", 1,       [](){  LD_r_r(&REG.D, &REG.E);       }},
        {"LD D,H", 1,       [](){  LD_r_r(&REG.D, &REG.H);       }},
        {"LD D,L", 1,       [](){  LD_r_r(&REG.D, &REG.L);       }},
        {"LD D,(HL)", 1,    [](){  LD_r_mm(&REG.D, REG.HL);      }},
        {"LD D,A", 1,       [](){  LD_r_r(&REG.D, &REG.A);       }},

        {"LD E,B", 1,       [](){  LD_r_r(&REG.E, &REG.B);       }},
        {"LD E,C", 1,       [](){  LD_r_r(&REG.E, &REG.C);       }},
        {"LD E,D", 1,       [](){  LD_r_r(&REG.E, &REG.D);       }},
        {"LD E,E", 1,       [](){  LD_r_r(&REG.E, &REG.E);       }},
        {"LD E,H", 1,       [](){  LD_r_r(&REG.E, &REG.H);       }},
        {"LD E,L", 1,       [](){  LD_r_r(&REG.E, &REG.L);       }},
        {"LD E,(HL)", 1,    [](){  LD_r_mm(&REG.E, REG.HL);      }},
        {"LD E,A", 1,       [](){  LD_r_r(&REG.E, &REG.A);       }},

        {"LD H,B", 1,       [](){  LD_r_r(&REG.H, &REG.B);       }},
        {"LD H,C", 1,       [](){  LD_r_r(&REG.H, &REG.C);       }},
        {"LD H,D", 1,       [](){  LD_r_r(&REG.H, &REG.D);       }},
        {"LD H,E", 1,       [](){  LD_r_r(&REG.H, &REG.E);       }},
        {"LD H,H", 1,       [](){  LD_r_r(&REG.H, &REG.H);       }},
        {"LD H,L", 1,       [](){  LD_r_r(&REG.H, &REG.L);       }},
        {"LD H,(HL)", 1,    [](){  LD_r_mm(&REG.H, REG.HL);      }},
        {"LD H,A", 1,       [](){  LD_r_r(&REG.H, &REG.A);       }},

        {"LD L,B", 1,       [](){  LD_r_r(&REG.L, &REG.B);       }},
        {"LD L,C", 1,       [](){  LD_r_r(&REG.L, &REG.C);       }},
        {"LD L,D", 1,       [](){  LD_r_r(&REG.L, &REG.D);       }},
        {"LD L,E", 1,       [](){  LD_r_r(&REG.L, &REG.E);       }},
        {"LD L,H", 1,       [](){  LD_r_r(&REG.L, &REG.H);       }},
        {"LD L,L", 1,       [](){  LD_r_r(&REG.L, &REG.L);       }},
        {"LD L,(HL)", 1,    [](){  LD_r_mm(&REG.L, REG.HL);      }},
        {"LD L,A", 1,       [](){  LD_r_r(&REG.L, &REG.A);       }},

        {"LD (HL),B", 1,    [](){  LD_mm_r(REG.HL, &REG.B);      }},
        {"LD (HL),C", 1,    [](){  LD_mm_r(REG.HL, &REG.C);      }},
        {"LD (HL),D", 1,    [](){  LD_mm_r(REG.HL, &REG.D);      }},
        {"LD (HL),E", 1,    [](){  LD_mm_r(REG.HL, &REG.E);      }},
        {"LD (HL),H", 1,    [](){  LD_mm_r(REG.HL, &REG.H);      }},
        {"LD (HL),L", 1,    [](){  LD_mm_r(REG.HL, &REG.L);      }},
        {"HALT", 1,         [](){  HALT(); }},
        {"LD (HL),A", 1,    [](){  LD_mm_r(REG.HL, &REG.A);      }},

        {"LD A,B", 1,       [](){  LD_r_r(&REG.A, &REG.B);       }},
        {"LD A,C", 1,       [](){  LD_r_r(&REG.A, &REG.C);       }},
        {"LD A,D", 1,       [](){  LD_r_r(&REG.A, &REG.D);       }},
        {"LD A,E", 1,       [](){  LD_r_r(&REG.A, &REG.E);       }},
        {"LD A,H", 1,       [](){  LD_r_r(&REG.A, &REG.H);       }},
        {"LD A,L", 1,       [](){  LD_r_r(&REG.A, &REG.L);       }},
        {"LD A,(HL)", 1,    [](){  LD_r_mm(&REG.A, REG.HL);      }},
        {"LD A,A", 1,       [](){  LD_r_r(&REG.A, &REG.A);       }},

        {"ADD A,B", 1,      [](){  ADD_r_r(&REG.A, &REG.B);      }},
        {"ADD A,C", 1,      [](){  ADD_r_r(&REG.A, &REG.C);      }},
        {"ADD A,D", 1,      [](){  ADD_r_r(&REG.A, &REG.D);      }},
        {"ADD A,E", 1,      [](){  ADD_r_r(&REG.A, &REG.E);      }},
        {"ADD A,H", 1,      [](){  ADD_r_r(&REG.A, &REG.H);      }},
        {"ADD A,L", 1,      [](){  ADD_r_r(&REG.A, &REG.L);      }},
        {"ADD A,(HL)", 1,   [](){  ADD_r_mm(&REG.A, REG.HL);     }},
        {"ADD A,A", 1,      [](){  ADD_r_r(&REG.A, &REG.A);      }},

        {"ADC A,B", 1,      [](){  ADC_r_r(&REG.A, &REG.B);      }},
        {"ADC A,C", 1,      [](){  ADC_r_r(&REG.A, &REG.C);      }},
        {"ADC A,D", 1,      [](){  ADC_r_r(&REG.A, &REG.D);      }},
        {"ADC A,E", 1,      [](){  ADC_r_r(&REG.A, &REG.E);      }},
        {"ADC A,H", 1,      [](){  ADC_r_r(&REG.A, &REG.H);      }},
        {"ADC A,L", 1,      [](){  ADC_r_r(&REG.A, &REG.L);      }},
        {"ADC A,(HL)", 1,   [](){  ADC_r_mm(&REG.A, REG.HL);     }},
        {"ADC A,A", 1,      [](){  ADC_r_r(&REG.A, &REG.A);      }},

        {"SUB A,B", 1,      [](){  SUB_r_r(&REG.A, &REG.B);      }},
        {"SUB A,C", 1,      [](){  SUB_r_r(&REG.A, &REG.C);      }},
        {"SUB A,D", 1,      [](){  SUB_r_r(&REG.A, &REG.D);      }},
        {"SUB A,E", 1,      [](){  SUB_r_r(&REG.A, &REG.E);      }},
        {"SUB A,H", 1,      [](){  SUB_r_r(&REG.A, &REG.H);      }},
        {"SUB A,L", 1,      [](){  SUB_r_r(&REG.A, &REG.L);      }},
        {"SUB A,(HL)", 1,   [](){  SUB_r_mm(&REG.A, REG.HL);     }},
        {"SUB A,A", 1,      [](){  SUB_r_r(&REG.A, &REG.A);      }},

        {"SBC A,B", 1,      [](){  SBC_r_r(&REG.A, &REG.B);      }},
        {"SBC A,C", 1,      [](){  SBC_r_r(&REG.A, &REG.C);      }},
        {"SBC A,D", 1,      [](){  SBC_r_r(&REG.A, &REG.D);      }},
        {"SBC A,E", 1,      [](){  SBC_r_r(&REG.A, &REG.E);      }},
        {"SBC A,H", 1,      [](){  SBC_r_r(&REG.A, &REG.H);      }},
        {"SBC A,L", 1,      [](){  SBC_r_r(&REG.A, &REG.L);      }},
        {"SBC A,(HL)", 1,   [](){  SBC_r_mm(&REG.A, REG.HL);     }},
        {"SBC A,A", 1,      [](){  SBC_r_r(&REG.A, &REG.A);      }},

        {"AND B", 1,        [](){  AND_r_r(&REG.A, &REG.B);      }},
        {"AND C", 1,        [](){  AND_r_r(&REG.A, &REG.C);      }},
        {"AND D", 1,        [](){  AND_r_r(&REG.A, &REG.D);      }},
        {"AND E", 1,        [](){  AND_r_r(&REG.A, &REG.E);      }},
        {"AND H", 1,        [](){  AND_r_r(&REG.A, &REG.H);      }},
        {"AND L", 1,        [](){  AND_r_r(&REG.A, &REG.L);      }},
        {"AND (HL)", 1,     [](){  AND_r_mm(&REG.A, REG.HL);     }},
        {"AND A", 1,        [](){  AND_r_r(&REG.A, &REG.A);      }},

        {"XOR B", 1,        [](){  XOR_r_r(&REG.A, &REG.B);      }},
        {"XOR C", 1,        [](){  XOR_r_r(&REG.A, &REG.C);      }},
        {"XOR D", 1,        [](){  XOR_r_r(&REG.A, &REG.D);      }},
        {"XOR E", 1,        [](){  XOR_r_r(&REG.A, &REG.E);      }},
        {"XOR H", 1,        [](){  XOR_r_r(&REG.A, &REG.H);      }},
        {"XOR L", 1,        [](){  XOR_r_r(&REG.A, &REG.L);      }},
        {"XOR (HL)", 1,     [](){  XOR_r_mm(&REG.A, REG.HL);     }},
        {"XOR A", 1,        [](){  XOR_r_r(&REG.A, &REG.A);      }},

        {"OR B", 1,         [](){  OR_r_r(&REG.A, &REG.B);       }},
        {"OR C", 1,         [](){  OR_r_r(&REG.A, &REG.C);       }},
        {"OR D", 1,         [](){  OR_r_r(&REG.A, &REG.D);       }},
        {"OR E", 1,         [](){  OR_r_r(&REG.A, &REG.E);       }},
        {"OR H", 1,         [](){  OR_r_r(&REG.A, &REG.H);       }},
        {"OR L", 1,         [](){  OR_r_r(&REG.A, &REG.L);       }},
        {"OR (HL)", 1,      [](){  OR_r_mm(&REG.A, REG.HL);      }},
        {"OR A", 1,         [](){  OR_r_r(&REG.A, &REG.A);       }},

        {"CP B", 1,         [](){  CP_r_r(&REG.A, &REG.B);       }},
        {"CP C", 1,         [](){  CP_r_r(&REG.A, &REG.C);       }},
        {"CP D", 1,         [](){  CP_r_r(&REG.A, &REG.D);       }},
        {"CP E", 1,         [](){  CP_r_r(&REG.A, &REG.E);       }},
        {"CP H", 1,         [](){  CP_r_r(&REG.A, &REG.H);       }},
        {"CP L", 1,         [](){  CP_r_r(&REG.A, &REG.L);       }},
        {"CP (HL)", 1,      [](){  CP_r_mm(&REG.A, REG.HL);      }},
        {"CP A", 1,         [](){  CP_r_r(&REG.A, &REG.A);       }},

        {"RET NZ", 1,       [](){  RET_NZ();                     }},
        {"POP BC", 1,       [](){  POP_rr(&REG.BC);              }},
        {"JP NZ,%04x", 3,   [](){  JP_NZ_mm(READ16());           }},
        {"JP %04x", 3,      [](){  JP_mm(READ16());              }},
        {"CALL NZ,%04x", 3, [](){  CALL_NZ_mm(READ16());         }},
        {"PUSH BC", 1,      [](){  PUSH_rr(&REG.BC);             }},
        {"ADD A,%02x", 2,   [](){  ADD_r_n(&REG.A, READ8());     }},
        {"RST 0", 1,        [](){  RST_n(0x0);                     }},
        {"RET Z", 1,        [](){  RET_Z();                      }},
        {"RET", 1,          [](){  RET();                        }},
        {"JP Z,%04x", 3,    [](){  JP_Z_mm(READ16());            }},
        {"0xCB ", 2,        [](){  NOP();                        }},
        {"CALL Z,%04x", 3,  [](){  CALL_Z_mm(READ16());          }},
        {"CALL %04x", 3,    [](){  CALL_mm(READ16());            }},
        {"ADC A,%02x", 2,   [](){  ADC_r_n(&REG.A, READ8());     }},
        {"RST 8", 1,        [](){  RST_n(0x8);                     }},
        {"RET NC", 1,       [](){  RET_NC();                     }},
        {"POP DE", 1,       [](){  POP_rr(&REG.DE);              }},
        {"JP NC,%04x", 3,   [](){  JP_NC_mm(READ16());           }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"CALL NC,%04x", 3, [](){  CALL_NC_mm(READ16());         }},
        {"PUSH DE", 1,      [](){  PUSH_rr(&REG.DE);             }},
        {"SUB A,%02x", 2,   [](){  SUB_r_n(&REG.A, READ8());     }},
        {"RST 10", 1,       [](){  RST_n(0x10);                    }},
        {"RET C", 1,        [](){  RET_C();                      }},
        {"RETI", 1,         [](){  RETI();                       }},
        {"JP C,%04x", 3,    [](){  JP_C_mm(READ16());            }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"CALL C,%04x", 3,  [](){  CALL_C_mm(READ16());          }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"SBC A,%02x", 2,   [](){  SBC_r_n(&REG.A, READ8());     }},
        {"RST 18", 1,       [](){  RST_n(0x18);                    }},
        {"LDH (%02x),A", 2, [](){  LD_mm_r(0xFF00+READ8(), &REG.A);}},
        {"POP HL", 1,       [](){  POP_rr(&REG.HL);              }},
        {"LDH (C),A", 1,    [](){  LD_mm_r(0xFF00+REG.C, &REG.A);}},
        {"XX", 1,           [](){  ERROR();                      }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"PUSH HL", 1,      [](){  PUSH_rr(&REG.HL);             }},
        {"AND %02x", 2,     [](){  AND_r_n(&REG.A, READ8());     }},
        {"RST 20", 1,       [](){  RST_n(0x20);                  }},
        {"ADD SP,%03d", 2,  [](){  ADD_rr_d(&REG.SP, READ8());   }},
        {"JP (HL)", 1,      [](){  JP_mm(REG.HL);                }},
        {"LD (%04x),A", 3,  [](){  LD_mm_r(READ16(), &REG.A);    }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"XOR %02x", 2,     [](){  XOR_r_n(&REG.A, READ8());     }},
        {"RST 28", 1,       [](){  RST_n(0x28);                  }},
        {"LDH A,(%02x)", 2, [](){  LD_r_mm(&REG.A, READ8()+0xFF00);}},
        {"POP AF", 1,       [](){  POP_rr(&REG.AF);              }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"DI", 1,           [](){  DI();                         }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"PUSH AF", 1,      [](){  PUSH_rr(&REG.AF);             }},
        {"OR %02x", 2,      [](){  OR_r_n(&REG.A, READ8());      }},
        {"RST 30", 1,       [](){  RST_n(0x30);                  }},
        {"LD HL,SP+%03d", 2,[](){  LDHL_SP_d(READ8());           }},
        {"LD SP,HL", 1,     [](){  LD_rr_rr(&REG.SP, &REG.HL);   }},
        {"LD A,(%04x)", 3,  [](){  LD_r_mm(&REG.A, READ16());    }},
        {"EI", 1,           [](){  EI();                         }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"XX", 1,           [](){  ERROR();                      }},
        {"CP %02x", 2,      [](){  CP_r_n(&REG.A, READ8());      }},
        {"RST 38", 1,       [](){  RST_n(0x38);                  }},
    
        //>>0xCB
        {"RLC B", 1,        [](){   RLC_r(&REG.B);   }},
        {"RLC C", 1, 	    [](){   RLC_r(&REG.C);   }},
        {"RLC D", 1,	    [](){   RLC_r(&REG.D);   }},
        {"RLC E", 1, 	    [](){   RLC_r(&REG.E);   }},
        {"RLC H", 1, 	    [](){   RLC_r(&REG.H);   }},
        {"RLC L", 1, 	    [](){   RLC_r(&REG.L);   }},
        {"RLC (HL)", 1,     [](){   RLC_mm(REG.HL);  }},
        {"RLC A", 1, 	    [](){   RLC_r(&REG.A);   }},

        {"RRC B", 1,        [](){   RRC_r(&REG.B);   }},
        {"RRC C", 1, 	    [](){   RRC_r(&REG.C);   }},
        {"RRC D", 1,	    [](){   RRC_r(&REG.D);   }},
        {"RRC E", 1, 	    [](){   RRC_r(&REG.E);   }},
        {"RRC H", 1, 	    [](){   RRC_r(&REG.H);   }},
        {"RRC L", 1, 	    [](){   RRC_r(&REG.L);   }},
        {"RRC (HL)", 1,     [](){   RRC_mm(REG.HL);  }},
        {"RRC A", 1, 	    [](){   RRC_r(&REG.A);   }},
        
        {"RL B", 1,         [](){   RL_r(&REG.B);    }},
        {"RL C", 1, 	    [](){   RL_r(&REG.C);    }},
        {"RL D", 1,	        [](){   RL_r(&REG.D);    }},
        {"RL E", 1, 	    [](){   RL_r(&REG.E);    }},
        {"RL H", 1, 	    [](){   RL_r(&REG.H);    }},
        {"RL L", 1, 	    [](){   RL_r(&REG.L);    }},
        {"RL (HL)", 1,      [](){   RL_mm(REG.HL);   }},
        {"RL A", 1, 	    [](){   RL_r(&REG.A);    }},

        {"RR B", 1,         [](){   RR_r(&REG.B);    }},
        {"RR C", 1, 	    [](){   RR_r(&REG.C);    }},
        {"RR D", 1,	        [](){   RR_r(&REG.D);    }},
        {"RR E", 1, 	    [](){   RR_r(&REG.E);    }},
        {"RR H", 1, 	    [](){   RR_r(&REG.H);    }},
        {"RR L", 1, 	    [](){   RR_r(&REG.L);    }},
        {"RR (HL)", 1,      [](){   RR_mm(REG.HL);   }},
        {"RR A", 1, 	    [](){   RR_r(&REG.A);    }},

        {"SLA B", 1,        [](){   SLA_r(&REG.B);   }},
        {"SLA C", 1, 	    [](){   SLA_r(&REG.C);   }},
        {"SLA D", 1,	    [](){   SLA_r(&REG.D);   }},
        {"SLA E", 1, 	    [](){   SLA_r(&REG.E);   }},
        {"SLA H", 1, 	    [](){   SLA_r(&REG.H);   }},
        {"SLA L", 1, 	    [](){   SLA_r(&REG.L);   }},
        {"SLA (HL)", 1,     [](){   SLA_mm(REG.HL);  }},
        {"SLA A", 1, 	    [](){   SLA_r(&REG.A);   }},

        {"SRA B", 1,        [](){   SRA_r(&REG.B);   }},
        {"SRA C", 1, 	    [](){   SRA_r(&REG.C);   }},
        {"SRA D", 1,	    [](){   SRA_r(&REG.D);   }},
        {"SRA E", 1, 	    [](){   SRA_r(&REG.E);   }},
        {"SRA H", 1, 	    [](){   SRA_r(&REG.H);   }},
        {"SRA L", 1, 	    [](){   SRA_r(&REG.L);   }},
        {"SRA (HL)", 1,     [](){   SRA_mm(REG.HL);  }},
        {"SRA A", 1, 	    [](){   SRA_r(&REG.A);   }},

        {"SWAP B", 1,       [](){   SWAP_r(&REG.B);  }},
        {"SWAP C", 1, 	    [](){   SWAP_r(&REG.C);  }},
        {"SWAP D", 1,	    [](){   SWAP_r(&REG.D);  }},
        {"SWAP E", 1, 	    [](){   SWAP_r(&REG.E);  }},
        {"SWAP H", 1, 	    [](){   SWAP_r(&REG.H);  }},
        {"SWAP L", 1, 	    [](){   SWAP_r(&REG.L);  }},
        {"SWAP (HL)", 1,    [](){   SWAP_mm(REG.HL); }},
        {"SWAP A", 1, 	    [](){   SWAP_r(&REG.A);  }},

        {"SRL B", 1,        [](){   SRL_r(&REG.B);   }},
        {"SRL C", 1, 	    [](){   SRL_r(&REG.C);   }},
        {"SRL D", 1,	    [](){   SRL_r(&REG.D);   }},
        {"SRL E", 1, 	    [](){   SRL_r(&REG.E);   }},
        {"SRL H", 1, 	    [](){   SRL_r(&REG.H);   }},
        {"SRL L", 1, 	    [](){   SRL_r(&REG.L);   }},
        {"SRL (HL)", 1,     [](){   SRL_mm(REG.HL);  }},
        {"SRL A", 1, 	    [](){   SRL_r(&REG.A);   }},

        {"BIT 0,B", 1,  	[](){   BIT_b_r(0,REG.B);   }},
        {"BIT 0,C", 1,  	[](){   BIT_b_r(0,REG.C);   }},
        {"BIT 0,D", 1,   	[](){   BIT_b_r(0,REG.D);   }},
        {"BIT 0,E", 1,  	[](){   BIT_b_r(0,REG.E);   }},
        {"BIT 0,H", 1,  	[](){   BIT_b_r(0,REG.H);   }},
        {"BIT 0,L", 1,   	[](){   BIT_b_r(0,REG.L);   }},
        {"BIT 0,(HL)", 1,   [](){   BIT_b_mm(0,REG.HL); }},
        {"BIT 0,A", 1,  	[](){   BIT_b_r(0,REG.A);   }},

        {"BIT 1,B", 1,  	[](){   BIT_b_r(1,REG.B);   }},
        {"BIT 1,C", 1,  	[](){   BIT_b_r(1,REG.C);   }},
        {"BIT 1,D", 1,   	[](){   BIT_b_r(1,REG.D);   }},
        {"BIT 1,E", 1,  	[](){   BIT_b_r(1,REG.E);   }},
        {"BIT 1,H", 1,  	[](){   BIT_b_r(1,REG.H);   }},
        {"BIT 1,L", 1,   	[](){   BIT_b_r(1,REG.L);   }},
        {"BIT 1,(HL)", 1,   [](){   BIT_b_mm(1,REG.HL); }},
        {"BIT 1,A", 1,  	[](){   BIT_b_r(1,REG.A);   }},

        {"BIT 2,B", 1,  	[](){   BIT_b_r(2,REG.B);   }},
        {"BIT 2,C", 1,  	[](){   BIT_b_r(2,REG.C);   }},
        {"BIT 2,D", 1,   	[](){   BIT_b_r(2,REG.D);   }},
        {"BIT 2,E", 1,  	[](){   BIT_b_r(2,REG.E);   }},
        {"BIT 2,H", 1,  	[](){   BIT_b_r(2,REG.H);   }},
        {"BIT 2,L", 1,   	[](){   BIT_b_r(2,REG.L);   }},
        {"BIT 2,(HL)", 1,   [](){   BIT_b_mm(2,REG.HL); }},
        {"BIT 2,A", 1,  	[](){   BIT_b_r(2,REG.A);   }},

        {"BIT 3,B", 1,  	[](){   BIT_b_r(3,REG.B);   }},
        {"BIT 3,C", 1,  	[](){   BIT_b_r(3,REG.C);   }},
        {"BIT 3,D", 1,   	[](){   BIT_b_r(3,REG.D);   }},
        {"BIT 3,E", 1,  	[](){   BIT_b_r(3,REG.E);   }},
        {"BIT 3,H", 1,  	[](){   BIT_b_r(3,REG.H);   }},
        {"BIT 3,L", 1,   	[](){   BIT_b_r(3,REG.L);   }},
        {"BIT 3,(HL)", 1,   [](){   BIT_b_mm(3,REG.HL); }},
        {"BIT 3,A", 1,  	[](){   BIT_b_r(3,REG.A);   }},

        {"BIT 4,B", 1,  	[](){   BIT_b_r(4,REG.B);   }},
        {"BIT 4,C", 1,  	[](){   BIT_b_r(4,REG.C);   }},
        {"BIT 4,D", 1,   	[](){   BIT_b_r(4,REG.D);   }},
        {"BIT 4,E", 1,  	[](){   BIT_b_r(4,REG.E);   }},
        {"BIT 4,H", 1,  	[](){   BIT_b_r(4,REG.H);   }},
        {"BIT 4,L", 1,   	[](){   BIT_b_r(4,REG.L);   }},
        {"BIT 4,(HL)", 1,   [](){   BIT_b_mm(4,REG.HL); }},
        {"BIT 4,A", 1,  	[](){   BIT_b_r(4,REG.A);   }},

        {"BIT 5,B", 1,  	[](){   BIT_b_r(5,REG.B);   }},
        {"BIT 5,C", 1,  	[](){   BIT_b_r(5,REG.C);   }},
        {"BIT 5,D", 1,   	[](){   BIT_b_r(5,REG.D);   }},
        {"BIT 5,E", 1,  	[](){   BIT_b_r(5,REG.E);   }},
        {"BIT 5,H", 1,  	[](){   BIT_b_r(5,REG.H);   }},
        {"BIT 5,L", 1,   	[](){   BIT_b_r(5,REG.L);   }},
        {"BIT 5,(HL)", 1,   [](){   BIT_b_mm(5,REG.HL); }},
        {"BIT 5,A", 1,  	[](){   BIT_b_r(5,REG.A);   }},

        {"BIT 6,B", 1,  	[](){   BIT_b_r(6,REG.B);   }},
        {"BIT 6,C", 1,  	[](){   BIT_b_r(6,REG.C);   }},
        {"BIT 6,D", 1,   	[](){   BIT_b_r(6,REG.D);   }},
        {"BIT 6,E", 1,  	[](){   BIT_b_r(6,REG.E);   }},
        {"BIT 6,H", 1,  	[](){   BIT_b_r(6,REG.H);   }},
        {"BIT 6,L", 1,   	[](){   BIT_b_r(6,REG.L);   }},
        {"BIT 6,(HL)", 1,   [](){   BIT_b_mm(6,REG.HL); }},
        {"BIT 6,A", 1,  	[](){   BIT_b_r(6,REG.A);   }},

        {"BIT 7,B", 1,  	[](){   BIT_b_r(7,REG.B);   }},
        {"BIT 7,C", 1,  	[](){   BIT_b_r(7,REG.C);   }},
        {"BIT 7,D", 1,   	[](){   BIT_b_r(7,REG.D);   }},
        {"BIT 7,E", 1,  	[](){   BIT_b_r(7,REG.E);   }},
        {"BIT 7,H", 1,  	[](){   BIT_b_r(7,REG.H);   }},
        {"BIT 7,L", 1,   	[](){   BIT_b_r(7,REG.L);   }},
        {"BIT 7,(HL)", 1,   [](){   BIT_b_mm(7,REG.HL); }},
        {"BIT 7,A", 1,  	[](){   BIT_b_r(7,REG.A);   }},

        {"RES 0,B", 1,  	[](){   RES_b_r(0,&REG.B);   }},
        {"RES 0,C", 1,  	[](){   RES_b_r(0,&REG.C);   }},
        {"RES 0,D", 1,   	[](){   RES_b_r(0,&REG.D);   }},
        {"RES 0,E", 1,  	[](){   RES_b_r(0,&REG.E);   }},
        {"RES 0,H", 1,  	[](){   RES_b_r(0,&REG.H);   }},
        {"RES 0,L", 1,   	[](){   RES_b_r(0,&REG.L);   }},
        {"RES 0,(HL)", 1,   [](){   RES_b_mm(0,REG.HL);  }},
        {"RES 0,A", 1,  	[](){   RES_b_r(0,&REG.A);   }},

        {"RES 1,B", 1,  	[](){   RES_b_r(1,&REG.B);   }},
        {"RES 1,C", 1,  	[](){   RES_b_r(1,&REG.C);   }},
        {"RES 1,D", 1,   	[](){   RES_b_r(1,&REG.D);   }},
        {"RES 1,E", 1,  	[](){   RES_b_r(1,&REG.E);   }},
        {"RES 1,H", 1,  	[](){   RES_b_r(1,&REG.H);   }},
        {"RES 1,L", 1,   	[](){   RES_b_r(1,&REG.L);   }},
        {"RES 1,(HL)", 1,   [](){   RES_b_mm(1,REG.HL);  }},
        {"RES 1,A", 1,  	[](){   RES_b_r(1,&REG.A);   }},

        {"RES 2,B", 1,  	[](){   RES_b_r(2,&REG.B);   }},
        {"RES 2,C", 1,  	[](){   RES_b_r(2,&REG.C);   }},
        {"RES 2,D", 1,   	[](){   RES_b_r(2,&REG.D);   }},
        {"RES 2,E", 1,  	[](){   RES_b_r(2,&REG.E);   }},
        {"RES 2,H", 1,  	[](){   RES_b_r(2,&REG.H);   }},
        {"RES 2,L", 1,   	[](){   RES_b_r(2,&REG.L);   }},
        {"RES 2,(HL)", 1,   [](){   RES_b_mm(2,REG.HL);  }},
        {"RES 2,A", 1,  	[](){   RES_b_r(2,&REG.A);   }},

        {"RES 3,B", 1,  	[](){   RES_b_r(3,&REG.B);   }},
        {"RES 3,C", 1,  	[](){   RES_b_r(3,&REG.C);   }},
        {"RES 3,D", 1,   	[](){   RES_b_r(3,&REG.D);   }},
        {"RES 3,E", 1,  	[](){   RES_b_r(3,&REG.E);   }},
        {"RES 3,H", 1,  	[](){   RES_b_r(3,&REG.H);   }},
        {"RES 3,L", 1,   	[](){   RES_b_r(3,&REG.L);   }},
        {"RES 3,(HL)", 1,   [](){   RES_b_mm(3,REG.HL);  }},
        {"RES 3,A", 1,  	[](){   RES_b_r(3,&REG.A);   }},

        {"RES 4,B", 1,  	[](){   RES_b_r(4,&REG.B);   }},
        {"RES 4,C", 1,  	[](){   RES_b_r(4,&REG.C);   }},
        {"RES 4,D", 1,   	[](){   RES_b_r(4,&REG.D);   }},
        {"RES 4,E", 1,  	[](){   RES_b_r(4,&REG.E);   }},
        {"RES 4,H", 1,  	[](){   RES_b_r(4,&REG.H);   }},
        {"RES 4,L", 1,   	[](){   RES_b_r(4,&REG.L);   }},
        {"RES 4,(HL)", 1,   [](){   RES_b_mm(4,REG.HL);  }},
        {"RES 4,A", 1,  	[](){   RES_b_r(4,&REG.A);   }},

        {"RES 5,B", 1,  	[](){   RES_b_r(5,&REG.B);   }},
        {"RES 5,C", 1,  	[](){   RES_b_r(5,&REG.C);   }},
        {"RES 5,D", 1,   	[](){   RES_b_r(5,&REG.D);   }},
        {"RES 5,E", 1,  	[](){   RES_b_r(5,&REG.E);   }},
        {"RES 5,H", 1,  	[](){   RES_b_r(5,&REG.H);   }},
        {"RES 5,L", 1,   	[](){   RES_b_r(5,&REG.L);   }},
        {"RES 5,(HL)", 1,   [](){   RES_b_mm(5,REG.HL);  }},
        {"RES 5,A", 1,  	[](){   RES_b_r(5,&REG.A);   }},

        {"RES 6,B", 1,  	[](){   RES_b_r(6,&REG.B);   }},
        {"RES 6,C", 1,  	[](){   RES_b_r(6,&REG.C);   }},
        {"RES 6,D", 1,   	[](){   RES_b_r(6,&REG.D);   }},
        {"RES 6,E", 1,  	[](){   RES_b_r(6,&REG.E);   }},
        {"RES 6,H", 1,  	[](){   RES_b_r(6,&REG.H);   }},
        {"RES 6,L", 1,   	[](){   RES_b_r(6,&REG.L);   }},
        {"RES 6,(HL)", 1,   [](){   RES_b_mm(6,REG.HL);  }},
        {"RES 6,A", 1,  	[](){   RES_b_r(6,&REG.A);   }},

        {"RES 7,B", 1,  	[](){   RES_b_r(7,&REG.B);   }},
        {"RES 7,C", 1,  	[](){   RES_b_r(7,&REG.C);   }},
        {"RES 7,D", 1,   	[](){   RES_b_r(7,&REG.D);   }},
        {"RES 7,E", 1,  	[](){   RES_b_r(7,&REG.E);   }},
        {"RES 7,H", 1,  	[](){   RES_b_r(7,&REG.H);   }},
        {"RES 7,L", 1,   	[](){   RES_b_r(7,&REG.L);   }},
        {"RES 7,(HL)", 1,   [](){   RES_b_mm(7,REG.HL);  }},
        {"RES 7,A", 1,  	[](){   RES_b_r(7,&REG.A);   }},


        {"SET 0,B", 1,  	[](){   SET_b_r(0,&REG.B);   }},
        {"SET 0,C", 1,  	[](){   SET_b_r(0,&REG.C);   }},
        {"SET 0,D", 1,   	[](){   SET_b_r(0,&REG.D);   }},
        {"SET 0,E", 1,  	[](){   SET_b_r(0,&REG.E);   }},
        {"SET 0,H", 1,  	[](){   SET_b_r(0,&REG.H);   }},
        {"SET 0,L", 1,   	[](){   SET_b_r(0,&REG.L);   }},
        {"SET 0,(HL)", 1,   [](){   SET_b_mm(0,REG.HL);  }},
        {"SET 0,A", 1,  	[](){   SET_b_r(0,&REG.A);   }},

        {"SET 1,B", 1,  	[](){   SET_b_r(1,&REG.B);   }},
        {"SET 1,C", 1,  	[](){   SET_b_r(1,&REG.C);   }},
        {"SET 1,D", 1,   	[](){   SET_b_r(1,&REG.D);   }},
        {"SET 1,E", 1,  	[](){   SET_b_r(1,&REG.E);   }},
        {"SET 1,H", 1,  	[](){   SET_b_r(1,&REG.H);   }},
        {"SET 1,L", 1,   	[](){   SET_b_r(1,&REG.L);   }},
        {"SET 1,(HL)", 1,   [](){   SET_b_mm(1,REG.HL);  }},
        {"SET 1,A", 1,  	[](){   SET_b_r(1,&REG.A);   }},

        {"SET 2,B", 1,  	[](){   SET_b_r(2,&REG.B);   }},
        {"SET 2,C", 1,  	[](){   SET_b_r(2,&REG.C);   }},
        {"SET 2,D", 1,   	[](){   SET_b_r(2,&REG.D);   }},
        {"SET 2,E", 1,  	[](){   SET_b_r(2,&REG.E);   }},
        {"SET 2,H", 1,  	[](){   SET_b_r(2,&REG.H);   }},
        {"SET 2,L", 1,   	[](){   SET_b_r(2,&REG.L);   }},
        {"SET 2,(HL)", 1,   [](){   SET_b_mm(2,REG.HL);  }},
        {"SET 2,A", 1,  	[](){   SET_b_r(2,&REG.A);   }},

        {"SET 3,B", 1,  	[](){   SET_b_r(3,&REG.B);   }},
        {"SET 3,C", 1,  	[](){   SET_b_r(3,&REG.C);   }},
        {"SET 3,D", 1,   	[](){   SET_b_r(3,&REG.D);   }},
        {"SET 3,E", 1,  	[](){   SET_b_r(3,&REG.E);   }},
        {"SET 3,H", 1,  	[](){   SET_b_r(3,&REG.H);   }},
        {"SET 3,L", 1,   	[](){   SET_b_r(3,&REG.L);   }},
        {"SET 3,(HL)", 1,   [](){   SET_b_mm(3,REG.HL);  }},
        {"SET 3,A", 1,  	[](){   SET_b_r(3,&REG.A);   }},

        {"SET 4,B", 1,  	[](){   SET_b_r(4,&REG.B);   }},
        {"SET 4,C", 1,  	[](){   SET_b_r(4,&REG.C);   }},
        {"SET 4,D", 1,   	[](){   SET_b_r(4,&REG.D);   }},
        {"SET 4,E", 1,  	[](){   SET_b_r(4,&REG.E);   }},
        {"SET 4,H", 1,  	[](){   SET_b_r(4,&REG.H);   }},
        {"SET 4,L", 1,   	[](){   SET_b_r(4,&REG.L);   }},
        {"SET 4,(HL)", 1,   [](){   SET_b_mm(4,REG.HL);  }},
        {"SET 4,A", 1,  	[](){   SET_b_r(4,&REG.A);   }},

        {"SET 5,B", 1,  	[](){   SET_b_r(5,&REG.B);   }},
        {"SET 5,C", 1,  	[](){   SET_b_r(5,&REG.C);   }},
        {"SET 5,D", 1,   	[](){   SET_b_r(5,&REG.D);   }},
        {"SET 5,E", 1,  	[](){   SET_b_r(5,&REG.E);   }},
        {"SET 5,H", 1,  	[](){   SET_b_r(5,&REG.H);   }},
        {"SET 5,L", 1,   	[](){   SET_b_r(5,&REG.L);   }},
        {"SET 5,(HL)", 1,   [](){   SET_b_mm(5,REG.HL);  }},
        {"SET 5,A", 1,  	[](){   SET_b_r(5,&REG.A);   }},

        {"SET 6,B", 1,  	[](){   SET_b_r(6,&REG.B);   }},
        {"SET 6,C", 1,  	[](){   SET_b_r(6,&REG.C);   }},
        {"SET 6,D", 1,   	[](){   SET_b_r(6,&REG.D);   }},
        {"SET 6,E", 1,  	[](){   SET_b_r(6,&REG.E);   }},
        {"SET 6,H", 1,  	[](){   SET_b_r(6,&REG.H);   }},
        {"SET 6,L", 1,   	[](){   SET_b_r(6,&REG.L);   }},
        {"SET 6,(HL)", 1,   [](){   SET_b_mm(6,REG.HL);  }},
        {"SET 6,A", 1,  	[](){   SET_b_r(6,&REG.A);   }},

        {"SET 7,B", 1,  	[](){   RES_b_r(7,&REG.B);   }},
        {"SET 7,C", 1,  	[](){   RES_b_r(7,&REG.C);   }},
        {"SET 7,D", 1,   	[](){   RES_b_r(7,&REG.D);   }},
        {"SET 7,E", 1,  	[](){   RES_b_r(7,&REG.E);   }},
        {"SET 7,H", 1,  	[](){   RES_b_r(7,&REG.H);   }},
        {"SET 7,L", 1,   	[](){   RES_b_r(7,&REG.L);   }},
        {"SET 7,(HL)", 1,   [](){   RES_b_mm(7,REG.HL);  }},
        {"SET 7,A", 1,  	[](){   RES_b_r(7,&REG.A);   }},
    };

    bool CLOCK()
    {
        unsigned short thisInst = READ8();

        if(thisInst == 0xCB)
            thisInst = READ8()+0x100;

        printf("%04x: ", REG.PC-1);

        //This is for log!
        switch(INSTRUCTIONS[thisInst].LENGTH)
        {
            case 1:
                printf(INSTRUCTIONS[thisInst].LABEL);
                break;
            case 2:
            {
                switch(thisInst)
                {
                    //>RST<
                    case 0x18: goto $F8;
                    case 0x20: goto $F8;
                    case 0x28: goto $F8;
                    case 0x30: goto $F8;
                    case 0x38: goto $F8;
                    case 0xE8: goto $F8;
                    case 0xF8:
                        $F8:
                        printf(INSTRUCTIONS[thisInst].LABEL);
                        break;
                    default:
                        printf(INSTRUCTIONS[thisInst].LABEL, CARTRIDGE::BUFFER[REG.PC]);
                        break;
                }
                break;
            }
            case 3:
                printf(INSTRUCTIONS[thisInst].LABEL, CARTRIDGE::BUFFER[REG.PC]|(CARTRIDGE::BUFFER[REG.PC+1]<<8));
                break;
        };

        printf("\n");
        INSTRUCTIONS[thisInst].EXEC();
        return true;
    }  
    bool INIT()
    {
        //Initial values;
        REG.AF = 0x0000;
        REG.BC = 0x0000;
        REG.DE = 0x0000;
        REG.HL = 0x0000;
        REG.SP = 0xFFFE;
        REG.PC = 0x0100;

        return true;
    }      
};