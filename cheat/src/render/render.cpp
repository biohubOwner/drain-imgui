#include <render/render.h>
#include <dwmapi.h>
#include <cstdio>
#include <chrono>
#include <thread>

#include <render/framework/data/fonts.h>
#include <features/cheats/visuals/visuals.h>
#include <render/framework/settings/functions.h>
#include <features/cache/cache.h>
#include <imgui/imgui_freetype.h>

#include <globals.h>
#include <memory/driver_interface.h>

enum WINDOWCOMPOSITIONATTRIB
{
    WCA_ACCENT_POLICY = 19
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

enum ACCENT_STATE
{
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_INVALID_STATE = 5
};

struct ACCENT_POLICY
{
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
};

typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

void set_blur_behind(HWND hwnd, bool enable, float intensity)
{
    static pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(GetModuleHandleA("user32.dll"), "SetWindowCompositionAttribute");
    if (SetWindowCompositionAttribute)
    {
        ACCENT_POLICY policy = { enable ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_DISABLED, 0, 0, 0 };
        if (enable)
        {
            DWORD alpha = (DWORD)(intensity * 255.0f);
            policy.GradientColor = (alpha << 24) | (0x000000 & 0xFFFFFF);
        }
        WINDOWCOMPOSITIONATTRIBDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(policy) };
        SetWindowCompositionAttribute(hwnd, &data);
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_F4) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_DESTROY:
        globals::running = false;
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

renderer::renderer()
{
    detail_ptr = std::make_unique<detail>();
}

renderer::~renderer()
{
    destroy_imgui();
    destroy_window();
    destroy_device();
}

bool renderer::create_window()
{
    detail_ptr->window_class.cbSize = sizeof(detail_ptr->window_class);
    detail_ptr->window_class.style = CS_CLASSDC;
    detail_ptr->window_class.lpszClassName = "Niggalantuh";
    detail_ptr->window_class.hInstance = GetModuleHandleA(0);
    detail_ptr->window_class.lpfnWndProc = wnd_proc;
    detail_ptr->window_class.hCursor = NULL;

    RegisterClassExA(&detail_ptr->window_class);

    detail_ptr->window = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        detail_ptr->window_class.lpszClassName,
        "Niggalantuh",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        0,
        0,
        detail_ptr->window_class.hInstance,
        0
    );

    if (!detail_ptr->window)
    {
        return false;
    }

    SetLayeredWindowAttributes(detail_ptr->window, RGB(0, 0, 0), 255, LWA_ALPHA);

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(detail_ptr->window, &margins);

    ShowWindow(detail_ptr->window, SW_SHOW);
    UpdateWindow(detail_ptr->window);

    return true;
}

bool renderer::create_device()
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};

    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;

    swap_chain_desc.BufferDesc.Width = 0;
    swap_chain_desc.BufferDesc.Height = 0;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    swap_chain_desc.OutputWindow = detail_ptr->window;

    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Windowed = 1;

    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;

    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL feature_level;
    D3D_FEATURE_LEVEL feature_level_list[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_level_list,
        2,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &detail_ptr->swap_chain,
        &detail_ptr->device,
        &feature_level,
        &detail_ptr->device_context
    );

    if (FAILED(result))
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            feature_level_list,
            2,
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &detail_ptr->swap_chain,
            &detail_ptr->device,
            &feature_level,
            &detail_ptr->device_context
        );
    }

    if (FAILED(result))
    {
        return false;
    }

    ID3D11Texture2D* back_buffer{ nullptr };
    detail_ptr->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));

    if (back_buffer)
    {
        detail_ptr->device->CreateRenderTargetView(back_buffer, nullptr, &detail_ptr->render_target_view);
        back_buffer->Release();
    }

    return detail_ptr->render_target_view != nullptr;
}

bool renderer::create_imgui()
{
    using namespace ImGui;
    CreateContext();
    StyleColorsDark();

    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    style.ScaleAllSizes(main_scale);
    io.FontGlobalScale = main_scale;

    io.Fonts->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
    io.Fonts->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;

    ImFontConfig font_config;
    font_config.OversampleH = 3;
    font_config.OversampleV = 3;
    font_config.PixelSnapH = true;
    font_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;
    font_config.FontDataOwnedByAtlas = false;

    var->font.icons[0] = io.Fonts->AddFontFromMemoryTTF(section_icons_hex, sizeof section_icons_hex, 15.f, &font_config, io.Fonts->GetGlyphRangesCyrillic());
    var->font.icons[1] = io.Fonts->AddFontFromMemoryTTF(icons_hex, sizeof icons_hex, 5.f, &font_config, io.Fonts->GetGlyphRangesCyrillic());
    var->font.visitor = io.Fonts->AddFontFromMemoryTTF(visitor_font, sizeof visitor_font, 9.33f, &font_config, io.Fonts->GetGlyphRangesCyrillic());
    var->font.tahoma = io.Fonts->AddFontFromMemoryTTF(tahoma_hex, sizeof tahoma_hex, 13.f, &font_config, io.Fonts->GetGlyphRangesCyrillic());

    if (!ImGui_ImplWin32_Init(detail_ptr->window))
    {
        return false;
    }

    if (!detail_ptr->device || !detail_ptr->device_context)
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(detail_ptr->device, detail_ptr->device_context))
    {
        return false;
    }

    return true;
}

