#ifndef SOK_TEST_KEYDEV_H
#define SOK_TEST_KEYDEV_H
#include <gint/keyboard.h>
#include <stdint.h>
#define KEYBOARD_QUEUE_SIZE 32
typedef struct {uint32_t time;int8_t queue_next,queue_end;uint8_t state_now[12],state_queue[12];} keydev_t;
typedef struct {int enabled;int (*repeater)(int,int,int);} keydev_transform_t;
enum {KEYDEV_TR_REPEATS=0x10};
keydev_t *keydev_std(void);
void keydev_set_transform(keydev_t *device,keydev_transform_t transform);
key_event_t keydev_read(keydev_t *device,bool wait,volatile int *timeout);
bool keydev_idle(keydev_t *device,...);
#endif
