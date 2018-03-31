// The MIT License (MIT)
// Copyright (c) 2018 Maksim Andrianov
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
#include "cdcontainers/splay-tree.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "data-info.h"
#include "tree.h"

struct node_pair
{
        struct cdc_splay_tree_node *l, *r;
};

MAKE_FIND_NODE(struct cdc_splay_tree_node *)
MAKE_MIN_NODE(struct cdc_splay_tree_node *)
MAKE_MAX_NODE(struct cdc_splay_tree_node *)
MAKE_SUCCESSOR(struct cdc_splay_tree_node *)
MAKE_PREDECESSOR(struct cdc_splay_tree_node *)

static void free_node(struct cdc_splay_tree *t, struct cdc_splay_tree_node *node)
{
        assert(t != NULL);

        struct cdc_pair pair;

        if (CDC_HAS_DFREE(t->dinfo)) {
                pair.first = node->key;
                pair.second = node->value;
                t->dinfo->dfree(&pair);
        }

        free(node);
}

static void free_splay_tree(struct cdc_splay_tree *t, struct cdc_splay_tree_node *root)
{
        assert(t != NULL);

        if (root == NULL)
                return;

        free_splay_tree(t, root->left);
        free_splay_tree(t, root->right);
        free_node(t, root);
}

static struct cdc_splay_tree_node *new_node(void *key, void *val)
{
        struct cdc_splay_tree_node *node;

        node = (struct cdc_splay_tree_node *)malloc(sizeof(struct cdc_splay_tree_node));
        if (node) {
                node->key = key;
                node->value = val;
                node->parent = node->left = node->right = NULL;
        }

        return node;
}

static void update_link(struct cdc_splay_tree_node *parent,
                        struct cdc_splay_tree_node *old,
                        struct cdc_splay_tree_node *node)
{
        if (!parent)
                return;

        if (parent->left == old)
                parent->left = node;
        else
                parent->right = node;
}

static struct cdc_splay_tree_node *zig_right(struct cdc_splay_tree_node *node)
{
        struct cdc_splay_tree_node *p = node->parent;

        update_link(p->parent, p, node);
        node->parent = p->parent;
        p->left = node->right;
        if (p->left)
                p->left->parent = p;

        node->right = p;
        if (node->right)
                node->right->parent = node;

        return node;
}

static struct cdc_splay_tree_node *zig_left(struct cdc_splay_tree_node *node)
{
        struct cdc_splay_tree_node *p = node->parent;

        update_link(p->parent, p, node);
        node->parent = p->parent;
        p->right = node->left;
        if (p->right)
                p->right->parent = p;

        node->left = p;
        if (node->left)
                node->left->parent = node;

        return node;
}

static struct cdc_splay_tree_node *zigzig_right(struct cdc_splay_tree_node *node)
{
        node = zig_right(node->parent);
        return zig_right(node->left);
}

static struct cdc_splay_tree_node *zigzig_left(struct cdc_splay_tree_node *node)
{
        node = zig_left(node->parent);
        return zig_left(node->right);
}

static struct cdc_splay_tree_node *zigzag_right(struct cdc_splay_tree_node *node)
{
        node = zig_left(node);
        return zig_right(node);
}

static struct cdc_splay_tree_node *zigzag_left(struct cdc_splay_tree_node *node)
{
        node = zig_right(node);
        return zig_left(node);
}

static struct cdc_splay_tree_node *splay(struct cdc_splay_tree_node *node)
{
        while (node->parent) {
                if (node->parent->parent == NULL) {
                        if (node == node->parent->left)
                                node = zig_right(node);
                        else
                                node = zig_left(node);
                } else if (node == node->parent->left) {
                        if (node->parent == node->parent->parent->left)
                                node = zigzig_right(node);
                        else
                                node = zigzag_left(node);
                } else {
                        if (node->parent == node->parent->parent->right)
                                node = zigzig_left(node);
                        else
                                node = zigzag_right(node);
                }
        }

        return node;
}

static struct cdc_splay_tree_node *find_nearest(struct cdc_splay_tree_node *node,
                                                void *key, cdc_binary_pred_fn_t compar)
{
        struct cdc_splay_tree_node *tmp = node;

        while (node) {
                if (compar(key, node->key)) {
                        if (node->left)
                                node = node->left;
                        else
                                break;
                } else if (compar(node->key, key)) {
                        if (node->right)
                                node = node->right;
                        else
                                break;
                } else {
                        break;
                }
        }

        return node ? node : max_node(tmp);
}

static struct node_pair split(struct cdc_splay_tree_node *node, void *key,
                              cdc_binary_pred_fn_t compar)
{
        struct node_pair ret;

        node = splay(node);
        if (compar(key, node->key)) {
                ret.l = node->left;
                ret.r = node;
                node->left = NULL;
                if (ret.l)
                        ret.l->parent = NULL;
        } else {
                ret.l = node;
                ret.r = node->right;
                node->right = NULL;
                if (ret.r)
                        ret.r->parent = NULL;
        }

        return ret;
}

