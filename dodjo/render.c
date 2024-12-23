#include "render.h"

#include <locale.h>
#include <omp.h>
#include <string.h>

#include "dynarray.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "player.h"

typedef struct renderbuffer {
    board bd;
    board pv;
    int* rc;
} renderbuffer;

/// @brief Render Constants
const int RENDER_WIDTH = 130;
const int RENDER_HEIGHT = 40;

int abs(int x) { return x > 0 ? x : -x; }

/// @brief Create a board object
/// @return board
board create_board() {
    wchar_t* data = malloc(sizeof(wchar_t) * RENDER_WIDTH * RENDER_HEIGHT);
    board b = malloc(sizeof(wchar_t*) * RENDER_HEIGHT);
    for (int i = 0; i < RENDER_HEIGHT; i++) {
        b[i] = &data[i * RENDER_WIDTH];
    }
    return b;
}

void blank_screen(board b) {
    for (int i = 0; i < RENDER_HEIGHT; i++) {
        for (int j = 0; j < RENDER_WIDTH; j++) {
            b[i][j] = ' ';
        }
    }
}
void default_screen(board b) {
    for (int i = 0; i < RENDER_HEIGHT; i++) {
        for (int j = 0; j < RENDER_WIDTH; j++) {
            if (i > 0 && (j == 0 || j == RENDER_WIDTH - 1)) {
                if (j == 0) {
                    if (i == 3) {
                        b[i][j] = 9568;
                    } else if (i == RENDER_HEIGHT - 1) {
                        b[i][j] = 9556;
                    } else {
                        b[i][j] = 9553;
                    }
                } else if (j == RENDER_WIDTH - 1) {
                    if (i == 3) {
                        b[i][j] = 9571;
                    } else if (i == RENDER_HEIGHT - 1) {
                        b[i][j] = 9559;
                    } else {
                        b[i][j] = 9553;
                    }
                }
            } else if ((i == 0 || i == RENDER_HEIGHT - 1) || i == 3) {
                if (j != 0 && j != RENDER_WIDTH - 1) {
                    b[i][j] = 9552;
                } else if (i == 0) {
                    if (j == 0) {
                        b[i][j] = 9562;
                    } else {
                        b[i][j] = 9565;
                    }
                }
            } else if (i == 2 && (abs(RENDER_WIDTH / 2 - j) < 10 && j % 2 == 0)) {
                b[i][j] = 9145;
            } else {
                b[i][j] = ' ';
            }
        }
    }
}

/// @brief Create the row changed flag array
/// @return row changed array
int* create_rc() {
    return calloc(RENDER_HEIGHT, sizeof(int));
}

renderbuffer* create_screen() {
    renderbuffer* r = malloc(sizeof(renderbuffer));

    board b = create_board();
    default_screen(b);

    board pv = create_board();
    blank_screen(pv);

    r->bd = b;
    r->pv = pv;
    r->rc = create_rc();
    return r;
}

board get_board(renderbuffer* r) {
    return r->bd;
}

/// @brief Place char c on the board with center based coordinates
/// @param b board
/// @param x centered x pos
/// @param y centered y pos
/// @param c char to display
void render_char(board b, int x, int y, int c) {
    b[y + 1 + RENDER_HEIGHT / 2][x + RENDER_WIDTH / 2] = c;
}

void render_chunk(renderbuffer* r, chunk* c) {
    dynarray* d = get_chunk_furniture_list(c);
    board b = r->bd;
    for (int i = 0; i < len_dyn(d); i++) {
        item* it = get_dyn(d, i);
        render_char(b, get_item_x(it), get_item_y(it), get_item_display(it));
    }

    render_char(b, -41, 15, 'x');
    render_char(b, -41, 14, 'y');
    render_char(b, -40, 15, get_chunk_x(c) + 48);
    render_char(b, -40, 14, get_chunk_y(c) + 48);
}

void render_player(renderbuffer* r, player* p) {
    render_char(r->bd, get_player_px(p), get_player_py(p), ' ');
    render_char(r->bd, get_player_x(p), get_player_y(p), get_player_design(p));
}

void render_hotbar(renderbuffer* r, hotbar* h) {
    int display = 57;
    for (int i = 0; i < get_hotbar_max_size(h); i++) {
        if (get_hotbar(h, i) == NULL) {
            r->bd[2][display] = ' ';
        } else {
            r->bd[2][display] = get_item_display(get_hotbar(h, i));
        }
        if (get_selected_slot(h) == i) {
            r->bd[1][display] = 9651;
        } else {
            r->bd[1][display] = ' ';
        }
        display += 2;
    }
}

void render(renderbuffer* r, map* m) {
    player* p = get_player(m);
    render_from_player(r, p);
}

void render_from_player(renderbuffer* r, player* p) {
    chunk* curr = get_player_chunk(p);
    default_screen(r->bd);
    render_chunk(r, curr);
    render_player(r, p);
    render_hotbar(r, get_player_hotbar(p));
}

/// @brief Print only modified parts of the screen based on the buffer
/// @param r render buffer
void update_screen_(renderbuffer* r) {
    for (int i = RENDER_HEIGHT - 1; i >= 0; i--) {
        if (!r->rc[i]) continue;

        int screen_row = RENDER_HEIGHT - i;
        r->rc[i] = 0;
        wchar_t buffer[RENDER_WIDTH + 1];
        buffer[RENDER_WIDTH] = L'\0';  // Null-terminate the buffer

        int start = -1;
        for (int j = 0; j < RENDER_WIDTH; j++) {
            if (r->bd[i][j] != r->pv[i][j]) {
                if (start == -1) start = j;
                buffer[j] = r->bd[i][j];
                r->pv[i][j] = r->bd[i][j];
            } else if (start != -1) {
                buffer[j] = L'\0';
                wprintf(L"\033[%d;%dH%ls", screen_row, start + 1, &buffer[start]);
                start = -1;
            }
        }
        if (start != -1) {
            buffer[RENDER_WIDTH] = L'\0';
            wprintf(L"\033[%d;%dH%ls", screen_row, start + 1, &buffer[start]);
        }
    }
    fflush(stdout);
}

/// @brief Update changed rows between buffer arrays
/// @param r render buffer
void mark_changed_rows(renderbuffer* r) {
#pragma omp parallel for
    for (int i = 0; i < RENDER_HEIGHT; i++) {
        int changed = 0;
        for (int j = 0; j < RENDER_WIDTH; j++) {
            if (r->bd[i][j] != r->pv[i][j]) {
                changed = 1;
                break;
            }
        }
        r->rc[i] = changed;
    }
}

void update_screen(renderbuffer* r) {
    mark_changed_rows(r);
    update_screen_(r);
}