#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

void BUTTON_Init(void);
void BUTTON_Process(void);

bool BUTTON_WasPressed(void);
bool BUTTON_IsPressed(void);

#endif
