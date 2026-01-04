// Saving and loading state

#include "emulated.h"

#include <stdio.h>
#include <stdbool.h>

bool emulated_state_save(struct EmulatedSystem *emulated_system, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) return false;
    else if (fwrite(emulated_system, sizeof(struct EmulatedSystem), 1, file) != 1) {
        fclose(file);
        return false;
    }
    else {
        fclose(file);
        return true;
    }
}

bool emulated_state_load(struct EmulatedSystem *emulated_system, const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
      fprintf(stderr, "Não foi possível encontrar o save %s\n", filename);
      return false;
    }
    else if (fread(emulated_system, sizeof(struct EmulatedSystem), 1, file) != 1) {
        fprintf(stderr, "Não foi possível ler o save %s\n", filename);
        fclose(file);
        return false;
    }
    else {
        fclose(file);
        return true;
    }
}
