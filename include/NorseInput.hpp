// NorseInput.hpp - one list of rebindable controls for the Norse overhaul (2026-11-17). Header-only.
//
// Each module's named actions and the controls bound to them live in ue4ss\Mods\NorseInput\bindings\*.ini (NorseManifest
// order; a later file's line for the same action replaces the earlier one):
//   [NorseRealtimeNative]                 ; the module that owns the actions
//   LightAttack = Pad.RB, Mouse.Left
//   PowerAttack = Pad.RT, Key.F
//   Dodge       = Pad.B:double            ; a gesture: tap (default), :hold (held >= 0.35 s), :double (two taps in 0.3 s)
// Controls: Pad.A B X Y LB RB LT RT L3 R3 Back Start Up Down Left Right; Key.<letter / digit / F1-F12 / Space Shift Ctrl
// Alt Tab Enter Esc>; Mouse.Left Right Middle X1 X2. Two actions of one module on the same control and gesture are a
// conflict and are reported (the later one is dropped). Modelled on Satisfactory Mod Loader's key bind registry,
// Dynamic Key Action Framework and Universal Hotkey Manager. A rebinding page comes with NMMF.
//
// Use: Bindings::Load(files, log) once; then each frame Reader::Poll(now, focused) for one module's actions -> a list of
// events (Pressed / Released / Hold / Double) and Reader::Down(action).
#pragma once

