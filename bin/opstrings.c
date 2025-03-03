/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

const char *NORMAL_OPS[] = {"special", "bcond", "j",   "jal",  "beq",  "bne",  "blez", "bgtz", "addi", "addiu", "slti", "sltiu", "andi",
                            "ori",     "xori",  "lui", "cop0", "cop1", "cop2", "cop3", "024",  "025",  "026",   "027",  "030",   "031",
                            "032",     "033",   "034", "035",  "036",  "037",  "lb",   "lh",   "lwl",  "lw",    "lbu",  "lhu",   "lwr",
                            "047",     "sb",    "sh",  "swl",  "sw",   "054",  "055",  "swr",  "057",  "lwc0",  "lwc1", "lwc2",  "lwc3",
                            "064",     "065",   "066", "067",  "swc0", "swc1", "swc2", "swc3", "074",  "075",   "076",  "077"};

const char *SPECIAL_OPS[] = {
    "sll",  "001",  "srl",  "sra",  "sllv", "005", "srlv", "srav", "jr",   "jalr",  "012", "013",  "syscall", "break", "016", "017",
    "mfhi", "mthi", "mflo", "mtlo", "024",  "025", "026",  "027",  "mult", "multu", "div", "divu", "034",     "035",   "036", "037",
    "add",  "addu", "sub",  "subu", "and",  "or",  "xor",  "nor",  "050",  "051",   "slt", "sltu", "054",     "055",   "056", "057",
    "060",  "061",  "062",  "063",  "064",  "065", "066",  "067",  "070",  "071",   "072", "073",  "074",     "075",   "076", "077",
};
