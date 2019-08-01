#ifndef OUTPUT_ASM_H
#define OUTPUT_ASM_H

int output_asm(FILE *fout, struct label *labels, size_t label_count, struct instruction *insts, size_t insts_count);

#endif /* OUTPUT_ASM_H */
