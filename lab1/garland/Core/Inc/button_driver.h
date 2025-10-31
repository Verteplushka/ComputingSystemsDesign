#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

void BUTTON_Init(void);
/* must be called periodically from main loop (non-blocking) to sample button.
   It internally samples not more often than SAMPLE_MS */
void BUTTON_Process(void);

bool BUTTON_WasPressed(void); // returns true once per press (edge)
bool BUTTON_IsPressed(void);  // current level

#endif // BUTTON_DRIVER_H
