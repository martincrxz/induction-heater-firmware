#ifndef _FREQ_H_
#define _FREQ_H_

void timer_capture_init();
void timer_capture_int_handler();
void send_freq_timer_init();
void send_freq_timer_int_handler();
uint32_t get_freq_count();

#endif
