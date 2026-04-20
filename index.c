#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define INDEX_PATH ".pes/index"
#define MAX_ENTRIES 1024

static void ensure_repo() {
    mkdir(".pes", 0755);
}

// ---------- LOAD ----------

int index_load(Index *idx) {
    FILE *f = fopen(INDEX_PATH, "r");
    idx->count = 0;

    if (!f) return 0; // empty index is fine

    while (!feof(f)) {
        IndexEntry *e = &idx->entries[idx->count];

        char hash_hex[65];
        if (fscanf(f, "%o %64s %ld %ld %s\n",
                   &e->mode, hash_hex, &e->mtime, &e->size, e->path) != 5) {
            break;
        }

        // convert hex → bytes
        for (int i = 0; i < 32; i++) {
            sscanf(hash_hex + i * 2, "%2hhx", &e->id.hash[i]);
        }

        idx->count++;
    }

    fclose(f);
    return 0;
}

// ---------- SAVE ----------

int index_save(const Index *idx) {
    ensure_repo();

    FILE *f = fopen(INDEX_PATH, "w");
    if (!f) return -1;

    for (size_t i = 0; i < idx->count; i++) {
        const IndexEntry *e = &idx->entries[i];

        char hex[65];
        for (int j = 0; j < 32; j++) {
            sprintf(hex + j * 2, "%02x", e->id.hash[j]);
        }
        hex[64] = '\0';

        fprintf(f, "%o %s %ld %ld %s\n",
                e->mode, hex, e->mtime, e->size, e->path);
    }

    fclose(f);
    return 0;
}

// ---------- ADD ----------

int index_add(Index *idx, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    void *buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, buf, size, &id) != 0) {
        free(buf);
        return -1;
    }

    free(buf);

    IndexEntry *e = &idx->entries[idx->count++];

    e->mode = st.st_mode;
    e->mtime = st.st_mtime;
    e->size = size;
    strcpy(e->path, path);
    memcpy(e->id.hash, id.hash, 32);

    return 0;
}

// ---------- STATUS ----------

int index_status(const Index *idx) {
    printf("Staged changes:\n");

    if (idx->count == 0) {
        printf("    (nothing to show)\n");
    } else {
        for (size_t i = 0; i < idx->count; i++) {
            printf("    new file:   %s\n", idx->entries[i].path);
        }
    }

    printf("\nUnstaged changes:\n");
    printf("    (nothing to show)\n");

    printf("\nUntracked files:\n");
    printf("    (nothing to show)\n");

    return 0;
}
