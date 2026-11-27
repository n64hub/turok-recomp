// The Graphics tab's Field of View option, from us.toml's hooks: how wide the view is, 100% the game's own.
//
// The camera (func_80037E3C) works out its perspective's fovy, in degrees, once a frame: a base angle (the float at
// 0x80119740) times a factor of the camera's own (its struct's +0x15C), at 0x80038524. That one value goes to
// guPerspectiveF, into the camera's +0x34, and into the view pyramid the level is culled to (sp+0xB0), so scaling it
// there widens what is drawn and what is culled alike. A cinematic (the word at 0x80112B78 set, which also hides the
// HUD and draws Turok's body instead of the weapon) keeps the framing it was made with. The fovy is kept under
// kMaxFovy, where the view would fold over.
//
// The weapon in Turok's hands keeps the game's own view. It is the player's own object, slot 0 of the actor block, which
// the world's loop never draws: func_8003119C draws it last in the scene, after a depth clear, with func_8001C928
// (0x8003152C), and it loads only modelview matrices, so it draws with the projection the view loaded at its start
// (0x80038C70): the camera's matrix for the display list, at the pointer the camera reads from 0x800FA310 and keeps at
// its sp+0xB8. With it scaled, a wider view would draw the weapon smaller and further from the screen's corner and a
// narrower one larger, as Turok 2's did. So before the weapon a copy of the matrix with the game's own field of view
// goes into the display list, branched over, and is loaded, and after it the world's is loaded again. The camera's
// matrix is the view, then the perspective, then a scale on the axes, and a perspective's field of view scales only its
// x and y columns, by the cotangent of half its fovy: the copy is the camera's with those two columns scaled by
// tan(fovy / 2) / tan(own fovy / 2).
//
// The world draws with a second copy beside the weapon's, the camera's matrix as it is, and the view's load of the
// camera's matrix (searched for from where the frame starts its list, t1_field_of_view_frame_begin, to the weapon) is
// pointed at it. The renderer reads the list on its own thread once the game is done with it, and the high frame rate's
// interpolation pairs each frame's scenes with the frame before's by how alike their views are: in Turok 2 the world's
// matrix, read from where the game keeps it, was at times one written after the weapon's copy, a frame apart, and the
// frames drawn between the game's jumped back and forth (its v0.2.27 and v0.2.28). With both copies in the list,
// written at the same moment, the list holds one camera for the world and the weapon whenever it is read.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <string>

#include "recomp.h"
#include "recompui/config.h"

#include "turok1.h"

namespace {
    const std::string field_of_view_option = "field_of_view";
    // The setting, as a factor: read from the config at each change, so the camera reads a float a frame.
    std::atomic<float> field_of_view{ 1.0f };

    constexpr float kMaxFovy = 160.0f;

    constexpr uint32_t kCinematic = 0x80112B78;
    constexpr uint32_t kMtxSize = 0x40;
    constexpr uint32_t kLoadProjection = 0x01030040; // G_MTX (GBI 1): G_MTX_PROJECTION | G_MTX_LOAD, an Mtx's size
    constexpr uint32_t kBranch = 0x06010000;         // G_DL (GBI 1), G_DL_NOPUSH
    // How long a frame's list is at most: a head further from the frame's start, or before it, isn't in its list.
    constexpr uint32_t kMaxFrameBytes = 0x40000;

    // The frames counted by t1_field_of_view_frame_begin, and the frame the camera last made its matrix in: the
    // camera's matrix is the frame's own, so the weapon takes a copy of it only in that frame.
    uint32_t frame = 0;
    uint32_t camera_frame = UINT32_MAX;
    uint32_t frame_start = 0;

    // The camera's matrix for the display list this frame, and how much wider its view is than the game's own, as the
    // matrix's x and y columns are scaled.
    uint32_t camera_matrix = 0;
    double camera_scale = 1.0;

    // While the weapon is drawn: the matrix the view draws with after it (the world's copy, or the camera's own where
    // the view's load wasn't found). 0 when the weapon keeps the camera's matrix.
    uint32_t world_matrix = 0;

    double radians(double degrees) {
        return degrees * (3.14159265358979323846 / 180.0);
    }

    uint32_t read_word(uint8_t* rdram, uint32_t address) {
        return (uint32_t)MEM_W(0, (gpr)(int32_t)address);
    }

    void write_word(uint8_t* rdram, uint32_t address, uint32_t value) {
        MEM_W(0, (gpr)(int32_t)address) = (int32_t)value;
    }

    // An Mtx's element n (row * 4 + column), s15.16: the integer halves in its first 32 bytes, the fractions after.
    int32_t read_element(uint8_t* rdram, uint32_t mtx, int n) {
        uint32_t whole = read_word(rdram, mtx + 4 * (n / 2));
        uint32_t fraction = read_word(rdram, mtx + 32 + 4 * (n / 2));
        int shift = (n & 1) ? 0 : 16;
        return (int32_t)((((whole >> shift) & 0xFFFF) << 16) | ((fraction >> shift) & 0xFFFF));
    }

