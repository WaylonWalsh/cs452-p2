/**
 * @file lab.c
 * @author Waylon Walsh
 * @brief Implementation of a thread-safe FIFO queue for solving the bounded buffer problem
 * @date 10/20/2024
 *
 * This file implements a fixed-capacity, thread-safe FIFO (First-In-First-Out) queue
 * that can be used to solve the bounded buffer problem in concurrent programming.
 * The queue uses mutex locks and condition variables to ensure thread safety and
 * efficient blocking when the queue is full or empty.
 *
 */

#include "lab.h"
#include <pthread.h>
#include <stdio.h>

// Define the structure for the queue
struct queue {
    void **buffer;               // Pointer to the array that will hold our data
    int capacity;                // Maximum number of items the queue can hold
    int size;                    // Current number of items in the queue
    int front;                   // Index of the front of the queue
    int rear;                    // Index of the rear of the queue
    pthread_mutex_t mutex;       // Mutex for ensuring thread-safe access to the queue
    pthread_cond_t not_full;     // Condition variable for signaling when queue is not full
    pthread_cond_t not_empty;    // Condition variable for signaling when queue is not empty
    bool is_shutdown;            // Flag to indicate if the queue has been shut down
};

// Initialize a new queue
queue_t queue_init(int capacity) {

    // Allocate memory for the queue structure
    queue_t q = malloc(sizeof(struct queue));
    if (!q) return NULL; // If malloc fails, return NULL

    // Allocate memory for the buffer
    q->buffer = malloc(capacity * sizeof(void*));
    if (!q->buffer) {
        free(q); // If buffer allocation fails, free the queue and return NULL
        return NULL;
    }

    // Initialize queue properties
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = -1; // -1 indicates an empty queue
    q->is_shutdown = false;

    // Initialize synchronization primitives
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);

    return q;
}

// Destroy the queue and free all associated memory
void queue_destroy(queue_t q) {
    if (!q) return; // If queue is NULL, do nothing

    // Destroy synchronization primitives
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);

    // Free allocated memory
    free(q->buffer);
    free(q);
}

// Add an item to the queue
void enqueue(queue_t q, void *data) {
    pthread_mutex_lock(&q->mutex);   // Lock the mutex to ensure thread-safe access

    // Wait while the queue is full and not shut down
    while (q->size == q->capacity && !q->is_shutdown) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }

    // If the queue has been shut down, unlock and return
    if (q->is_shutdown) {
        pthread_mutex_unlock(&q->mutex);
        return;
    }

    // Add the item to the queue
    q->rear = (q->rear + 1) % q->capacity; // Circular buffer implementation
    q->buffer[q->rear] = data;
    q->size++;

    pthread_cond_signal(&q->not_empty);  // Signal that the queue is not empty
    pthread_mutex_unlock(&q->mutex);     // Unlock the mutex
}

// Remove and return an item from the queue
void *dequeue(queue_t q) {
    pthread_mutex_lock(&q->mutex);  // Lock the mutex to ensure thread-safe access

    // Wait while the queue is empty and not shut down
    while (q->size == 0 && !q->is_shutdown) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }

    // If the queue is empty (could happen after shutdown), return NULL
    if (q->size == 0) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    // Remove and return the item from the front of the queue
    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;  // Circular buffer implementation
    q->size--;

    pthread_cond_signal(&q->not_full);  // Signal that the queue is not full
    pthread_mutex_unlock(&q->mutex);    // Unlock the mutex

    return data;
}

// Shut down the queue
void queue_shutdown(queue_t q) {
    pthread_mutex_lock(&q->mutex);  // Lock the mutex to ensure thread-safe access

    q->is_shutdown = true; // Set the shutdown flag

    // Wake up all threads waiting on the queue
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);

    pthread_mutex_unlock(&q->mutex); // Unlock the mutex

    fprintf(stderr, "Shutdown requested!\n");
}

// Check if the queue is empty
bool is_empty(queue_t q) {
    pthread_mutex_lock(&q->mutex);   // Lock the mutex to ensure thread-safe access
    bool empty = (q->size == 0);     // Check if size is 0
    pthread_mutex_unlock(&q->mutex); // Unlock the mutex
    return empty;
}

// Check if the queue has been shut down
bool is_shutdown(queue_t q) {
    pthread_mutex_lock(&q->mutex);   // Lock the mutex to ensure thread-safe access
    bool shutdown = q->is_shutdown;  // Check the shutdown flag
    pthread_mutex_unlock(&q->mutex); // Unlock the mutex
    return shutdown;
}