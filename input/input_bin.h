#ifndef INPUT_BIN_H
#define INPUT_BIN_H

size_t disasm_single(struct instruction *i, uint16_t pc, uint16_t inst, uint16_t extra);
int input_bin(FILE *f, struct instruction **i, size_t *i_count);

#endif /* INPUT_BIN_H */
