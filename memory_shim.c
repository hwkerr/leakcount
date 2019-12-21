#define _GNU_SOURCE

void __attribute__ ((constructor)) malloc_init(void);
void __attribute__ ((constructor)) free_init(void);
void __attribute__ ((destructor)) cleanup(void);

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

void *(*original_malloc)(size_t) = NULL;
void (*original_free)(void *) = NULL;

// This struct holds information about each object that is malloced
// so that it can be used in a linked list
struct Memspace
{
  int size;
  void *address;
  struct Memspace *prev;
  struct Memspace *next;
};

struct Memspace* head = NULL;
struct Memspace* tail = NULL;

// requires memory has already been allocated for newitem
// requires newitem->size and newitem->address already have values
// ensures newitem is part of the linked list
void addItem(struct Memspace *newitem)
{
  newitem->next = NULL;

  if (head == NULL) {
    newitem->prev = NULL;
    head = newitem;
    tail = head;
  } else {
    newitem->prev = tail;
    tail->next = newitem;
    tail = newitem;
  }
}

// Attempts to remove an item from the Memspace list with address = ptr
// removeItem = 1 if successful, 0 if failed to find member with address = ptr
int removeItem(void *ptr)
{
  struct Memspace *item = head;
  while (item != NULL)
  {
    if (item->address == ptr) {
      if (item == head) {
        if (item == tail) {
          head = NULL;
          tail = NULL;
        } else {
          head = item->next;
          head->prev = NULL;
        }
      } else if (item != head) {
        item->prev->next = item->next;
        if (item == tail)
          tail = item->prev;
        else if (item->next != NULL)
          item->next->prev = item->prev;
      }
      original_free(item);
      return 1;
    }
    item = item->next;
  }
  return 0;
}

// Called when the library is unloaded
void cleanup(void)
{
  int leakcount = 0, leaksize = 0;
  struct Memspace *item = head;
  while (item != NULL)
  {
    leakcount++;
    leaksize += item->size;
    fprintf(stderr, "LEAK\t%d\n", item->size);
    item = item->next;
  }
  fprintf(stderr, "TOTAL\t%d\t%d\n", leakcount, leaksize);
}

void malloc_init(void)
{
  if (original_malloc == NULL) {
  	original_malloc = dlsym(RTLD_NEXT, "malloc");
  }
}

void free_init(void)
{
  if (original_free == NULL) {
    original_free = dlsym(RTLD_NEXT, "free");
  }
}

// replacement of the original malloc function
void *malloc (size_t size)
{
  void *ptr = original_malloc(size);

  struct Memspace *newitem = NULL;
  newitem = (struct Memspace*)original_malloc(sizeof(struct Memspace));
  newitem->size = size;
  newitem->address = ptr;
  addItem(newitem);

  return ptr;
}

// replacement of the original free function
void free (void *ptr)
{
  removeItem(ptr);
  return original_free(ptr);
}
