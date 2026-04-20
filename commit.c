#include "commit.h"
#include "tree.h"
#include "object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEAD_PATH ".pes/HEAD"
#define REF_PATH ".pes/refs/heads/main"

// ---------- HEAD READ ----------

int head_read(ObjectID *id_out) {
    FILE *f = fopen(REF_PATH, "r");
    if (!f) return -1;

    char hex[65];
    if (fscanf(f, "%64s", hex) != 1) {
        fclose(f);
        return -1;
    }
    fclose(f);

    for (int i = 0; i < 32; i++) {
        sscanf(hex + i * 2, "%2hhx", &id_out->hash[i]);
    }

    return 0;
}

// ---------- HEAD UPDATE ----------

int head_update(const ObjectID *id) {
    FILE *f = fopen(REF_PATH, "w");
    if (!f) return -1;

    char hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(hex + i * 2, "%02x", id->hash[i]);
    }
    hex[64] = '\0';

    fprintf(f, "%s\n", hex);
    fclose(f);
    return 0;
}

// ---------- COMMIT CREATE ----------

int commit_create(const char *message, ObjectID *id_out) {
    ObjectID tree_id;

    // minimal tree
    if (tree_from_index(&tree_id) != 0) {
        return -1;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf),
             "tree ");

    char tree_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(tree_hex + i * 2, "%02x", tree_id.hash[i]);
    }
    tree_hex[64] = '\0';

    strcat(buf, tree_hex);
    strcat(buf, "\n\n");
    strcat(buf, message);
    strcat(buf, "\n");

    if (object_write(OBJ_COMMIT, buf, strlen(buf), id_out) != 0) {
        return -1;
    }

    return head_update(id_out);
}