#include <Windows.h>
#include <Xinput.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace ni
{
namespace fs = std::filesystem;

enum class Device
{
    Pad,
    Key,
    Mouse
};
enum class Gesture
{
    Tap,
    Hold,
    Double
};

struct Control
{
    Device dev = Device::Key;
    int code = 0;    // XINPUT button bit / pseudo code for triggers; or a virtual-key code
    Gesture gesture = Gesture::Tap;
    std::string text; // as written, for the log
};

struct Action
{
    std::string owner, name;
    std::vector<Control> controls;
    std::string where;
};

enum class EventKind
{
    Pressed,  // a Tap control went down
    Released, // a Tap control went up
    Hold,     // a :hold control has been down 0.35 s
    Double    // a :double control was tapped twice within 0.3 s
};

struct Event
{
    std::string action;
    EventKind kind;
};

inline const char* KindName(EventKind k)
{
    switch (k)
    {
    case EventKind::Pressed: return "pressed";
    case EventKind::Released: return "released";
    case EventKind::Hold: return "hold";
    default: return "double";
    }
}

constexpr int kPadLT = 0x10000, kPadRT = 0x20000; // triggers are not button bits

inline std::string Lw(std::string s)
{
    for (auto& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

inline bool ParseControl(std::string t, Control& c, std::string& why)
{
    auto trim = [](std::string s) {
        while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) s.erase(0, 1);
        while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
        return s;
    };
    t = trim(t);
    c.text = t;
    size_t colon = t.find(':');
    if (colon != std::string::npos)
    {
        std::string g = Lw(trim(t.substr(colon + 1)));
        t = trim(t.substr(0, colon));
        if (g == "hold") c.gesture = Gesture::Hold;
        else if (g == "double") c.gesture = Gesture::Double;
        else if (g != "tap")
        {
            why = "unknown gesture '" + g + "' (tap, hold, double)";
            return false;
        }
    }
    size_t dot = t.find('.');
    if (dot == std::string::npos)
    {
        why = "a control is Pad.X, Key.X or Mouse.X";
        return false;
    }
    std::string dev = Lw(t.substr(0, dot)), k = Lw(t.substr(dot + 1));
    if (dev == "pad")
    {
        static const std::map<std::string, int> pad = {
            {"a", XINPUT_GAMEPAD_A}, {"b", XINPUT_GAMEPAD_B}, {"x", XINPUT_GAMEPAD_X}, {"y", XINPUT_GAMEPAD_Y},
            {"lb", XINPUT_GAMEPAD_LEFT_SHOULDER}, {"rb", XINPUT_GAMEPAD_RIGHT_SHOULDER}, {"lt", kPadLT}, {"rt", kPadRT},
            {"l3", XINPUT_GAMEPAD_LEFT_THUMB}, {"r3", XINPUT_GAMEPAD_RIGHT_THUMB}, {"back", XINPUT_GAMEPAD_BACK},
            {"start", XINPUT_GAMEPAD_START}, {"up", XINPUT_GAMEPAD_DPAD_UP}, {"down", XINPUT_GAMEPAD_DPAD_DOWN},
            {"left", XINPUT_GAMEPAD_DPAD_LEFT}, {"right", XINPUT_GAMEPAD_DPAD_RIGHT}};
        auto it = pad.find(k);
        if (it == pad.end())
        {
            why = "unknown pad control '" + k + "'";
            return false;
        }
        c.dev = Device::Pad;
        c.code = it->second;
        return true;
    }
    if (dev == "mouse")
    {
        static const std::map<std::string, int> m = {{"left", VK_LBUTTON}, {"right", VK_RBUTTON}, {"middle", VK_MBUTTON}, {"x1", VK_XBUTTON1}, {"x2", VK_XBUTTON2}};
        auto it = m.find(k);
        if (it == m.end())
        {
            why = "unknown mouse button '" + k + "'";
            return false;
        }
        c.dev = Device::Mouse;
        c.code = it->second;
        return true;
    }
    if (dev == "key")
    {
        static const std::map<std::string, int> named = {{"space", VK_SPACE}, {"shift", VK_SHIFT}, {"ctrl", VK_CONTROL}, {"alt", VK_MENU},
                                                         {"tab", VK_TAB}, {"enter", VK_RETURN}, {"esc", VK_ESCAPE}};
        c.dev = Device::Key;
        if (auto it = named.find(k); it != named.end()) c.code = it->second;
        else if (k.size() == 1 && isalnum(static_cast<unsigned char>(k[0]))) c.code = toupper(static_cast<unsigned char>(k[0]));
        else if (k.size() >= 2 && k[0] == 'f' && atoi(k.c_str() + 1) >= 1 && atoi(k.c_str() + 1) <= 12) c.code = VK_F1 + atoi(k.c_str() + 1) - 1;
        else
        {
            why = "unknown key '" + k + "'";
            return false;
        }
        return true;
    }
    why = "unknown device '" + dev + "' (Pad, Key, Mouse)";
    return false;
}

class Bindings
{
  public:
    using LogFn = std::function<void(const std::string&)>;

    void Load(const std::vector<fs::path>& files, LogFn log = nullptr)
    {
        m_actions.clear();
        m_index.clear();
        for (const auto& f : files) Read(f, log);
        Conflicts(log);
        if (log)
            for (auto& a : m_actions)
            {
                std::string s;
                for (auto& c : a.controls) s += (s.empty() ? "" : ", ") + c.text;
                log("binding " + a.owner + "." + a.name + " = " + s + " (" + a.where + ")");
            }
    }

    std::vector<const Action*> Of(const std::string& owner) const
    {
        std::vector<const Action*> out;
        for (auto& a : m_actions)
            if (Lw(a.owner) == Lw(owner)) out.push_back(&a);
        return out;
    }

    size_t Size() const { return m_actions.size(); }

  private:
    std::vector<Action> m_actions;          // in load order: a conflict is lost by the later one
    std::map<std::string, size_t> m_index;  // "owner.action" lower-case -> position

    void Read(const fs::path& f, const LogFn& log)
    {
        std::ifstream in(f, std::ios::binary);
        std::string line, owner, file = f.filename().string();
        int n = 0;
        while (std::getline(in, line))
        {
            ++n;
            if (n == 1 && line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            for (size_t i = 0; i < line.size(); ++i)
                if ((line[i] == ';' || line[i] == '#') && (i == 0 || isspace(static_cast<unsigned char>(line[i - 1]))))
                {
                    line.resize(i);
                    break;
                }
            while (!line.empty() && isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
            while (!line.empty() && isspace(static_cast<unsigned char>(line.front()))) line.erase(0, 1);
            if (line.empty()) continue;
            std::string where = file + ":" + std::to_string(n);
            if (line.front() == '[' && line.back() == ']')
            {
                owner = line.substr(1, line.size() - 2);
                continue;
            }
            size_t eq = line.find('=');
            if (owner.empty() || eq == std::string::npos)
            {
                if (log) log(where + ": not a [Module] or Action = controls line - skipped");
                continue;
            }
            Action a;
            a.owner = owner;
            a.name = line.substr(0, eq);
            while (!a.name.empty() && isspace(static_cast<unsigned char>(a.name.back()))) a.name.pop_back();
            a.where = where;
            std::stringstream ss(line.substr(eq + 1));
            std::string part;
            while (std::getline(ss, part, ','))
            {
                Control c;
                std::string why;
                if (ParseControl(part, c, why)) a.controls.push_back(c);
                else if (log && part.find_first_not_of(" \t") != std::string::npos) log(where + ": '" + part + "' - " + why + " - ignored");
            }
            std::string key = Lw(owner + "." + a.name);
            if (auto it = m_index.find(key); it != m_index.end())
            {
                if (log) log(where + ": " + owner + "." + a.name + " replaces " + m_actions[it->second].where);
                m_actions[it->second] = a; // keeps its place in the order
            }
            else
            {
                m_index[key] = m_actions.size();
                m_actions.push_back(a);
            }
        }
    }

    // two actions of one module on the same control and gesture: the later file / line loses that control
    void Conflicts(const LogFn& log)
    {
        std::map<std::string, std::string> taken; // owner|dev|code|gesture -> action
        for (auto& a : m_actions)
        {
            std::vector<Control> keep;
            for (const Control& c : a.controls)
            {
                std::string slot = Lw(a.owner) + "|" + std::to_string(static_cast<int>(c.dev)) + "|" + std::to_string(c.code) + "|" +
                                   std::to_string(static_cast<int>(c.gesture));
                if (auto it = taken.find(slot); it != taken.end())
                {
                    if (log) log("CONFLICT: " + a.owner + "." + a.name + " and " + it->second + " both on " + c.text + " - " + a.name + " loses it (" + a.where + ")");
                    continue;
                }
                taken[slot] = a.owner + "." + a.name;
                keep.push_back(c);
            }
            a.controls = keep;
        }
    }
};

// The current state of the devices: what a Reader looks at each frame.
struct DeviceState
{
    WORD pad = 0;
    BYTE lt = 0, rt = 0;
    bool padOk = false;
    std::function<bool(int vk)> keyDown; // keyboard and mouse
};

class Reader
{
  public:
    double holdTime = 0.35, doubleGap = 0.30;

    Reader(const Bindings& b, const std::string& owner) { Rebind(b, owner); }

    void Rebind(const Bindings& b, const std::string& owner)
    {
        m_actions = b.Of(owner);
        m_state.assign(m_actions.size(), {});
        for (size_t i = 0; i < m_actions.size(); ++i) m_state[i].assign(m_actions[i]->controls.size(), {});
    }

    // One frame: the events since the last frame. focused = the game window is in front (keyboard and mouse only count then).
    std::vector<Event> Poll(double now, bool focused, const DeviceState& d)
    {
        std::vector<Event> out;
        for (size_t i = 0; i < m_actions.size(); ++i)
            for (size_t j = 0; j < m_actions[i]->controls.size(); ++j)
            {
                const Control& c = m_actions[i]->controls[j];
                CState& s = m_state[i][j];
                bool down = IsDown(c, focused, d, s.down);
                const std::string& name = m_actions[i]->name;
                if (down && !s.down)
                {
                    s.since = now;
                    s.holdFired = false;
                    if (c.gesture == Gesture::Tap) out.push_back({name, EventKind::Pressed});
                    if (c.gesture == Gesture::Double)
                    {
                        if (now - s.lastTap <= doubleGap)
                        {
                            out.push_back({name, EventKind::Double});
                            s.lastTap = -1e9;
                        }
                        else s.lastTap = now;
                    }
                }
                if (!down && s.down && c.gesture == Gesture::Tap) out.push_back({name, EventKind::Released});
                if (down && c.gesture == Gesture::Hold && !s.holdFired && now - s.since >= holdTime)
                {
                    s.holdFired = true;
                    out.push_back({name, EventKind::Hold});
                }
                s.down = down;
            }
        return out;
    }

    bool Down(const std::string& action) const
    {
        for (size_t i = 0; i < m_actions.size(); ++i)
            if (m_actions[i]->name == action)
                for (const CState& s : m_state[i])
                    if (s.down) return true;
        return false;
    }

    size_t Size() const { return m_actions.size(); }

    // the devices as they are now: the first connected pad, and GetAsyncKeyState
    static DeviceState Sample(DWORD(WINAPI* xinput)(DWORD, XINPUT_STATE*))
    {
        DeviceState d;
        if (xinput)
            for (DWORD u = 0; u < 4; ++u)
            {
                XINPUT_STATE st{};
                if (xinput(u, &st) != ERROR_SUCCESS) continue;
                d.pad = st.Gamepad.wButtons;
                d.lt = st.Gamepad.bLeftTrigger;
                d.rt = st.Gamepad.bRightTrigger;
                d.padOk = true;
                break;
            }
        d.keyDown = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
        return d;
    }

  private:
    struct CState
    {
        bool down = false, holdFired = false;
        double since = 0, lastTap = -1e9;
    };
    std::vector<const Action*> m_actions;
    std::vector<std::vector<CState>> m_state;

    static bool IsDown(const Control& c, bool focused, const DeviceState& d, bool wasDown)
    {
        switch (c.dev)
        {
        case Device::Pad:
            if (!d.padOk) return false;
            if (c.code == kPadLT) return d.lt > (wasDown ? 60 : 100); // hysteresis: a held trigger does not chatter
            if (c.code == kPadRT) return d.rt > (wasDown ? 60 : 100);
            return (d.pad & c.code) != 0;
        default: return focused && d.keyDown && d.keyDown(c.code);
        }
    }
};
} // namespace ni
