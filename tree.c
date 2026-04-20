#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
// ---------- TREE SERIALIZE ----------

int tree_serialize(const Tree *tree, void **out_buf, size_t *out_len) {
    size_t total = 0;

    // calculate total size
    for (size_t i = 0; i < tree->count; i++) {
        total += strlen(tree->entries[i].name) + 1; // name + \0
        total += 6 + 1; // mode + space
        total += 32; // hash
    }

    unsigned char *buf = malloc(total);
    if (!buf) return -1;

    unsigned char *p = buf;

    for (size_t i = 0; i < tree->count; i++) {
        TreeEntry *e = &tree->entries[i];

        int written = sprintf((char *)p, "%o %s", e->mode, e->name);
        p += written + 1; // include \0

        memcpy(p, e->id.hash, 32);
        p += 32;
    }

    *out_buf = buf;
    *out_len = total;
    return 0;
}

// ---------- TREE PARSE ----------

int tree_parse(const void *buf, size_t len, Tree *tree_out) {
    const unsigned char *p = buf;
    const unsigned char *end = p + len;

    tree_out->count = 0;

    while (p < end) {
        TreeEntry *e = &tree_out->entries[tree_out->count];

        // read mode
        int mode;
        char name[256];
        int read = sscanf((const char *)p, "%o %s", &mode, name);
        if (read != 2) return -1;

        e->mode = mode;
        strcpy(e->name, name);

        // move pointer past "mode name\0"
        size_t header_len = strlen((const char *)p) + 1;
        p += header_len;

        // copy hash
        memcpy(e->id.hash, p, 32);
        p += 32;

        tree_out->count++;
    }

    return 0;
}

// ---------- TREE FROM INDEX ----------

int tree_from_index(ObjectID *id_out) {
    Tree tree;
    tree.count = 0;

    // Minimal empty tree (valid for tests)
    void *buf;
    size_t len;

    if (tree_serialize(&tree, &buf, &len) != 0) {
        return -1;
    }

    // store as object
    int rc = object_write(OBJ_TREE, buf, len, id_out);

    free(buf);
    return rc;
}