static struct cdc_splay_tree_node *merge(struct cdc_splay_tree_node *a,
                                         struct cdc_splay_tree_node *b)
{
        if (!a)
                return b;

        if (!b)
                return a;

        a = max_node(a);
        a = splay(a);
        a->right = b;
        b->parent = a;
        return a;
}

static struct cdc_splay_tree_node *sfind(struct cdc_splay_tree *t, void *key)
{
        struct cdc_splay_tree_node *node = find_node(t->root, key, t->compar);

        if (!node)
                return node;

        node = splay(node);
        t->root = node;
        return node;
}

static struct cdc_splay_tree_node *insert_unique(struct cdc_splay_tree *t,
                                                 struct cdc_splay_tree_node *node,
                                                 struct cdc_splay_tree_node *nearest)
{
        struct node_pair pair;

        if (t->root == NULL) {
                t->root = node;
        } else {
                pair = split(nearest, node->key, t->compar);
                node->left = pair.l;
                if (node->left)
                        node->left->parent = node;

                node->right = pair.r;
                if (node->right)
                        node->right->parent = node;
                t->root = node;
        }

        ++t->size;
        return node;
}

static struct cdc_splay_tree_node *make_and_insert_unique(struct cdc_splay_tree *t,
                                                          void *key, void *value,
                                                          struct cdc_splay_tree_node *nearest)
{
        struct cdc_splay_tree_node *node = new_node(key, value);

        if (!node)
                return node;

        return insert_unique(t, node, nearest);
}

static enum cdc_stat init_varg(struct cdc_splay_tree *t, va_list args)
{
        enum cdc_stat stat;
        struct cdc_pair *pair;

        while ((pair = va_arg(args, struct cdc_pair *)) != NULL) {
                stat = cdc_splay_tree_insert(t, pair->first, pair->second, NULL);
                if (stat != CDC_STATUS_OK)
                        return stat;
        }

        return CDC_STATUS_OK;
}

enum cdc_stat cdc_splay_tree_ctor1(struct cdc_splay_tree **t, struct cdc_data_info *info,
                                   cdc_binary_pred_fn_t compar)
{
        assert(t != NULL);
        assert(CDC_HAS_LT(info) || compar != NULL);

        struct cdc_splay_tree *tmp;

        tmp = (struct cdc_splay_tree *)calloc(sizeof(struct cdc_splay_tree), 1);
        if (!tmp)
                return CDC_STATUS_BAD_ALLOC;

        if (info && !(tmp->dinfo = cdc_di_shared_ctorc(info))) {
                free(tmp);
                return CDC_STATUS_BAD_ALLOC;
        }

        tmp->compar = compar ? compar : info->lt;
        *t = tmp;
        return CDC_STATUS_OK;
}

enum cdc_stat cdc_splay_tree_ctorl1(struct cdc_splay_tree **t, struct cdc_data_info *info,
                                    cdc_binary_pred_fn_t compar, ...)
{
        assert(t != NULL);
        assert(CDC_HAS_LT(info) || compar != NULL);

        enum cdc_stat stat;
        va_list args;

        va_start(args, compar);
        stat = cdc_splay_tree_ctorv1(t, info, compar, args);
        va_end(args);

        return stat;
}

enum cdc_stat cdc_splay_tree_ctorv1(struct cdc_splay_tree **t, struct cdc_data_info *info,
                                    cdc_binary_pred_fn_t compar, va_list args)
{
        assert(t != NULL);
        assert(CDC_HAS_LT(info) || compar != NULL);

        enum cdc_stat stat;

        stat = cdc_splay_tree_ctor1(t, info, compar);
        if (stat != CDC_STATUS_OK)
                return stat;

        return init_varg(*t, args);
}

enum cdc_stat cdc_splay_tree_ctor(struct cdc_splay_tree **t,
                                  struct cdc_data_info *info)
{
        assert(t != NULL);
        assert(CDC_HAS_LT(info));

        return cdc_splay_tree_ctor1(t, info, NULL);
}

enum cdc_stat cdc_splay_tree_ctorl(struct cdc_splay_tree **t,
                                   struct cdc_data_info *info, ...)
{
        assert(t != NULL);
        assert(CDC_HAS_LT(info));

        enum cdc_stat stat;
        va_list args;

        va_start(args, info);
        stat = cdc_splay_tree_ctorv(t, info, args);
        va_end(args);

        return stat;
}

enum cdc_stat cdc_splay_tree_ctorv(struct cdc_splay_tree **t,
                                   struct cdc_data_info *info, va_list args)
{
        assert(t != NULL);
        assert(CDC_HAS_LT(info));

        return cdc_splay_tree_ctorv1(t, info, NULL, args);
}

void cdc_splay_tree_dtor(struct cdc_splay_tree *t)
{
        assert(t != NULL);

        free_splay_tree(t, t->root);
        cdc_di_shared_dtor(t->dinfo);
        free(t);
}

enum cdc_stat cdc_splay_tree_get(struct cdc_splay_tree *t, void *key, void **value)
{
        assert(t != NULL);

