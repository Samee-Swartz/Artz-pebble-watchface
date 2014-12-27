#ifndef DATETIME_H
#define DATETIME_H

TextLayer *call_layer;

void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context);
void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context);
void send_cmd(void);

#endif
