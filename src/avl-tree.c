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
#include "cdcontainers/avl-tree.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "data-info.h"

static void free_node(struct cdc_avl_tree *t, struct cdc_avl_tree_node *node)
{
        assert(t != NULL);

        struct cdc_pair pair;

        if (CDC_HAS_DFREE(t)) {
                pair.first = node->key;
                pair.second = node->value;
                t->dinfo->dfree(&pair);
        }

        free(node);
}

static void free_avl_tree(struct cdc_avl_tree *t, struct cdc_avl_tree_node *root)
{
        assert(t != NULL);

        if (root == NULL)
                return;

        free_avl_tree(t, root->left);
        free_avl_tree(t, root->right);
        free_node(t, root);
}

static struct cdc_avl_tree_node *new_node(void *key, void *val)
{
        struct cdc_avl_tree_node *node;

        node = (struct cdc_avl_tree_node *)malloc(sizeof(struct cdc_avl_tree_node));
        if (node) {
                node->key = key;
                node->value = val;
                node->height = 1;
                node->parent = node->left = node->right = NULL;
        }

        return node;
}

static struct cdc_avl_tree_node *find_node(struct cdc_avl_tree_node *node,
                                           void *key, cdc_binary_pred_fn_t cmp)
{
        while (node && cdc_not_eq(cmp, node->key, key)) {
                if (cmp(key, node->key))
                        node = node->left;
                else
                        node = node->right;
        }

        return node;
}

static struct cdc_avl_tree_node *min_node(struct cdc_avl_tree_node *node)
{
        if (node == NULL)
                return NULL;

        while (node->left != NULL)
                node = node->left;

        return node;
}

static struct cdc_avl_tree_node *max_node(struct cdc_avl_tree_node *node)
{
        if (node == NULL)
                return NULL;

        while (node->right != NULL)
                node = node->right;

        return node;
}

static struct cdc_avl_tree_node *successor(struct cdc_avl_tree_node *node)
{
        struct cdc_avl_tree_node *p;

        if (node->right)
                return min_node(node->right);

        p = node->parent;
        while (p && node == p->right) {
                node = p;
                p = p->parent;
        }

        return p;
}

static struct cdc_avl_tree_node *predecessor(struct cdc_avl_tree_node *node)
{
        struct cdc_avl_tree_node *p;

        if (node->left)
                return max_node(node->left);

        p = node->parent;
        while (p && node == p->left) {
                node = p;
                p = p->parent;
        }

        return p;
}

static unsigned char height(struct cdc_avl_tree_node* node)
{
        return node ? node->height : 0;
}

static int height_diff(struct cdc_avl_tree_node* node)
{
        return height(node->right) - height(node->left);
}

static void update_height(struct cdc_avl_tree_node* node)
{
        unsigned char hl = height(node->left), hr = height(node->right);

        node->height = CDC_MAX(hl, hr) + 1;
}

static void update_link(struct cdc_avl_tree_node *parent,
                        struct cdc_avl_tree_node *old,
                        struct cdc_avl_tree_node *node)
{
        if (!parent)
                return;

        if (parent->left == old)
                parent->left = node;
        else
                parent->right = node;
}

static void unlink_list(struct cdc_avl_tree_node *p, struct cdc_avl_tree_node *node)
{
        if (!p)
                return;

        if (p->left == node)
                p->left = NULL;
        else
                p->right = NULL;
}

static struct cdc_avl_tree_node *rotate_right(struct cdc_avl_tree_node *node)
{
        struct cdc_avl_tree_node* q = node->left;

        update_link(node->parent, node, q);
        q->parent = node->parent;
        node->left = q->right;
        if (node->left)
                node->left->parent = node;

        q->right = node;
        if (q->right)
                q->right->parent = q;

        update_height(node);
        update_height(q);
        return q;
}

static struct cdc_avl_tree_node *rotate_left(struct cdc_avl_tree_node *node)
{
        struct cdc_avl_tree_node* q = node->right;

        update_link(node->parent, node, q);
        q->parent = node->parent;
        node->right = q->left;
        if (node->right)
                node->right->parent = node;

        q->left = node;
        if (q->left)
                q->left->parent = q;

        update_height(node);
        update_height(q);
        return q;
}

static struct cdc_avl_tree_node *balance(struct cdc_avl_tree_node *node)
{
        while (node) {
                update_height(node);
                if (height_diff(node) == 2) {
                        if (height_diff(node->right) < 0)
                                node->right = rotate_right(node->right);
                        node = rotate_left(node);
                } else if (height_diff(node) == -2) {
                        if (height_diff(node->left) > 0)
                                node->left = rotate_left(node->left);
                        node = rotate_right(node);
                } else {
                        if (node->parent)
                                node = node->parent;
                        else
                                break;
                }
        }

