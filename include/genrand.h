#ifndef GENRAND_H
#define GENRAND_H

#ifdef __cplusplus
extern "C" {
#endif
extern void sgenrand(unsigned int seed);
extern unsigned int genrandui(void);
extern float genrandf(void);
#ifdef __cplusplus
 }
#endif
#endif
