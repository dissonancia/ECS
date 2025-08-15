#ifndef CODE_H_
#define CODE_H_

int create_mnemonic_tables();
void free_mnemonic_tables();

int code_dest(unsigned char *mnemonic);
int code_comp(unsigned char *mnemonic);
int code_jump(unsigned char *mnemonic);

#endif // CODE_H_