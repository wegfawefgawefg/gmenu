#include "gmenu/gmenu.hpp"

#include <SDL3/SDL.h>
#include <array>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr gmenu::ScreenId kMain = 1;
constexpr gmenu::ScreenId kSettings = 2;
constexpr gmenu::ScreenId kProfiles = 3;

constexpr gmenu::StyleId kTitleStyle = 10;
constexpr gmenu::StyleId kButtonStyle = 20;
constexpr gmenu::StyleId kValueStyle = 30;

struct DemoState {
    bool fullscreen = false;
    float volume = 0.5f;
    int quality = 1;
    std::string profile_name = "Default";
    bool quit = false;
};

struct HeldInput {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

gmenu::CommandId g_quit_command = gmenu::invalid_command;

void command_quit(gmenu::BuildContext& ctx, int) {
    auto* state = static_cast<DemoState*>(ctx.user);
    state->quit = true;
}

gmenu::Widget styled(gmenu::Widget widget, gmenu::StyleId style) {
    widget.style = style;
    return widget;
}

std::vector<glayout::Layout> make_layouts() {
    glayout::Layout layout;
    layout.id = 100;
    layout.label = "desktop";
    layout.width = 1280;
    layout.height = 720;
    layout.objects.push_back(
        glayout::Object{1, "title", glayout::Rect{0.18f, 0.08f, 0.64f, 0.12f}});
    layout.objects.push_back(glayout::Object{2, "row0", glayout::Rect{0.24f, 0.27f, 0.52f, 0.09f}});
    layout.objects.push_back(glayout::Object{3, "row1", glayout::Rect{0.24f, 0.40f, 0.52f, 0.09f}});
    layout.objects.push_back(glayout::Object{4, "row2", glayout::Rect{0.24f, 0.53f, 0.52f, 0.09f}});
    layout.objects.push_back(glayout::Object{5, "row3", glayout::Rect{0.24f, 0.66f, 0.52f, 0.09f}});
    return {layout};
}

SDL_FRect to_sdl(glayout::Rect rect) {
    return SDL_FRect{rect.x, rect.y, rect.w, rect.h};
}

void set_color(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void draw_text(SDL_Renderer* renderer, float x, float y, const std::string& text) {
    SDL_RenderDebugText(renderer, x, y, text.c_str());
}

void draw_item(SDL_Renderer* renderer, const gmenu::DrawItem& item) {
    SDL_FRect rect = to_sdl(item.rect);

    if (item.type == gmenu::WidgetType::Label) {
        set_color(renderer, 244, 232, 190);
        draw_text(renderer, rect.x + 12.0f, rect.y + 16.0f, item.label);
        return;
    }

    if (item.state.focused) {
        set_color(renderer, 235, 184, 63);
    } else if (item.state.hovered) {
        set_color(renderer, 74, 111, 143);
    } else {
        set_color(renderer, 43, 70, 98);
    }
    SDL_RenderFillRect(renderer, &rect);

    set_color(renderer, item.state.focused ? 255 : 170, item.state.focused ? 244 : 190,
              item.state.focused ? 190 : 210);
    SDL_RenderRect(renderer, &rect);

    set_color(renderer, 248, 248, 238);
    draw_text(renderer, rect.x + 14.0f, rect.y + 14.0f, item.label);
    if (!item.value.empty()) {
        draw_text(renderer, rect.x + rect.w - 160.0f, rect.y + 14.0f, item.value);
    }
    if (item.state.editing) {
        draw_text(renderer, rect.x + rect.w - 24.0f, rect.y + 14.0f, "_");
    }
}

void apply_key(SDL_Keycode key, bool down, HeldInput& held, gmenu::Input& input) {
    switch (key) {
    case SDLK_UP:
    case SDLK_W:
        held.up = down;
        break;
    case SDLK_DOWN:
    case SDLK_S:
        held.down = down;
        break;
    case SDLK_LEFT:
    case SDLK_A:
        held.left = down;
        break;
    case SDLK_RIGHT:
    case SDLK_D:
        held.right = down;
        break;
    case SDLK_RETURN:
    case SDLK_SPACE:
        if (down) {
            input.select = true;
        }
        break;
    case SDLK_ESCAPE:
        if (down) {
            input.back = true;
        }
        break;
    case SDLK_BACKSPACE:
        if (down) {
            input.backspace = true;
        }
        break;
    case SDLK_Q:
        if (down) {
            input.page_prev = true;
        }
        break;
    case SDLK_E:
        if (down) {
            input.page_next = true;
        }
        break;
    default:
        break;
    }
}

} // namespace

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("gmenu SDL demo", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    DemoState state;
    std::vector<glayout::Layout> layouts = make_layouts();
    gmenu::Menu menu;
    menu.set_layouts(&layouts);
    menu.set_user_data(&state);
    g_quit_command = menu.register_command(command_quit);

    gmenu::ListScreenDef main_def;
    main_def.id = kMain;
    main_def.layout_id = 100;
    main_def.title_id = 1;
    main_def.title = "gmenu";
    main_def.title_style = kTitleStyle;
    main_def.default_focus = 10;
    main_def.items.push_back(
        gmenu::ListItem{10, "row0", "Play", "", gmenu::Action::none(), kButtonStyle});
    main_def.items.push_back(
        gmenu::ListItem{11, "row1", "Settings", "", gmenu::Action::push(kSettings), kButtonStyle});
    main_def.items.push_back(
        gmenu::ListItem{12, "row2", "Profiles", "", gmenu::Action::push(kProfiles), kButtonStyle});
    main_def.items.push_back(gmenu::ListItem{
        13, "row3", "Quit", "", gmenu::Action::command_id(g_quit_command), kButtonStyle});

    gmenu::BasicScreenDef settings_def;
    settings_def.id = kSettings;
    settings_def.layout_id = 100;
    settings_def.title_id = 2;
    settings_def.title = "settings";
    settings_def.title_style = kTitleStyle;
    settings_def.default_focus = 20;
    settings_def.widgets.push_back(
        styled(gmenu::toggle(20, "row0", "Fullscreen", state.fullscreen), kValueStyle));
    settings_def.widgets.push_back(styled(
        gmenu::slider_1d(21, "row1", "Volume", state.volume, 0.0f, 1.0f, 0.1f), kValueStyle));
    settings_def.widgets.push_back(
        styled(gmenu::option_cycle(22, "row2", "Quality", state.quality, {"low", "medium", "high"}),
               kValueStyle));
    settings_def.widgets.push_back(
        styled(gmenu::button(23, "row3", "Back", gmenu::Action::pop()), kButtonStyle));

    gmenu::BasicScreenDef profiles_def;
    profiles_def.id = kProfiles;
    profiles_def.layout_id = 100;
    profiles_def.title_id = 3;
    profiles_def.title = "profiles";
    profiles_def.title_style = kTitleStyle;
    profiles_def.default_focus = 30;
    profiles_def.widgets.push_back(
        styled(gmenu::text_input(30, "row0", "Name", state.profile_name, 32), kValueStyle));
    profiles_def.widgets.push_back(
        styled(gmenu::button(31, "row3", "Back", gmenu::Action::pop()), kButtonStyle));

    gmenu::register_list_screen(menu, main_def);
    gmenu::register_basic_screen(menu, settings_def);
    gmenu::register_basic_screen(menu, profiles_def);
    menu.set_root(kMain);

    HeldInput held;
    bool running = true;
    Uint64 last_ticks = SDL_GetTicks();

    while (running && !state.quit) {
        gmenu::Input input;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                apply_key(event.key.key, event.type == SDL_EVENT_KEY_DOWN, held, input);
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                input.text += event.text.text;
            }
        }

        input.up = held.up;
        input.down = held.down;
        input.left = held.left;
        input.right = held.right;
        input.mouse_valid = true;
        float mouse_x = 0.0f;
        float mouse_y = 0.0f;
        SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        input.mouse_x = mouse_x;
        input.mouse_y = mouse_y;
        input.mouse_down = (buttons & SDL_BUTTON_LMASK) != 0;

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        Uint64 now = SDL_GetTicks();
        float dt = static_cast<float>(now - last_ticks) / 1000.0f;
        last_ticks = now;
        menu.update(input, dt, width, height);

        set_color(renderer, 16, 22, 29);
        SDL_RenderClear(renderer);
        for (const gmenu::DrawItem& item : menu.draw_items()) {
            draw_item(renderer, item);
        }
        set_color(renderer, 155, 170, 185);
        draw_text(renderer, 18.0f, static_cast<float>(height) - 28.0f,
                  "arrows/wasd navigate, enter select, esc back, type in profile name");
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
