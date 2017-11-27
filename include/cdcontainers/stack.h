// The MIT License (MIT)
// Copyright (c) 2017 Maksim Andrianov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
/**
  * @file
  * @author Maksim Andrianov <maksimandrianov1@yandex.ru>
  * @brief The cdc_stack is a struct and functions that provide a stack
  */
#ifndef CDCONTAINERS_INCLUDE_CDCONTAINERS_STACK_H
#define CDCONTAINERS_INCLUDE_CDCONTAINERS_STACK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <cdcontainers/status.h>
#include <cdcontainers/common.h>

/**
 * @brief The cdc_stack_table struct
 * @warning To avoid problems, do not change the structure fields in the code.
 * Use only special functions to access and change structure fields.
 */
struct cdc_stack_table {
        enum cdc_stat (*ctor)(void **cntr, struct cdc_data_info *info);
        enum cdc_stat (*ctorv)(void **cntr, struct cdc_data_info *info,
                               va_list args);
        void (*dtor)(void *cntr);
        void *(*top)(void *cntr);
        bool (*empty)(void *cntr);
        size_t (*size)(void *cntr);
        enum cdc_stat (*push)(void *cntr, void *elem);
        enum cdc_stat (*pop)(void *cntr);
};

/**
 * @brief The cdc_stack struct
 * @warning To avoid problems, do not change the structure fields in the code.
 * Use only special functions to access and change structure fields.
 */
struct cdc_stack {
        void *container;
        const struct cdc_stack_table *table;
};

/**
 * @brief Table for the stack based on the vector
 */
extern const void *cdc_stackv_table;

/**
 * @brief Table for the stack based on the list
 */
extern const void *cdc_stackl_table;

/**
 * @brief Constructs an empty stack.
 * Returned CDC_STATUS_OK in a successful case or an excellent value
 * indicating an error
 */
enum cdc_stat cdc_stack_ctor(const struct cdc_stack_table *table,
                             struct cdc_stack **s, struct cdc_data_info *info);

/**
 * @brief Constructs a stack, initialized by an arbitrary number of pointers.
 * The last item must be NULL. Returned CDC_STATUS_OK in a successful case
 * or an excellent value indicating an error
 */
enum cdc_stat cdc_stack_ctorl(const struct cdc_stack_table *table,
                              struct cdc_stack **s,
                              struct cdc_data_info *info, ...);

/**
 * @brief Constructs a stack, initialized by args
 * The last item must be NULL. Returned CDC_STATUS_OK in a successful case
 * or an excellent value indicating an error
 */
enum cdc_stat cdc_stack_ctorv(const struct cdc_stack_table *table,
                              struct cdc_stack **s,
                              struct cdc_data_info *info, va_list args);

/**
 * @brief Constructs an empty stack based on list.
 * Returned CDC_STATUS_OK in a successful case or an excellent value
 * indicating an error
 */
static inline enum cdc_stat cdc_stackl_ctor(struct cdc_stack **s,
                                            struct cdc_data_info *info)
{
        assert(s != NULL);

        return cdc_stack_ctor(cdc_stackl_table, s, info);
}

/**
 * @brief Constructs a stack based on list, initialized by
 * an arbitrary number of pointers.
 * The last item must be NULL. Returned CDC_STATUS_OK in a successful case
 * or an excellent value indicating an error
 */
enum cdc_stat cdc_stackl_ctorl(struct cdc_stack **s,
                               struct cdc_data_info *info, ...);

/**
 * @brief Constructs a stack based on list, initialized by args
 * The last item must be NULL. Returned CDC_STATUS_OK in a successful case
 * or an excellent value indicating an error
 */
static inline enum cdc_stat cdc_stackl_ctorv(struct cdc_stack **s,
                                             struct cdc_data_info *info,
                                             va_list args)
{
        assert(s != NULL);

        return cdc_stack_ctorv(cdc_stackl_table, s, info, args);
}

/**
 * @brief Destroys the stack.
 */
void cdc_stack_dtor(struct cdc_stack *s);

// Element access
/**
 * @brief Returns a pointer to the stack's top item. This function assumes
 * that the stack isn't empty.
 */
static inline void *cdc_stack_top(struct cdc_stack *s)
{
        assert(s != NULL);

        return s->table->top(s->container);
}

// Capacity
/**
 * @brief Returns true if the stack has size 0; otherwise returns false.
 */
static inline bool cdc_stack_empty(struct cdc_stack *s)
{
        assert(s != NULL);

        return s->table->empty(s->container);
}

/**
 * @brief Returns the number of items in the stack.
 */
static inline size_t cdc_stack_size(struct cdc_stack *s)
{
        assert(s != NULL);

        return s->table->size(s->container);
}

// Modifiers
/**
 * @brief Adds element elem to the top of the stack. Returned CDC_STATUS_OK in a
 * successful case or an excellent value indicating an error
 */
static inline enum cdc_stat cdc_stack_push(struct cdc_stack *s, void *elem)
{
        assert(s != NULL);

        return s->table->push(s->container, elem);
}

/**
 * @brief Removes the top item from the stack.
 * This function assumes that the stack isn't empty. Returned CDC_STATUS_OK in
 * a successful case or an excellent value indicating an error
 */
static inline enum cdc_stat cdc_stack_pop(struct cdc_stack *s)
{
        assert(s != NULL);

        return s->table->pop(s->container);
}

/**
 * @brief Swaps stack a and b. This operation is very fast and never fails.
 */
void cdc_stack_swap(struct cdc_stack *a, struct cdc_stack *b);

// Short names
#ifdef CDC_USE_SHORT_NAMES
typedef struct cdc_stack stack_t;

#define stack_ctor(...)   cdc_stack_ctor(__VA_ARGS__)
#define stack_ctorl(...)  cdc_stack_ctorl(__VA_ARGS__)
#define stack_ctorv(...)  cdc_stack_ctorv(__VA_ARGS__)
#define stack_dtor(...)   cdc_stack_dtor(__VA_ARGS__)

// Element access
#define stack_top(...)    cdc_stack_top(__VA_ARGS__)

// Capacity
#define stack_empty(...)  cdc_stack_empty(__VA_ARGS__)
#define stack_size(...)   cdc_stack_size(__VA_ARGS__)

// Modifiers
#define stack_push(...)   cdc_stack_push(__VA_ARGS__)
#define stack_pop(...)    cdc_stack_pop(__VA_ARGS__)
#define stack_swap(...)   cdc_stack_swap(__VA_ARGS__)
#endif

#endif  // CDCONTAINERS_INCLUDE_CDCONTAINERS_STACK_H
