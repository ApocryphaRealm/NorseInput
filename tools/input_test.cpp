// input_test.cpp - offline test of NorseInput.hpp with simulated devices.
#include "NorseInput.hpp"

#include <cstdio>

static int g_pass = 0, g_fail = 0;
static void Check(bool ok, const char* what)
{
    if (ok) ++g_pass;
    else
    {
        ++g_fail;
        std::printf("FAIL: %s\n", what);
    }
}

int main(int argc, char** argv)
{
    namespace fs = std::filesystem;
    fs::path def = argc > 1 ? fs::path(argv[1]) : fs::path("package/bindings/_default.ini");
    fs::path tmp = fs::temp_directory_path() / "norse-input-test";
    fs::create_directories(tmp);
    std::ofstream(tmp / "z-test.ini") << "[NorseRealtimeNative]\nTestHold = Key.J:hold\nTestDouble = Key.K:double\nTestTap = Key.L, Pad.LT\n"
                                         "Clash = Pad.RB\nBad = Key.Nope, Pod.A, Pad.A:wiggle\n";
    ni::Bindings b;
    std::vector<std::string> log;
    b.Load({def, tmp / "z-test.ini"}, [&](const std::string& s) {
        log.push_back(s);
        std::printf("%s\n", s.c_str());
    });
    auto logged = [&](const char* t) { return std::any_of(log.begin(), log.end(), [&](const std::string& s) { return s.find(t) != std::string::npos; }); };
    Check(logged("CONFLICT") && logged("Pad.RB"), "two actions on Pad.RB: a conflict is reported");
    Check(logged("unknown key 'nope'") && logged("unknown device 'pod'") && logged("unknown gesture 'wiggle'"), "bad controls are reported");

    ni::Reader r(b, "NorseRealtimeNative");
    std::set<int> keys;
    WORD pad = 0;
    BYTE rt = 0;
    auto state = [&]() {
        ni::DeviceState d;
        d.pad = pad;
        d.rt = rt;
        d.padOk = true;
        d.keyDown = [&](int vk) { return keys.count(vk) > 0; };
        return d;
    };
    double t = 0;
    auto step = [&](double dt, bool focused = true) {
        t += dt;
        return r.Poll(t, focused, state());
    };
    auto has = [](const std::vector<ni::Event>& ev, const char* a, ni::EventKind k) {
        return std::any_of(ev.begin(), ev.end(), [&](const ni::Event& e) { return e.action == a && e.kind == k; });
    };

    pad = XINPUT_GAMEPAD_RIGHT_SHOULDER;
    auto e = step(0.016);
    Check(has(e, "LightAttack", ni::EventKind::Pressed), "RB: LightAttack pressed");
    Check(!has(e, "Clash", ni::EventKind::Pressed), "the losing action of a conflict does not fire");
    Check(step(0.016).empty() && r.Down("LightAttack"), "held: no repeat, Down is true");
    pad = 0;
    Check(has(step(0.016), "LightAttack", ni::EventKind::Released), "RB up: released");
    rt = 120;
    Check(has(step(0.016), "PowerAttack", ni::EventKind::Pressed), "RT past the threshold: PowerAttack");
    rt = 80;
    Check(step(0.016).empty(), "RT easing to 80 stays down (hysteresis)");
    rt = 0;
    step(0.016);
    keys = {'F'};
    Check(has(step(0.016), "PowerAttack", ni::EventKind::Pressed), "Key.F also fires PowerAttack");
    keys.clear();
    step(0.016);
    keys = {'F'};
    Check(!has(step(0.016, false), "PowerAttack", ni::EventKind::Pressed), "keyboard ignored while the game is not in front");
    keys.clear();
    step(0.016);
    keys = {'J'};
    Check(step(0.016).empty(), "J seen down: no hold yet");
    Check(step(0.2).empty(), "J down 0.2 s: no hold yet");
    Check(has(step(0.2), "TestHold", ni::EventKind::Hold), "J down 0.4 s: hold");
    Check(step(0.3).empty(), "hold fires once");
    keys.clear();
    step(0.016);
    keys = {'K'};
    Check(step(0.016).empty(), "first K tap: nothing yet");
    keys.clear();
    step(0.1);
    keys = {'K'};
    Check(has(step(0.1), "TestDouble", ni::EventKind::Double), "second K tap within 0.3 s: double");
    keys.clear();
    step(1.0);
    keys = {'K'};
    step(0.016);
    keys.clear();
    step(0.5);
    keys = {'K'};
    Check(!has(step(0.016), "TestDouble", ni::EventKind::Double), "taps 0.5 s apart: no double");
    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    fs::remove_all(tmp);
    return g_fail ? 1 : 0;
}
