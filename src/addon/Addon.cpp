#include "addon/Addon.hpp"

#include <addon/Utils.hpp>

#include "imgui/imgui.h"

std::filesystem::path Addon::AddonPath;
std::filesystem::path Addon::SettingsPath;

HWND m_window = nullptr;

void Addon::load(AddonAPI* aApi)
{
    APIDefs = aApi;
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(APIDefs->ImguiContext));
    ImGui::SetAllocatorFunctions(
        (void *(*)(size_t, void*))APIDefs->ImguiMalloc,
        (void (*)(void*, void*))APIDefs->ImguiFree); // on imgui 1.80+

    MumbleLink = (Mumble::Data*)APIDefs->GetResource("DL_MUMBLE_LINK");
    NexusLink = (NexusLinkData*)APIDefs->GetResource("DL_NEXUS_LINK");

    APIDefs->RegisterRender(ERenderType_Render, Addon::render);
    APIDefs->RegisterRender(ERenderType_OptionsRender, Addon::render_options);

    APIDefs->RegisterWndProc(Addon::wndproc);

    APIDefs->RegisterKeybindWithString("KB_HOVER_INFOS", Addon::process_keybindings, "C");

    AddonPath = APIDefs->GetAddonDirectory(m_addon_name.data());
    SettingsPath = APIDefs->GetAddonDirectory((std::string(m_addon_name.data()) + "/settings.json").c_str());
    std::filesystem::create_directory(AddonPath);
}

void Addon::unload()
{
    APIDefs->DeregisterRender(Addon::render);
    APIDefs->DeregisterRender(Addon::render_options);
    APIDefs->DeregisterKeybind("KB_HOVER_INFOS");
    MumbleLink = nullptr;
    NexusLink = nullptr;
}

void Addon::render()
{
    ImGui::PushFont(static_cast<ImFont*>(NexusLink->Font));
    if (ImGui::Begin("Hover Infos Debug", nullptr, 0))
    {
        ImGui::Text("Hello from %s", m_addon_name.data());
        ImGui::Text("Mouse position: %f, %f", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
        ImGui::End();
    }
    ImGui::PopFont();
}

void Addon::render_options()
{
    ImGui::TextDisabled("Widget");
}

void Addon::process_keybindings(const char* aIdentifier)
{
    std::string identifier = aIdentifier;
    if (identifier == "KB_HOVER_INFOS")
    {
        get_item_infos();
    }
}

unsigned int Addon::wndproc(HWND hWnd, unsigned int message, unsigned __int64 wParam, LPARAM lParam)
{
    if (!m_window)
        m_window = hWnd;
    return message;
}

void Addon::get_item_infos()
{
    if (m_window == nullptr)
        return;
    APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "Getting item infos");
    auto pos = MAKELPARAM((short)ImGui::GetIO().MousePos.x, (short)ImGui::GetIO().MousePos.y);

    // PostMessage(m_window, WM_KEYDOWN, VK_SHIFT, Utils::GetLParam(VK_SHIFT, true));
    // PostMessage(m_window, WM_LBUTTONDOWN, MK_SHIFT | MK_LBUTTON, pos);
    // PostMessage(m_window, WM_LBUTTONUP, MK_SHIFT, pos);
    // PostMessage(m_window, WM_KEYUP, VK_SHIFT, Utils::GetLParam(VK_SHIFT, false));

    INPUT inputs[4] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SHIFT;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].mi.dwExtraInfo = GetMessageExtraInfo();

    inputs[2].type = INPUT_MOUSE;
    inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    inputs[2].mi.dwExtraInfo = GetMessageExtraInfo();

    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_SHIFT;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    if (uSent != ARRAYSIZE(inputs))
    {
        APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "SendInput failed");
    }
}
