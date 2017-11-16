#include "test-common.h"

#include <CUnit/Basic.h>
#include <float.h>
#include <stdarg.h>
#include <assert.h>

static int gt_int(const void *a, const void *b)
{
        return *((int *)a) > *((int *)b);
}

void test_priority_queue_ctor()
{
        cdc_priority_queue_t *q;

        CU_ASSERT(cdc_priority_queue_ctor(&q, NULL, gt_int) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 0);

        cdc_priority_queue_dtor(q);
}

void test_priority_queue_ctorl()
{
        cdc_priority_queue_t *q;
        int a = 2, b = 3;
        void *elem;

        CU_ASSERT(cdc_priority_queue_ctorl(&q, NULL, gt_int, &a, &b, NULL) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 2);

        elem = cdc_priority_queue_top(q);
        CU_ASSERT(*((int *)elem) == b);
        CU_ASSERT(cdc_priority_queue_pop(q) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 1);

        elem = cdc_priority_queue_top(q);
        CU_ASSERT(*((int *)elem) == a);
        CU_ASSERT(cdc_priority_queue_pop(q) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 0);

        cdc_priority_queue_dtor(q);
}

void test_priority_queue_push()
{
        cdc_priority_queue_t *q;
        int a = 0, b = 1, c = 2;
        void *elem;

        CU_ASSERT(cdc_priority_queue_ctor(&q, NULL, gt_int) == CDC_STATUS_OK);

        CU_ASSERT(cdc_priority_queue_push(q, &a) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 1);
        elem = cdc_priority_queue_top(q);
        CU_ASSERT(*((int *)elem) == a);

        CU_ASSERT(cdc_priority_queue_push(q, &c) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 2);
        elem = cdc_priority_queue_top(q);
        CU_ASSERT(*((int *)elem) == c);

        CU_ASSERT(cdc_priority_queue_push(q, &b) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 3);
        elem = cdc_priority_queue_top(q);
        CU_ASSERT(*((int *)elem) == c);

        cdc_priority_queue_dtor(q);
}

void test_priority_queue_pop()
{
        cdc_priority_queue_t *q;
        int a = 0, b = 3, c = 2, d = 1;
        void *elem;

        CU_ASSERT(cdc_priority_queue_ctorl(&q, NULL, gt_int, &a, &b, &c, &d, NULL) == CDC_STATUS_OK);

        elem = cdc_priority_queue_top(q);
        CU_ASSERT(cdc_priority_queue_pop(q) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 3);
        CU_ASSERT(*((int *)elem) == b);

        elem = cdc_priority_queue_top(q);
        CU_ASSERT(cdc_priority_queue_pop(q) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 2);
        CU_ASSERT(*((int *)elem) == c);

        elem = cdc_priority_queue_top(q);
        CU_ASSERT(cdc_priority_queue_pop(q) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 1);
        CU_ASSERT(*((int *)elem) == d);

        elem = cdc_priority_queue_top(q);
        CU_ASSERT(cdc_priority_queue_pop(q) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_size(q) == 0);
        CU_ASSERT(*((int *)elem) == a);

        cdc_priority_queue_dtor(q);
}

void test_priority_queue_top()
{
        cdc_priority_queue_t *q;
        int a = 1, b = 10, c = 2;

        CU_ASSERT(cdc_priority_queue_ctorl(&q, NULL, gt_int, &a, &b, &c, NULL) == CDC_STATUS_OK);
        CU_ASSERT(*((int *)cdc_priority_queue_top(q)) == b);

        cdc_priority_queue_dtor(q);
}

void test_priority_queue_swap()
{
        cdc_priority_queue_t *v, *w;
        int a = 2, b = 3, c = 4;

        CU_ASSERT(cdc_priority_queue_ctorl(&v, NULL, gt_int, &b, NULL) == CDC_STATUS_OK);
        CU_ASSERT(cdc_priority_queue_ctorl(&w, NULL, gt_int, &a, &c, NULL) == CDC_STATUS_OK);

        cdc_priority_queue_swap(v, w);

        CU_ASSERT(cdc_priority_queue_size(v) == 2);
        CU_ASSERT(*((int *)cdc_priority_queue_top(v)) == c);
        CU_ASSERT(cdc_priority_queue_size(w) == 1);
        CU_ASSERT(*((int *)cdc_priority_queue_top(w)) == b);

        cdc_priority_queue_dtor(v);
        cdc_priority_queue_dtor(w);
}
