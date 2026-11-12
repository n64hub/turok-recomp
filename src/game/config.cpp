// The settings: RecompFrontend's General, Controls, Graphics, Sound and Mods tabs, with a first-person shooter's default
// bindings. Its own Graphics options carry what a recomp needs: Resolution and MS Anti-Aliasing (high resolution),
// Framerate (interpolation to the display's rate) and Aspect Ratio and HUD Placement (widescreen).

#include <filesystem>
#include <fstream>

#include "recompinput/input_mapping.h"
#include "recompinput/input_types.h"
#include "recompui/config.h"
#include "util/file.h"
#include "turok1.h"

// The game walks and strafes on the C buttons and turns and looks with the control stick; Z fires, R jumps, A and B
// change weapon and L shows the map, as in the later games. Here that is the layout a shooter has today: walking on the
// left stick (and WASD), looking on the right stick (and the arrow keys, and the mouse: src/main/main.cpp), firing on
// the right trigger (and the left mouse button), jumping on the bottom face button (and Space), the weapons on the
// shoulder buttons (Q and E) and the map on the top face button (Tab). The game looks down with the stick pushed up, as
// a plane's stick, so the up arrow and the right stick pushed up push the game's stick down: up looks up.
static void set_default_bindings() {
    using recompinput::GameInput;
    using recompinput::InputField;
    auto key = [](GameInput input, std::initializer_list<SDL_Scancode> keys) {
        std::vector<InputField> fields;
        for (SDL_Scancode k : keys) {
            fields.push_back(InputField::keyboard(k));
        }
        recompinput::set_default_mapping_for_keyboard(input, fields);
    };
    key(GameInput::C_UP, { SDL_SCANCODE_W });
    key(GameInput::C_DOWN, { SDL_SCANCODE_S });
    key(GameInput::C_LEFT, { SDL_SCANCODE_A });
    key(GameInput::C_RIGHT, { SDL_SCANCODE_D });
    key(GameInput::X_AXIS_NEG, { SDL_SCANCODE_LEFT });
    key(GameInput::X_AXIS_POS, { SDL_SCANCODE_RIGHT });
    key(GameInput::Y_AXIS_NEG, { SDL_SCANCODE_UP });
    key(GameInput::Y_AXIS_POS, { SDL_SCANCODE_DOWN });
    key(GameInput::Z, { SDL_SCANCODE_LCTRL });
    key(GameInput::R, { SDL_SCANCODE_SPACE });
    key(GameInput::A, { SDL_SCANCODE_E });
    key(GameInput::B, { SDL_SCANCODE_Q });
    key(GameInput::L, { SDL_SCANCODE_TAB });
    key(GameInput::START, { SDL_SCANCODE_RETURN });
    key(GameInput::DPAD_LEFT, { SDL_SCANCODE_J });
    key(GameInput::DPAD_RIGHT, { SDL_SCANCODE_L });
    key(GameInput::DPAD_UP, { SDL_SCANCODE_I });
    key(GameInput::DPAD_DOWN, { SDL_SCANCODE_K });

    auto pad = [](GameInput input, std::initializer_list<InputField> fields) {
        recompinput::set_default_mapping_for_controller(input, fields);
    };
    pad(GameInput::C_UP, { InputField::controller_analog(SDL_CONTROLLER_AXIS_LEFTY, false) });
    pad(GameInput::C_DOWN, { InputField::controller_analog(SDL_CONTROLLER_AXIS_LEFTY, true) });
    pad(GameInput::C_LEFT, { InputField::controller_analog(SDL_CONTROLLER_AXIS_LEFTX, false) });
    pad(GameInput::C_RIGHT, { InputField::controller_analog(SDL_CONTROLLER_AXIS_LEFTX, true) });
    pad(GameInput::X_AXIS_NEG, { InputField::controller_analog(SDL_CONTROLLER_AXIS_RIGHTX, false) });
    pad(GameInput::X_AXIS_POS, { InputField::controller_analog(SDL_CONTROLLER_AXIS_RIGHTX, true) });
    pad(GameInput::Y_AXIS_NEG, { InputField::controller_analog(SDL_CONTROLLER_AXIS_RIGHTY, false) });
    pad(GameInput::Y_AXIS_POS, { InputField::controller_analog(SDL_CONTROLLER_AXIS_RIGHTY, true) });
    pad(GameInput::Z, { InputField::controller_analog(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, true) });
    pad(GameInput::R, { InputField::controller_digital(recompinput::SDL_CONTROLLER_BUTTON_SOUTH),
        InputField::controller_analog(SDL_CONTROLLER_AXIS_TRIGGERLEFT, true) });
    pad(GameInput::A, { InputField::controller_digital(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) });
    pad(GameInput::B, { InputField::controller_digital(SDL_CONTROLLER_BUTTON_LEFTSHOULDER) });
    pad(GameInput::L, { InputField::controller_digital(recompinput::SDL_CONTROLLER_BUTTON_NORTH) });
    pad(GameInput::START, { InputField::controller_digital(SDL_CONTROLLER_BUTTON_START) });
}

void t1::init_config() {
    set_default_bindings();

    std::filesystem::path app_dir = recompui::file::get_app_folder_path();
    if (!app_dir.empty()) {
        std::filesystem::create_directories(app_dir);
        // Mouse look on from the first start: RecompFrontend's Mouse Sensitivity defaults to 0, which turns it off (its
        // default is for gyro mapped to the mouse). Where the General settings were never saved, they start with it at
        // 50%; every other option is left out of the file and so at its default. A page reads its saved files in before
        // this runs, so a player's own setting is never overwritten.
        std::filesystem::path general = app_dir / "general.json";
        if (!std::filesystem::exists(general)) {
            std::ofstream(general) << "{ \"mouse_sensitivity\": 50 }\n";
        }
    }

    recompui::config::GeneralTabOptions general_options{};
    general_options.has_rumble_strength = true;
    general_options.has_gyro_sensitivity = true;
    general_options.has_mouse_sensitivity = true;
    recompui::config::create_general_tab(general_options);
    recompui::config::create_controls_tab();
    recompui::config::create_graphics_tab();
    recompui::config::create_sound_tab();
    recompui::config::create_mods_tab();
    recompui::config::finalize();
}
