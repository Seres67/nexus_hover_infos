#include "addon/Addon.hpp"

#include <ranges>
#include <thread>
#include <addon/Utils.hpp>

#include "imgui/imgui.h"

#include <Windows.h>
#include <winuser.h>

#include <fstream>

#include <nlohmann/json.hpp>
#include "cpr/cpr.h"


std::filesystem::path Addon::AddonPath;
std::filesystem::path Addon::SettingsPath;

HWND m_window = nullptr;

nlohmann::json item_names;

bool show_popup = false;

std::string item_name;
long long buy;
long long sell;
long long supply;
long long demand;

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

    // open item names json file
    std::ifstream ifs(AddonPath / "items-names.json");
    if (!ifs.is_open())
    {
        APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "Could not open item names json file");
        return;
    }
    item_names = nlohmann::json::parse(ifs);
    ifs.close();
    APIDefs->Log(ELogLevel_INFO, "Hover Infos", "Parsed item names");
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
    if (show_popup)
    {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().MousePos.x + 60, ImGui::GetIO().MousePos.y));
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (ImGui::Begin("Hover Infos", nullptr, window_flags))
        {
            ImGui::Text("Item: %s", item_name.c_str());
            ImGui::Text("Buy: %lld, Sell: %lld, Supply: %lld, Demand: %lld", buy, sell, supply, demand);
            ImGui::End();
        }
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
    // char log[1024] = {};
    // sprintf_s(log, "message: %d, WParam: %d, LParam: %d", message, wParam, lParam);
    // APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", log);
    switch (message)
    {
    case WM_LBUTTONDOWN:
        show_popup = false;
        break;
    case WM_LBUTTONUP:
        show_popup = false;
        break;
    }
    return message;
}

void Addon::get_item_infos()
{
    if (m_window == nullptr)
        return;
    APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "Getting item infos");
    std::thread([]()
    {
        // Press keys and wait for chat to open
        INPUT inputs[6] = {};
        ZeroMemory(inputs, sizeof(inputs));

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_MENU;
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 'D';
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = VK_SHIFT;

        inputs[3].type = INPUT_MOUSE;
        inputs[3].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        if (uSent != ARRAYSIZE(inputs))
        {
            APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "SendInput failed");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        // Release keys
        INPUT release_inputs[2] = {};
        ZeroMemory(release_inputs, sizeof(release_inputs));

        release_inputs[0].type = INPUT_MOUSE;
        release_inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        release_inputs[1].type = INPUT_KEYBOARD;
        release_inputs[1].ki.wVk = VK_SHIFT;
        release_inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

        uSent = SendInput(ARRAYSIZE(release_inputs), release_inputs, sizeof(INPUT));
        if (uSent != ARRAYSIZE(release_inputs))
        {
            APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "SendInput failed");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        INPUT select_text[1] = {};
        ZeroMemory(select_text, sizeof(select_text));

        select_text[0].type = INPUT_KEYBOARD;
        select_text[0].ki.wVk = VK_CONTROL;

        uSent = SendInput(ARRAYSIZE(select_text), select_text, sizeof(INPUT));
        if (uSent != ARRAYSIZE(select_text))
        {
            APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "SendInput failed");
        }
        SendMessage(m_window, WM_KEYDOWN, 'A', Utils::GetLParam('A', true));
        SendMessage(m_window, WM_KEYUP, 'A', Utils::GetLParam('A', false));
        SendMessage(m_window, WM_KEYDOWN, 'C', Utils::GetLParam('C', true));
        SendMessage(m_window, WM_KEYUP, 'C', Utils::GetLParam('C', false));
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        ZeroMemory(select_text, sizeof(select_text));

        select_text[0].type = INPUT_KEYBOARD;
        select_text[0].ki.wVk = VK_CONTROL;
        select_text[0].ki.dwFlags = KEYEVENTF_KEYUP;

        uSent = SendInput(ARRAYSIZE(select_text), select_text, sizeof(INPUT));
        if (uSent != ARRAYSIZE(select_text))
        {
            APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "SendInput failed");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        // open and read windows clipboard
        OpenClipboard(m_window);
        HGLOBAL hClipboardData = GetClipboardData(CF_TEXT);
        std::string clipboardData = (char*)GlobalLock(hClipboardData);
        GlobalUnlock(hClipboardData);
        CloseClipboard();

        auto out = clipboardData | std::ranges::views::filter([](auto& c)
        {
            return std::isalpha(c) || c == ' ';
        });
        std::string outString = std::string(out.begin(), out.end());
        if (outString[0] == ' ')
            outString.erase(0, 1);
        item_name = outString;
        //TODO: handle when item name is plural
        APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", outString.c_str());
        long long int id = -1;
        for (auto& item : item_names["items"])
        {
            if (outString == item[1].get<std::string>())
            {
                id = item[0].get<long long int>();
                APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", "found item id");
                break;
            }
        }
        if (id == -1)
        {
            show_popup = false;
            return;
        }
        cpr::Response r = cpr::Get(cpr::Url{"http://api.gw2tp.com/1/items?ids=" + std::to_string(id) + "&fields=buy,sell,supply,demand"});
        nlohmann::json j = nlohmann::json::parse(r.text);
        buy = j["results"][0][1].get<long long>();
        sell = j["results"][0][2].get<long long>();
        supply = j["results"][0][3].get<long long>();
        demand = j["results"][0][0].get<long long>();
        show_popup = true;
        char log[1024] = {};
        sprintf_s(log, "Buy: %lld, Sell: %lld, Supply: %lld, Demand: %lld", buy, sell, supply, demand);
        APIDefs->Log(ELogLevel_DEBUG, "Hover Infos", log);
    }).detach();
}