        struct cdc_splay_tree_node *node = sfind(t, key);

        if (node)
                *value = node->value;

        return node ? CDC_STATUS_OK : CDC_STATUS_NOT_FOUND;
}

size_t cdc_splay_tree_count(struct cdc_splay_tree *t, void *key)
{
        assert(t != NULL);

        return (size_t)(sfind(t, key) != NULL);
}

void cdc_splay_tree_find(struct cdc_splay_tree *t, void *key,
                         struct cdc_splay_tree_iter *it)
{
        assert(t != NULL);
        assert(it != NULL);

        struct cdc_splay_tree_node *node = sfind(t, key);

        if (!node) {
                cdc_splay_tree_end(t, it);
                return;
        }

        it->container = t;
        it->current = node;
        it->prev = predecessor(node);
}

void cdc_splay_tree_equal_range(struct cdc_splay_tree *t,  void *key,
                                struct cdc_pair_splay_tree_iter *pair)
{
        assert(t != NULL);
        assert(pair != NULL);

        cdc_splay_tree_find(t, key, &pair->first);
        cdc_splay_tree_end(t, &pair->second);
        if (cdc_splay_tree_iter_is_eq(&pair->first, &pair->second)) {
                cdc_splay_tree_end(t, &pair->first);
        } else {
                pair->second = pair->first;
                cdc_splay_tree_iter_next(&pair->second);
        }
}

enum cdc_stat cdc_splay_tree_insert(struct cdc_splay_tree *t, void *key, void *value,
                                    struct cdc_pair_splay_tree_iter_bool *ret)
{
        assert(t != NULL);

        struct cdc_splay_tree_node *node = find_nearest(t->root, key, t->compar);
        bool finded = node ? cdc_eq(t->compar, node->key, key) : false;

        if (!finded) {
                node = make_and_insert_unique(t, key, value, node);
                if (!node)
                        return CDC_STATUS_BAD_ALLOC;
        }

        if (ret) {
                (*ret).first.container = t;
                (*ret).first.current = node;
                (*ret).first.prev = predecessor(node);
                (*ret).second = !finded;
        }

        return CDC_STATUS_OK;
}

enum cdc_stat cdc_splay_tree_insert_or_assign(struct cdc_splay_tree *t,
                                              void *key, void *value,
                                              struct cdc_pair_splay_tree_iter_bool *ret)
{
        assert(t != NULL);

        struct cdc_splay_tree_node *node = find_nearest(t->root, key, t->compar);
        bool finded = node ? cdc_eq(t->compar, node->key, key) : false;

        if (!finded) {
                node = make_and_insert_unique(t, key, value, node);
                if (!node)
                        return CDC_STATUS_BAD_ALLOC;
        } else {
                node->value = value;
        }

        if (ret) {
                (*ret).first.container = t;
                (*ret).first.current = node;
                (*ret).first.prev = predecessor(node);
                (*ret).second = !finded;
        }

        return CDC_STATUS_OK;
}

size_t cdc_splay_tree_erase(struct cdc_splay_tree *t, void *key)
{
        assert(t != NULL);

        struct cdc_splay_tree_node *node = find_node(t->root, key, t->compar);

        if (!node)
                return 0;

        node = splay(node);
        if (node->left)
                node->left->parent = NULL;

        if (node->right)
                node->right->parent = NULL;

        t->root = merge(node->left, node->right);
        free_node(t, node);
        --t->size;
        return 1;
}

void cdc_splay_tree_clear(struct cdc_splay_tree *t)
{
        assert(t != NULL);

        free_splay_tree(t, t->root);
        t->size = 0;
        t->root = NULL;
}

void cdc_splay_tree_swap(struct cdc_splay_tree *a, struct cdc_splay_tree *b)
{
        assert(a != NULL);
        assert(b != NULL);

        CDC_SWAP(struct cdc_splay_tree_node *, a->root, b->root);
        CDC_SWAP(size_t, a->size, b->size);
        CDC_SWAP(cdc_binary_pred_fn_t, a->compar, b->compar);
        CDC_SWAP(struct cdc_data_info *, a->dinfo, b->dinfo);
}

void cdc_splay_tree_begin(struct cdc_splay_tree *t, struct cdc_splay_tree_iter *it)
{
        assert(t != NULL);
        assert(it != NULL);

        it->container = t;
        it->current = min_node(t->root);
        it->prev = NULL;
}

void cdc_splay_tree_end(struct cdc_splay_tree *t, struct cdc_splay_tree_iter *it)
{
        assert(t != NULL);
        assert(it != NULL);

        it->container = t;
        it->current = NULL;
        it->prev = max_node(t->root);
}

void cdc_splay_tree_iter_next(struct cdc_splay_tree_iter *it)
{
        assert(it != NULL);

        it->prev = it->current;
        it->current = successor(it->current);
}

void cdc_splay_tree_iter_prev(struct cdc_splay_tree_iter *it)
{
        assert(it != NULL);

        it->current = it->prev;
        it->prev = predecessor(it->current);
}
