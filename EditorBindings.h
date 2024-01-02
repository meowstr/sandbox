#pragma once

extern "C" {
struct editor_state_t {
    int selectedProperty;
};

struct editor_click_t {
    // in pixels, top-left corner is origin
    int x;
    int y;
};

struct editor_move_t {
    int deltaX; // -1, 0, 1
    int deltaY; // -1, 0, 1
};

struct editor_key_t {
    int keycode;
};
}