        return node;
}

static struct cdc_avl_tree_node *make_and_insert_unique(struct cdc_avl_tree *t,
                                                        void *key, void *value,
                                                        bool *inserted)
{
        struct cdc_avl_tree_node *tmp = t->root, *node;

        if (t->root == NULL) {
                node = new_node(key, value);
                t->root = node;
        } else {
                while (true) {
                        if (t->compar(key, tmp->key)) {
                                if (tmp->left) {
                                        tmp = tmp->left;
                                } else {
                                        node = new_node(key, value);
                                        tmp->left = node;
                                        node->parent = tmp;
                                        break;
                                }
                        } else if (t->compar(tmp->key, key)) {
                                if (tmp->right) {
                                        tmp = tmp->right;
                                } else {
                                        node = new_node(key, value);
                                        tmp->right = node;
                                        node->parent = tmp;
                                        break;
                                }
                        } else {
                                *inserted = false;
                                return tmp;
                        }
                }

                t->root = balance(tmp);
        }

        ++t->size;
        *inserted = true;
        return node;
}

static struct cdc_avl_tree_node *erase_node(struct cdc_avl_tree *t,
                                            struct cdc_avl_tree_node *node)
{
        struct cdc_avl_tree_node *mnode = NULL, *parent;

        if (node->left == NULL && node->right == NULL) {
                parent = node->parent;
                unlink_list(parent, node);
        } else if (node->right == NULL) {
                parent = node->parent;
                if (parent->left == node) {
                        parent->left = node->left;
                        parent->left->parent = parent;
                } else {
                        parent->right = node->left;
                        parent->right->parent = parent;
                }
        } else {
                mnode = min_node(node->right);
                parent = mnode->parent;
                unlink_list(parent, mnode);
                if (parent == node)
                        parent = node->parent;

                update_link(parent, node, mnode);
                mnode->left = node->left;
                mnode->right = node->right;
                mnode->parent = node->parent;
                if (mnode->left)
                        mnode->left->parent = mnode;

                if (mnode->right)
                        mnode->right->parent = mnode;
        }

        free_node(t, node);
        return balance(parent ? parent : mnode);
}


static enum cdc_stat init_varg(struct cdc_avl_tree *t, va_list args)
{
        enum cdc_stat stat;
        struct cdc_pair *pair;

        while ((pair = va_arg(args, struct cdc_pair *)) != NULL) {
                stat = cdc_avl_tree_insert(t, pair->first, pair->second, NULL);
                if (stat != CDC_STATUS_OK)
                        return stat;
        }

        return CDC_STATUS_OK;
}

enum cdc_stat cdc_avl_tree_ctor(struct cdc_avl_tree **t, struct cdc_data_info *info,
                                cdc_binary_pred_fn_t compar)
{
        assert(t != NULL);

        struct cdc_avl_tree *tmp;

        tmp = (struct cdc_avl_tree *)calloc(sizeof(struct cdc_avl_tree), 1);
        if (!tmp)
                return CDC_STATUS_BAD_ALLOC;

        if (info && !(tmp->dinfo = cdc_di_shared_ctorc(info))) {
                free(tmp);
                return CDC_STATUS_BAD_ALLOC;
        }

        tmp->compar = compar;
        *t = tmp;
        return CDC_STATUS_OK;
}

enum cdc_stat cdc_avl_tree_ctorl(struct cdc_avl_tree **t, struct cdc_data_info *info,
                                 cdc_binary_pred_fn_t compar, ...)
{
        assert(t != NULL);
        assert(compar != NULL);

        enum cdc_stat stat;
        va_list args;

        va_start(args, compar);
        stat = cdc_avl_tree_ctorv(t, info, compar, args);
        va_end(args);

        return stat;
}

enum cdc_stat cdc_avl_tree_ctorv(struct cdc_avl_tree **t, struct cdc_data_info *info,
                                 cdc_binary_pred_fn_t compar, va_list args)
{
        assert(t != NULL);
        assert(compar != NULL);

        enum cdc_stat stat;

        stat = cdc_avl_tree_ctor(t, info, compar);
        if (stat != CDC_STATUS_OK)
                return stat;

        return init_varg(*t, args);
}

void cdc_avl_tree_dtor(struct cdc_avl_tree *t)
{
        assert(t != NULL);

        free_avl_tree(t, t->root);
        cdc_di_shared_dtor(t->dinfo);
        free(t);
}

enum cdc_stat cdc_avl_tree_get(struct cdc_avl_tree *t, void *key, void **value)
{
        assert(t != NULL);

