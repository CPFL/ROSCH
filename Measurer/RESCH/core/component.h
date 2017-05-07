#ifndef __RESCH_COMPONENT_H__
#define __RESCH_COMPONENT_H__

int api_component_create(void);
int api_component_destroy(int);
int api_compose(int, int);
int api_decompose(int);

void component_init(void);
void component_exit(void);

#endif
