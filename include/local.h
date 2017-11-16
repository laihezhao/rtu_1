#ifndef __LOCAL_H_
#define __LOCAL_H_

int local_di(int);
int local_do(int, int);
float local_ai(int);

int open_di_device(void);
int open_do_device(void);
int open_ai_device(void);

void close_di_device(void);
void close_do_device(void);
void close_ai_device(void);

#endif //__LOCAL_H_
