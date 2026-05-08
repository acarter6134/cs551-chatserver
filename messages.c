#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "messages.h"

struct MessageQueue message_queue_new(size_t cap, bool overwrite) {
  // NOTE: capacity is cap + 1 because ring-buffers need a dummy space
  // to distinguish between full and empty states
  struct Message *messages = calloc(cap + 1, sizeof(struct Message));
  return (struct MessageQueue){.messages = messages,
                               .messages_cap = cap + 1,
                               .write_index = 0,
                               .read_index = 0,
                               .overwrite = overwrite};
}

void message_queue_free(struct MessageQueue q) { free(q.messages); }

bool message_queue_is_empty(struct MessageQueue q) {
  return q.write_index == q.read_index;
}

bool message_queue_is_full(struct MessageQueue q) {
  return (q.write_index + 1) % q.messages_cap == q.read_index;
}

bool message_queue_pop(struct MessageQueue *q, struct Message *c) {
  if (message_queue_is_empty(*q)) {
    return false;
  }
  *c = q->messages[q->read_index];
  q->read_index = (q->read_index + 1) % q->messages_cap;
  return true;
}

bool message_queue_push(struct MessageQueue *q, struct Message c) {
  if (message_queue_is_full(*q)) {
    if (q->overwrite) {
      q->read_index = (q->read_index + 1) % q->messages_cap;
    } else {
      return false;
    }
  }
  q->messages[q->write_index] = c;
  q->write_index = (q->write_index + 1) % q->messages_cap;
  return true;
}

bool waiting_messages_init(void) {
  for (size_t i = 0; i < MAX_JOINED_CLIENTS; ++i) {
    if (pthread_mutex_init(&waiting_messages_mutexes[i], NULL) == -1) {
      perror("pthread_mutex_init()");
      return false;
    }
    if (pthread_cond_init(&waiting_messages_conds[i], NULL) == -1) {
      perror("pthread_cond_init()");
      return false;
    }
    waiting_messages[i] = message_queue_new(MAX_MESSAGES, false);
  }
  return true;
}

void waiting_messages_free(void) {
  for (size_t i = 0; i < MAX_JOINED_CLIENTS; ++i) {
    message_queue_free(waiting_messages[i]);
  }
}
