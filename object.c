// Phase 1 progress
#include "pes.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
// directory creation logic
#define HASH_SIZE 32

// REQUIRED by tests
void hash_to_hex(const ObjectID *id, char *hex_out) {
    for (int i = 0; i < HASH_SIZE; i++) {
        sprintf(hex_out + i * 2, "%02x", id->hash[i]);
    }
    hex_out[64] = '\0';
}
// implemented object_read
// REQUIRED by tests
void object_path(const ObjectID *id, char *path_out) {
    char hex[65];
    hash_to_hex(id, hex);
    sprintf(path_out, ".pes/objects/%.2s/%s", hex, hex + 2);
}

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out) {
    (void)type;

    char header[64];
    int header_len = snprintf(header, sizeof(header), "blob %zu", len) + 1;

    size_t total_len = header_len + len;
    unsigned char *full = malloc(total_len);
    if (!full) return -1;

    memcpy(full, header, header_len);
    memcpy(full + header_len, data, len);

    unsigned char hash[HASH_SIZE];
    EVP_Digest(full, total_len, hash, NULL, EVP_sha256(), NULL);

    if (id_out) {
        memcpy(id_out->hash, hash, HASH_SIZE);
    }

    char hex[65];
    hash_to_hex(id_out, hex);

    mkdir(".pes", 0755);
    mkdir(".pes/objects", 0755);

    char dir[256];
    snprintf(dir, sizeof(dir), ".pes/objects/%.2s", hex);
    mkdir(dir, 0755);

    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, hex + 2);

    FILE *f = fopen(path, "wb");
    if (!f) {
        free(full);
        return -1;
    }

    fwrite(full, 1, total_len, f);
    fclose(f);
    free(full);

    return 0;
}

int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out) {
    if (type_out) {
    *type_out = OBJ_BLOB;
}

    char path[512];
    object_path(id, path);

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    unsigned char *buf = malloc(size);
    if (!buf) {
        fclose(f);
        return -1;
    }

    fread(buf, 1, size, f);
    fclose(f);

    unsigned char *null_ptr = memchr(buf, '\0', size);
    if (!null_ptr) {
        free(buf);
        return -1;
    }

   size_t data_len = size - (null_ptr - buf) - 1;

void *data = malloc(data_len);
if (!data) {
    free(buf);
    return -1;
}

memcpy(data, null_ptr + 1, data_len);

*data_out = data;
*len_out = data_len;

free(buf);
    return 0;
}