void renderer::destroy_device()
{
    if (detail_ptr->render_target_view) detail_ptr->render_target_view->Release();
    if (detail_ptr->swap_chain) detail_ptr->swap_chain->Release();
    if (detail_ptr->device_context) detail_ptr->device_context->Release();
    if (detail_ptr->device) detail_ptr->device->Release();
}

void renderer::destroy_window()
{
    DestroyWindow(detail_ptr->window);
    UnregisterClassA(detail_ptr->window_class.lpszClassName, detail_ptr->window_class.hInstance);
}

void renderer::destroy_imgui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void renderer::start_render()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static float last_dpi_scaling = 1.0f;
    if (globals::settings::dpi_scaling != last_dpi_scaling)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = globals::settings::dpi_scaling;
        last_dpi_scaling = globals::settings::dpi_scaling;
    }

    static bool last_streamproof = false;
    if (globals::settings::streamproof != last_streamproof)
    {
        SetWindowDisplayAffinity(detail_ptr->window, globals::settings::streamproof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
        last_streamproof = globals::settings::streamproof;
    }

    static bool last_states[256]{};
    auto is_key_pressed = [&](int key) {
        if (key <= 0 || key >= 256) return false;
        bool is_down = driver->get_key_state(key);
        bool pressed = is_down && !last_states[key];
        last_states[key] = is_down;
        return pressed;
    };

    if (is_key_pressed(var->gui.menu_key))
    {
        var->gui.menu_opened = !var->gui.menu_opened;

        if (var->gui.menu_opened)
        {
            SetWindowLongPtr(detail_ptr->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED);
            SetForegroundWindow(detail_ptr->window);
        }
        else
        {
            SetWindowLongPtr(detail_ptr->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT);
        }

        SetWindowPos(detail_ptr->window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    static bool last_acrylic = false;
    static float last_intensity = 0.0f;
    static bool last_menu_opened = false;

    if (var->window.window_acrylic != last_acrylic || var->window.acrylic_intensity != last_intensity || var->gui.menu_opened != last_menu_opened)
    {
        set_blur_behind(detail_ptr->window, var->gui.menu_opened && var->window.window_acrylic, var->window.acrylic_intensity);
        last_acrylic = var->window.window_acrylic;
        last_intensity = var->window.acrylic_intensity;
        last_menu_opened = var->gui.menu_opened;
    }

    ImGui::GetIO().MouseDrawCursor = var->gui.menu_opened;

    render_watermark();
    render_keybind_list();
    notify->render();
}

void renderer::end_render()
{
    ImGui::Render();

    float clear_color[4]{ 0, 0, 0, 0 };
    detail_ptr->device_context->OMSetRenderTargets(1, &detail_ptr->render_target_view, nullptr);
    detail_ptr->device_context->ClearRenderTargetView(detail_ptr->render_target_view, clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (globals::settings::performance_def == 0)
    {
        detail_ptr->swap_chain->Present(0, 0);
    }
    else if (globals::settings::performance_def == 1)
    {
        detail_ptr->swap_chain->Present(1, 0);
    }

}

void renderer::render_watermark()
{
    std::string player_name = cache::cached_local_player.name;
    if (player_name.empty()) player_name = "LocalPlayer";

    std::string text1 = "^__^ | " + player_name;
    std::string text2 = "Atlanta.gg | " + std::to_string((int)ImGui::GetIO().Framerate) + " fps";

    ImFont* font = var->font.tahoma;
    if (!font) font = var->font.visitor;
    if (!font) return;

    float font_size = 13.0f;
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();

    ImVec2 padding = ImVec2(8, 2);
    ImVec2 size1 = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text1.c_str());
    ImVec2 box1_size = ImVec2(size1.x + padding.x * 2, size1.y + padding.y * 2);
    ImVec2 size2 = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text2.c_str());
    ImVec2 box2_size = ImVec2(size2.x + padding.x * 2, size2.y + padding.y * 2);
    static bool positions_initialized = false;
    if (!positions_initialized) {
        var->watermark_pos_one = ImVec2(ImGui::GetIO().DisplaySize.x - box1_size.x - 10, 10);
        var->watermark_pos_two = ImVec2(ImGui::GetIO().DisplaySize.x - box2_size.x - 10, 10 + box1_size.y + 10);
        positions_initialized = true;
    }
    if (var->gui.menu_opened)
    {
        static bool dragging1 = false;
        static ImVec2 drag_offset1;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mouse_pos = io.MousePos;

        bool hovering1 = mouse_pos.x >= var->watermark_pos_one.x && mouse_pos.x <= var->watermark_pos_one.x + box1_size.x &&
                         mouse_pos.y >= var->watermark_pos_one.y && mouse_pos.y <= var->watermark_pos_one.y + box1_size.y;

        if (hovering1 && ImGui::IsMouseClicked(0))
        {
            dragging1 = true;
            drag_offset1 = mouse_pos - var->watermark_pos_one;
        }

        if (dragging1 && ImGui::IsMouseDown(0))
        {
            var->watermark_pos_one = mouse_pos - drag_offset1;
        }
        else
        {
            dragging1 = false;
        }
    }
    if (var->gui.menu_opened)
    {
        static bool dragging2 = false;
        static ImVec2 drag_offset2;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mouse_pos = io.MousePos;

        bool hovering2 = mouse_pos.x >= var->watermark_pos_two.x && mouse_pos.x <= var->watermark_pos_two.x + box2_size.x &&
                         mouse_pos.y >= var->watermark_pos_two.y && mouse_pos.y <= var->watermark_pos_two.y + box2_size.y;

        if (hovering2 && ImGui::IsMouseClicked(0))
        {
            dragging2 = true;
            drag_offset2 = mouse_pos - var->watermark_pos_two;
        }

        if (dragging2 && ImGui::IsMouseDown(0))
        {
            var->watermark_pos_two = mouse_pos - drag_offset2;
        }
        else
        {
            dragging2 = false;
        }
    }
    draw->rect_filled(draw_list, var->watermark_pos_one, var->watermark_pos_one + box1_size, draw->get_clr(clr->window.background_one.Value));
    draw->rect_filled(draw_list, var->watermark_pos_one, ImVec2(var->watermark_pos_one.x + box1_size.x, var->watermark_pos_one.y + 2), draw->get_clr(clr->accent.Value));
    draw->text(draw_list, font, font_size, var->watermark_pos_one + padding, draw->get_clr(clr->widgets.text.Value), text1.c_str());
    draw->rect_filled(draw_list, var->watermark_pos_two, var->watermark_pos_two + box2_size, draw->get_clr(clr->window.background_one.Value));
    draw->rect_filled(draw_list, var->watermark_pos_two, ImVec2(var->watermark_pos_two.x + box2_size.x, var->watermark_pos_two.y + 2), draw->get_clr(clr->accent.Value));
    draw->text(draw_list, font, font_size, var->watermark_pos_two + padding, draw->get_clr(clr->widgets.text.Value), text2.c_str());
}


#include <features/cheats/misc/misc_utils.h>
#include <render/framework/settings/functions.h>
#include <render/framework/settings/variables.h>
#include <render/framework/settings/colors.h>

void renderer::render_keybind_list()
{
    if (!var->keybind_list_enabled)
        return;

    struct keybind_info_t {
        std::string name;
        std::string mode;
    };

    std::vector<keybind_info_t> active_keybinds;

    auto add_if_active = [&](const char* name, int key, int mode, bool enabled = true) {
        if (!enabled || key <= 0) return;
        if (cheats::is_key_active(key, mode, false)) {
            active_keybinds.push_back({ name, mode == 0 ? "[hold]" : "[toggle]" });
        }
    };

    add_if_active("Aimbot", globals::aimbot::keybind, globals::aimbot::keybind_mode, globals::aimbot::enabled);
    add_if_active("Triggerbot", globals::aimbot::trigger::keybind, globals::aimbot::trigger::keybind_mode, globals::aimbot::trigger::enabled);
    add_if_active("Target Strafe", 0, 0, globals::aimbot::target_strafe::enabled);
    add_if_active("Click TP", globals::misc::click_tp_key, globals::misc::click_tp_mode, globals::misc::click_tp);
    add_if_active("Speed Hack", globals::misc::speed_key, globals::misc::speed_mode, globals::misc::speed_hack);
    add_if_active("Fly Hack", globals::misc::fly_key, globals::misc::fly_mode, globals::misc::fly_hack);
    add_if_active("Free Camera", globals::misc::free_camera_key, globals::misc::free_camera_mode, globals::misc::free_camera);
    add_if_active("Ideal Peak", globals::misc::ideal_peak_key, globals::misc::ideal_peak_mode, globals::misc::ideal_peak);
    add_if_active("Jump Power", globals::misc::jump_power_key, globals::misc::jump_power_mode, globals::misc::jump_power_enabled);
    add_if_active("Hip Height", globals::misc::hip_height_key, globals::misc::hip_height_mode, globals::misc::hip_height_enabled);
    add_if_active("Gravity", globals::misc::gravity_key, globals::misc::gravity_mode, globals::misc::gravity_enabled);
    add_if_active("Jump Bug", globals::misc::jump_bug_key, globals::misc::jump_bug_mode, globals::misc::jump_bug);
    add_if_active("No Clip", globals::misc::no_clip_key, globals::misc::no_clip_mode, globals::misc::no_clip);
    add_if_active("Zoom", globals::misc::zoom_key, globals::misc::zoom_mode, globals::misc::zoom_enabled);
    add_if_active("Anti-Aim", globals::misc::angles_key, globals::misc::angles_mode, globals::misc::angles_enabled);
    add_if_active("Anchor", globals::misc::anchor_key, globals::misc::anchor_mode, globals::misc::anchor_on_bind);
    add_if_active("Waypoint", globals::misc::waypoint_key, globals::misc::waypoint_mode, globals::misc::waypoint_on_bind);
    add_if_active("Hit Sounds", globals::misc::hit_sounds_key, globals::misc::hit_sounds_mode, globals::misc::hit_sounds);
    add_if_active("Death Sounds", globals::misc::death_sounds_key, globals::misc::death_sounds_mode, globals::misc::death_sounds);
    add_if_active("Ping Spike", globals::misc::ping_spike_key, globals::misc::ping_spike_mode, globals::misc::ping_spike);
    add_if_active("Desync", globals::misc::desync_key, globals::misc::desync_mode, globals::misc::desync);
    add_if_active("No Shadows", globals::visuals::no_shadows_key, globals::visuals::no_shadows_mode, globals::visuals::no_shadows);
    add_if_active("Fullbright", globals::visuals::fullbright_key, globals::visuals::fullbright_mode, globals::visuals::fullbright);
    add_if_active("No Fog", globals::visuals::no_fog_key, globals::visuals::no_fog_mode, globals::visuals::no_fog);

    for (const auto& wp : globals::misc::waypoints) {
        if (wp.keybind > 0 && cheats::is_key_active(wp.keybind, 1, false)) {
            active_keybinds.push_back({ wp.name, "[toggle]" });
        }
    }



    ImFont* font = var->font.tahoma;
    if (!font) font = var->font.visitor;
    if (!font) return;

    float font_size = 13.0f;
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();

    float width = 150.0f;
    for (const auto& kb : active_keybinds) {
        float text_width = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, kb.name.c_str()).x + font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, kb.mode.c_str()).x + 20;
        if (text_width > width) width = text_width;
    }

    ImVec2 size = ImVec2(width, 20 + (active_keybinds.size() * 18));
    if (active_keybinds.empty()) size.y = 20;

    static bool position_initialized = false;
    if (!position_initialized) {
        var->keybind_list_pos = ImVec2(10, 300);
        position_initialized = true;
    }


        static bool dragging = false;
        static ImVec2 drag_offset;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mouse_pos = io.MousePos;

        bool hovering = mouse_pos.x >= var->keybind_list_pos.x && mouse_pos.x <= var->keybind_list_pos.x + size.x &&
                        mouse_pos.y >= var->keybind_list_pos.y && mouse_pos.y <= var->keybind_list_pos.y + 20;

        if (hovering && ImGui::IsMouseClicked(0)) {
            dragging = true;
            drag_offset = mouse_pos - var->keybind_list_pos;
        }

        if (dragging && ImGui::IsMouseDown(0)) {
            var->keybind_list_pos = mouse_pos - drag_offset;
        } else {
            dragging = false;
        }



    draw->rect_filled(draw_list, var->keybind_list_pos, var->keybind_list_pos + size, draw->get_clr(clr->window.background_one.Value));
    draw->rect_filled(draw_list, var->keybind_list_pos, ImVec2(var->keybind_list_pos.x + size.x, var->keybind_list_pos.y + 2), draw->get_clr(clr->accent.Value));


    ImVec2 title_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, "keybinds");
    draw->text(draw_list, font, font_size, var->keybind_list_pos + ImVec2((size.x - title_size.x) / 2, 4), draw->get_clr(clr->widgets.text.Value), "keybinds");


    float y_offset = 22.0f;
    for (const auto& kb : active_keybinds) {
        draw->text(draw_list, font, font_size, var->keybind_list_pos + ImVec2(5, y_offset), draw->get_clr(clr->widgets.text.Value), kb.name.c_str());
        float mode_width = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, kb.mode.c_str()).x;
        draw->text(draw_list, font, font_size, var->keybind_list_pos + ImVec2(size.x - mode_width - 5, y_offset), draw->get_clr(clr->widgets.text.Value), kb.mode.c_str());
        y_offset += 18.0f;
    }
}

void renderer::render_menu()
{
    gui->render();
}

void renderer::render_visuals()
{
	cheats::hook_visuals();
}

