#include <stdbool.h>
#include <stdio.h>

#define MAX_MORSE_QUEUE_SIZE 6

typedef struct {
  int items[MAX_MORSE_QUEUE_SIZE];
  int head;
  int tail;
} MORSE_QUEUE;

void initQueue(MORSE_QUEUE *queue) {
  queue->head = -1;
  queue->tail = 0;
}

bool isFull(MORSE_QUEUE *queue) {
  return (queue->tail == MAX_MORSE_QUEUE_SIZE);
}

bool isEmpty(MORSE_QUEUE *queue) { return (queue->head == queue->tail - 1); }

void enqueue(MORSE_QUEUE *queue, int value) {
  if (isFull(queue)) {
    printf("Queue full\n");
    return;
  }

  queue->items[queue->tail] = value;
  queue->tail++;
  return;
}

void dequeue(MORSE_QUEUE *queue) {
  if (isEmpty(queue)) {
    printf("Queue empty\n");
    return;
  }
  queue->head++;
}

int peek(MORSE_QUEUE *queue) {
  if (isEmpty(queue)) {
    printf("Queue empty\n");
    return -1;
  }
  return queue->items[queue->head + 1];
}

void printQueue(MORSE_QUEUE *queue) {
  if (isEmpty(queue)) {
    printf("Queue empty\n");
    return;
  }

  for (int i = queue->head + 1; i < queue->tail; i++) {
    printf("%d ", queue->items[i]);
  }
  printf("\n");
  return;
}