    // A copy of the matrix at from, at mtx, its x and y columns scaled by scale.
    void write_matrix(uint8_t* rdram, uint32_t mtx, uint32_t from, double scale) {
        int32_t elements[16];
        for (int n = 0; n < 16; n++) {
            int32_t value = read_element(rdram, from, n);
            if ((n & 3) < 2 && scale != 1.0) {
                double scaled = std::round((double)value * scale);
                value = (int32_t)std::clamp(scaled, (double)INT32_MIN, (double)INT32_MAX);
            }
            elements[n] = value;
        }
        for (int n = 0; n < 16; n += 2) {
            uint32_t a = (uint32_t)elements[n];
            uint32_t b = (uint32_t)elements[n + 1];
            write_word(rdram, mtx + 4 * (n / 2), (a & 0xFFFF0000) | (b >> 16));
            write_word(rdram, mtx + 32 + 4 * (n / 2), (a << 16) | (b & 0xFFFF));
        }
    }

    // Every load of the matrix at from, from start up to end, pointed at to; how many there were.
    int redirect_loads(uint8_t* rdram, uint32_t start, uint32_t end, uint32_t from, uint32_t to) {
        int count = 0;
        for (uint32_t command = start; command < end; command += 8) {
            if (read_word(rdram, command) == kLoadProjection && read_word(rdram, command + 4) == from) {
                write_word(rdram, command + 4, to);
                count++;
            }
        }
        return count;
    }
}

void t1::add_field_of_view_option(recomp::config::Config& graphics) {
    graphics.add_number_option(
        field_of_view_option,
        "Field of View",
        "How much of the world the view takes in: 100% is the game's own, more shows more at the sides and above and "
        "below, less zooms in. Cinematics keep the game's own view, and the weapon in Turok's hands its own look.",
        70, 150, 5, 0, true, 100);
    // Called on load too, with the saved value, and at each change.
    graphics.add_option_change_callback(field_of_view_option,
        [](recomp::config::ConfigValueVariant cur_value, recomp::config::ConfigValueVariant,
           recomp::config::OptionChangeContext) {
            if (const double* percent = std::get_if<double>(&cur_value)) {
                field_of_view.store((float)(*percent / 100.0));
            }
        });
}

// The camera's perspective's fovy in degrees, as the game made it, scaled by the setting; matrix is where the camera
// makes its matrix for the display list.
extern "C" float t1_field_of_view(uint8_t* rdram, float fovy, uint32_t matrix) {
    float scale = field_of_view.load(std::memory_order_relaxed);
    float scaled = fovy;
    if (scale != 1.0f && read_word(rdram, kCinematic) == 0) {
        scaled = std::min(fovy * scale, std::max(fovy, kMaxFovy));
    }
    camera_scale = 1.0;
    double own_half = std::tan(radians(fovy) / 2.0);
    if (scaled != fovy && own_half > 0.0) {
        camera_scale = std::tan(radians(scaled) / 2.0) / own_half;
    }
    camera_matrix = matrix;
    camera_frame = frame;
    return scaled;
}

// The frame (func_800023EC, with its graphics struct) begins its display list, through the head at +0x1F810.
extern "C" void t1_field_of_view_frame_begin(uint8_t* rdram, uint32_t graphics) {
    frame++;
    frame_start = read_word(rdram, graphics + 0x1F810);
}

// Before the weapon is drawn, through the display list head at head_var: the copies of the camera's matrix, the
// weapon's with the game's own field of view and the world's as it is, written into the list and branched over, the
// weapon's loaded, and the view's load of the camera's matrix pointed at the world's.
extern "C" void t1_weapon_view_begin(uint8_t* rdram, uint32_t head_var) {
    world_matrix = 0;
    if (camera_frame != frame || camera_scale == 1.0 || read_word(rdram, kCinematic) != 0) {
        return;
    }
    uint32_t list = read_word(rdram, head_var);
    uint32_t mtx = list + 16;
    uint32_t end = mtx + kMtxSize;
    write_matrix(rdram, mtx, camera_matrix, camera_scale);
    world_matrix = camera_matrix;
    bool in_frame = frame_start != 0 && list >= frame_start && list - frame_start <= kMaxFrameBytes &&
        (frame_start & 7) == 0;
    if (in_frame && redirect_loads(rdram, frame_start, list, camera_matrix, end) != 0) {
        world_matrix = end;
        write_matrix(rdram, world_matrix, camera_matrix, 1.0);
        end += kMtxSize;
    }
    write_word(rdram, list, kLoadProjection);
    write_word(rdram, list + 4, mtx);
    write_word(rdram, list + 8, kBranch);
    write_word(rdram, list + 12, end);
    write_word(rdram, head_var, end);
}

// After the weapon is drawn (or skipped): the world's matrix is loaded again.
extern "C" void t1_weapon_view_end(uint8_t* rdram, uint32_t head_var) {
    if (world_matrix == 0) {
        return;
    }
    uint32_t list = read_word(rdram, head_var);
    write_word(rdram, list, kLoadProjection);
    write_word(rdram, list + 4, world_matrix);
    write_word(rdram, head_var, list + 8);
    world_matrix = 0;
}
