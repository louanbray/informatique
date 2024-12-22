#include "parser.h"

#include <stdio.h>

/// @brief Read and parse the given file using the dodjo format to update the chunk decorations
/// @param d Array of chunk decorations/items
/// @param filename file to read
void parse_chunk_file(dynarray* d, char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    int x, y, t, di;
    while (fscanf(file, "%d,%d,%d,%d", &x, &y, &t, &di) == 4)
        append(d, create_item(x, y, t, di));

    fclose(file);
}

void parse_chunk(dynarray* d, int type) {
    switch (type) {
        case SPAWN:
            parse_chunk_file(d, "assets/chunk_spawn.dodjo");
            break;
        default:
            parse_chunk_file(d, "assets/chunk_default.dodjo");
            break;
    }
}