        struct cdc_avl_tree_node *node = find_node(t->root, key, t->compar);

        if (node)
                *value = node->value;

        return node ? CDC_STATUS_OK : CDC_STATUS_NOT_FOUND;
}

size_t cdc_avl_tree_count(struct cdc_avl_tree *t, void *key)
{
        assert(t != NULL);

        return (size_t)(find_node(t->root, key, t->compar) != NULL);
}

void cdc_avl_tree_find(struct cdc_avl_tree *t, void *key,
                       struct cdc_avl_tree_iter *it)
{
        assert(t != NULL);
        assert(it != NULL);

        struct cdc_avl_tree_node *node = find_node(t->root, key, t->compar);

        if (!node) {
                cdc_avl_tree_end(t, it);
                return;
        }

        it->container = t;
        it->current = node;
        it->prev = predecessor(node);
}

void cdc_avl_tree_equal_range(struct cdc_avl_tree *t,  void *key,
                              struct cdc_pair_avl_tree_iter *pair)
{
        assert(t != NULL);
        assert(pair != NULL);

        cdc_avl_tree_find(t, key, &pair->first);
        cdc_avl_tree_end(t, &pair->second);
        if (cdc_avl_tree_iter_is_eq(&pair->first, &pair->second)) {
                cdc_avl_tree_end(t, &pair->first);
        } else {
                pair->second = pair->first;
                cdc_avl_tree_iter_next(&pair->second);
        }
}

enum cdc_stat cdc_avl_tree_insert(struct cdc_avl_tree *t, void *key, void *value,
                                  struct cdc_pair_avl_tree_iter_bool *ret)
{
        assert(t != NULL);

        bool inserted;
        struct cdc_avl_tree_node *node;

        node = make_and_insert_unique(t, key, value, &inserted);
        if (inserted && !node)
                return CDC_STATUS_BAD_ALLOC;

        if (ret) {
                (*ret).first.container = t;
                (*ret).first.current = node;
                (*ret).first.prev = predecessor(node);
                (*ret).second = inserted;
        }

        return CDC_STATUS_OK;
}

enum cdc_stat cdc_avl_tree_insert_or_assign(struct cdc_avl_tree *t,
                                            void *key, void *value,
                                            struct cdc_pair_avl_tree_iter_bool *ret)
{
        assert(t != NULL);

        bool inserted;
        struct cdc_avl_tree_node *node;

        node = make_and_insert_unique(t, key, value, &inserted);
        if (inserted && !node)
                return CDC_STATUS_BAD_ALLOC;

        if (!inserted)
                node->value = value;

        if (ret) {
                (*ret).first.container = t;
                (*ret).first.current = node;
                (*ret).first.prev = predecessor(node);
                (*ret).second = inserted;
        }

        return CDC_STATUS_OK;
}

size_t cdc_avl_tree_erase(struct cdc_avl_tree *t, void *key)
{
        assert(t != NULL);

        struct cdc_avl_tree_node *node = find_node(t->root, key, t->compar);

        if (!node)
                return 0;

        t->root = erase_node(t, node);
        --t->size;
        return 1;
}

void cdc_avl_tree_clear(struct cdc_avl_tree *t)
{
        assert(t != NULL);

        free_avl_tree(t, t->root);
        t->size = 0;
        t->root = NULL;
}

void cdc_avl_tree_swap(struct cdc_avl_tree *a, struct cdc_avl_tree *b)
{
        assert(a != NULL);
        assert(b != NULL);

        CDC_SWAP(struct cdc_avl_tree_node *, a->root, b->root);
        CDC_SWAP(size_t, a->size, b->size);
        CDC_SWAP(cdc_binary_pred_fn_t, a->compar, b->compar);
        CDC_SWAP(struct cdc_data_info *, a->dinfo, b->dinfo);
}

void cdc_avl_tree_begin(struct cdc_avl_tree *t, struct cdc_avl_tree_iter *it)
{
        assert(t != NULL);
        assert(it != NULL);

        it->container = t;
        it->current = min_node(t->root);
        it->prev = NULL;
}

void cdc_avl_tree_end(struct cdc_avl_tree *t, struct cdc_avl_tree_iter *it)
{
        assert(t != NULL);
        assert(it != NULL);

        it->container = t;
        it->current = NULL;
        it->prev = max_node(t->root);
}

void cdc_avl_tree_iter_next(struct cdc_avl_tree_iter *it)
{
        assert(it != NULL);

        it->prev = it->current;
        it->current = successor(it->current);
}

void cdc_avl_tree_iter_prev(struct cdc_avl_tree_iter *it)
{
        assert(it != NULL);

        it->current = it->prev;
        it->prev = predecessor(it->current);
}
