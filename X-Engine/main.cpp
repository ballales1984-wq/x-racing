#include "core/logger.h"
#include "core/clock.h"
#include "core/application.h"
#include "core/camera.h"
#include "core/fly_camera.h"
#include "rendering/dx12/dx12_renderer.h"
#include "rendering/scene.h"
#include "rendering/scene_loader.h"
#include "rendering/light.h"
#include "platform/win32/win32_window.h"
#include "platform/win32/win32_input.h"
#include "platform/win32/win32_mouse.h"
#include "platform/win32/win32_hud.h"
#include "debug/console.h"
#include "physics/physics_world.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace {

class SceneApp : public xe::Application {
public:
    using Application::Application;

    void SetMouse(xe::Mouse* mouse) { mouse_ = mouse; }
    void SetHud(xe::HudOverlay* hud) { hud_ = hud; }
    void SetConsole(xe::Console* c) { console_ = c; }
    void SetPhysics(xe::PhysicsWorld* p) { physics_ = p; }

protected:
    void OnUpdate(float dt) override {
        const float t = GetClock().GetTotalTime();
        xe::Input& inp = GetInput();

        if (inp.IsKeyPressed(xe::Key::Backquote)) {
            console_->Toggle();
            if (console_->IsOpen()) {
                cursor_captured_ = false;
                GetWindow().SetCursorCapture(false);
            }
        }
        if (inp.IsKeyPressed(xe::Key::Escape)) {
            if (console_->IsOpen()) {
                console_->SetOpen(false);
            } else {
                cursor_captured_ = !cursor_captured_;
                GetWindow().SetCursorCapture(cursor_captured_);
            }
        }

        if (console_->IsOpen()) {
            if (inp.IsKeyPressed(xe::Key::Enter)) {
                console_->SubmitInput();
            } else if (inp.IsKeyPressed(xe::Key::Backspace)) {
                console_->Backspace();
            } else if (inp.IsKeyPressed(xe::Key::Up)) {
                console_->HistoryPrev();
            } else if (inp.IsKeyPressed(xe::Key::Down)) {
                console_->HistoryNext();
            }
        } else {
            if (!cursor_captured_ && mouse_ && mouse_->GetState().WasPressed(xe::MouseButton::Left)) {
                cursor_captured_ = true;
                GetWindow().SetCursorCapture(true);
            }

            const auto& ms = mouse_ ? mouse_->GetState() : xe::MouseState{};
            controller_.Update(
                dt,
                inp.IsKeyDown(xe::Key::W) || inp.IsKeyDown(xe::Key::Up),
                inp.IsKeyDown(xe::Key::S) || inp.IsKeyDown(xe::Key::Down),
                inp.IsKeyDown(xe::Key::A) || inp.IsKeyDown(xe::Key::Left),
                inp.IsKeyDown(xe::Key::D) || inp.IsKeyDown(xe::Key::Right),
                inp.IsKeyDown(xe::Key::Space),
                inp.IsKeyDown(xe::Key::LeftCtrl) || inp.IsKeyDown(xe::Key::RightCtrl),
                cursor_captured_ ? ms.dx : 0,
                cursor_captured_ ? ms.dy : 0);

            float px, py, pz;
            controller_.GetPosition(px, py, pz);
            scene.camera_position = { px, py, pz };
            auto fwd = controller_.GetForward();
            scene.camera_target = { px + fwd.x * 5.0f, py + fwd.y * 5.0f, pz + fwd.z * 5.0f };
            scene.camera_up = { 0.0f, 1.0f, 0.0f };

            if (scene.objects.size() >= 3) {
                if (physics_ && physics_->IsEnabled() && physics_->Size() >= 3) {
                    physics_->Step(dt);
                    for (int i = 0; i < 3 && i < static_cast<int>(scene.objects.size()); ++i) {
                        const auto& body = physics_->Get(i);
                        scene.objects[i].instance.position = body.position;
                        scene.objects[i].instance.rotation_rad = body.orientation.ToEulerXYZ();
                    }
                } else {
                    for (int i = 0; i < 3 && i < static_cast<int>(scene.objects.size()); ++i) {
                        const float a = t * 0.7f + i * 2.094f;
                        scene.objects[i].instance.position = {
                            1.6f * std::cos(a),
                            std::sin(t * 1.3f + i) * 0.4f,
                            1.6f * std::sin(a),
                        };
                        scene.objects[i].instance.rotation_rad = { t * 0.5f + i, t * 0.7f, 0.0f };
                    }
                }
            }

            const float sa = t * 0.3f;
            sun.direction = { std::cos(sa) * 0.5f, 1.0f, std::sin(sa) * 0.5f };
            sun.color = { 1.0f, 0.95f, 0.85f };
            sun.intensity = 1.2f + 0.3f * std::sin(t * 0.5f);

            if (auto* r = dynamic_cast<xe::DX12Renderer*>(GetRenderer())) {
                r->SetScene(&scene);
                r->SetLight(&sun);
                r->SetCameraPosition(px, py, pz);
                r->SetClearColor(0.04f, 0.06f, 0.10f, 1.0f);
            }

            for (auto& obj : scene.objects) {
                if (obj.name == "sprite") {
                    const float dx = scene.camera_position.x - obj.instance.position.x;
                    const float dz = scene.camera_position.z - obj.instance.position.z;
                    obj.instance.rotation_rad.y = std::atan2(dx, dz);
                }
            }

            if (hud_) {
                hud_->BeginDraw();
                std::wostringstream ss;
                ss << L"X-Engine V0.11  |  FPS: " << static_cast<int>(1.0f / std::max(dt, 1e-6f))
                   << L"  |  Objs: " << scene.objects.size() << L"  |  t=" << t;
                hud_->DrawText(10, 10, ss.str(), RGB(255, 255, 255));
                ss.str(L"");
                ss << L"Cam: (" << px << "," << py << "," << pz << ")"
                   << L" yaw=" << controller_.GetYaw()
                   << L" pitch=" << controller_.GetPitch();
                hud_->DrawText(10, 30, ss.str(), RGB(180, 240, 200));
                ss.str(L"");
                ss << L"Cursor: " << (cursor_captured_ ? L"CAPTURED" : L"FREE  ")
                   << L"   `=console  ESC=toggle";
                hud_->DrawText(10, 50, ss.str(),
                               cursor_captured_ ? RGB(255, 220, 120) : RGB(180, 180, 200));
                hud_->EndDraw();
            }
        }

        if (hud_ && console_ && console_->IsOpen()) {
            int w, h;
            GetWindow().GetSize(w, h);
            hud_->DrawConsole(*console_, w, h);
        }
    }

private:
    xe::Scene scene = []{
        xe::Scene s;
        xe::SceneObject o1; o1.name = "cube_a"; o1.instance.position = { 1.0f, 0.0f, 0.0f };
        s.objects.push_back(o1);
        xe::SceneObject o2; o2.name = "cube_b"; o2.instance.position = { -1.0f, 0.0f, 0.0f };
        s.objects.push_back(o2);
        xe::SceneObject o3; o3.name = "cube_c"; o3.instance.position = { 0.0f, 0.0f, 1.0f };
        s.objects.push_back(o3);
        xe::SceneObject o4; o4.name = "ground";
        o4.instance.position = { 0.0f, -1.0f, 0.0f };
        o4.instance.scale = { 4.0f, 0.1f, 4.0f };
        o4.instance.tint = { 0.6f, 0.6f, 0.65f, 1.0f };
        s.objects.push_back(o4);
        xe::SceneObject tex; tex.name = "sprite";
        tex.instance.mesh = xe::MeshKind::Quad;
        tex.instance.position = { 0.0f, 1.6f, 0.0f };
        tex.instance.scale = { 1.5f, 1.0f, 1.0f };
        tex.instance.texture_path = "data/checker.png";
        s.objects.push_back(tex);
        return s;
    }();

    xe::FlyCameraController controller_;
    xe::DirectionalLight sun{};
    xe::Mouse* mouse_ = nullptr;
    xe::HudOverlay* hud_ = nullptr;
    xe::Console* console_ = nullptr;
    xe::PhysicsWorld* physics_ = nullptr;
    bool cursor_captured_ = true;
};

}  // namespace

int main() {
    xe::Logger::Init();
    XE_LOG_INFO("X-Engine V0.11");

    auto window   = std::make_unique<xe::Win32Window>();
    xe::Win32Window* raw_window = window.get();
    auto input    = std::make_unique<xe::Win32Input>();
    auto mouse    = std::make_unique<xe::Win32Mouse>();
    auto renderer = std::make_unique<xe::DX12Renderer>();

    SceneApp app(std::move(window), std::move(input), std::move(renderer));

    if (!app.Create("X-Engine V0.11 — Fly Camera + Console + Physics", 1280, 720)) {
        XE_LOG_ERROR("Failed to initialize engine");
        xe::Logger::Shutdown();
        return -1;
    }

    HWND hwnd = reinterpret_cast<HWND>(app.GetWindow().GetNativeHandle());
    xe::HudOverlay hud;
    hud.Initialize(hwnd);

    xe::Console console;

    // Register default commands
    console.Register("fps",  "Print current FPS", [&app, &console](auto&) {
        std::ostringstream o; o << "FPS: " << app.GetClock().GetFPS();
        console.PrintLn(o.str());
    });
    console.Register("clear", "Clear console output", [&console](auto&) {
        console.ClearOutput();
    });
    console.Register("objects", "Print scene object count", [&app, &console](auto&) {
        auto* r = dynamic_cast<xe::DX12Renderer*>(app.GetRenderer());
        if (r) {
            std::ostringstream o; o << "Scene stats: draw_calls=" << 0;
            console.PrintLn(o.str());
        }
    });
    console.Register("quit", "Exit the application", [&app](auto&) {
        app.Shutdown();
    });

    // Physics world — 3 OBB bodies matching the orbiting cubes.
    xe::PhysicsWorld physics;
    for (int i = 0; i < 3; ++i) {
        xe::RigidBody b;
        b.shape = xe::ShapeKind::Box;
        b.position = { 1.6f * std::cos(i * 2.094f), 0.0f, 1.6f * std::sin(i * 2.094f) };
        b.halfExtents = { 0.5f, 0.5f, 0.5f };  // 1m cube
        b.mass   = 1.0f;
        b.orientation = xe::Quat::FromAxisAngle({0,1,0}, i * 0.4f);
        physics.Add(b);
    }
    // Static ground plane approximated as a big static box.
    xe::RigidBody ground;
    ground.shape = xe::ShapeKind::Box;
    ground.position = { 0, -2.0f, 0 };
    ground.halfExtents = { 10, 0.25f, 10 };
    ground.dynamic = false;
    physics.Add(ground);
    physics.SetEnabled(false);  // off by default; toggle via console
    physics.CaptureInitialState();

    console.Register("physics", "Enable/disable physics simulation (on|off|toggle)",
                     [&physics, &console](const auto& args) {
        if (args.size() < 2) {
            console.PrintLn("Usage: physics on|off|toggle");
            return;
        }
        std::string a = args[1];
        bool cur = physics.IsEnabled();
        if (a == "on")      physics.SetEnabled(true);
        else if (a == "off") physics.SetEnabled(false);
        else if (a == "toggle") physics.SetEnabled(!cur);
        else { console.PrintLn("[err] expected on|off|toggle"); return; }
        std::ostringstream o; o << "Physics: " << (physics.IsEnabled() ? "ON" : "OFF")
                                << "  bodies=" << physics.Size();
        console.PrintLn(o.str());
    });
    console.Register("kick", "Kick body #N (0..N-1) with impulse (x y z)",
                     [&physics, &console](const auto& args) {
        if (args.size() < 5) {
            console.PrintLn("Usage: kick <body_idx> <x> <y> <z>");
            return;
        }
        int idx = std::stoi(args[1]);
        float x = std::stof(args[2]);
        float y = std::stof(args[3]);
        float z = std::stof(args[4]);
        physics.Kick(idx, x, y, z);
        std::ostringstream o; o << "Kicked body " << idx << " impulse=("
                                << x << "," << y << "," << z << ")";
        console.PrintLn(o.str());
    });
    console.Register("collisions", "Print last step collision count",
                     [&physics, &console](auto&) {
        std::ostringstream o; o << "Last collisions: " << physics.LastCollisionCount();
        console.PrintLn(o.str());
    });
    console.Register("reset", "Reset all bodies to their initial state",
                     [&physics, &console](auto&) {
        physics.ResetAll();
        console.PrintLn("Physics: bodies reset to initial state.");
    });
    console.Register("spin", "Set angular velocity of body N (ax ay az rad/s)",
                     [&physics, &console](const auto& args) {
        if (args.size() < 5) {
            console.PrintLn("Usage: spin <body_idx> <ax> <ay> <az>");
            return;
        }
        int idx = std::stoi(args[1]);
        float ax = std::stof(args[2]);
        float ay = std::stof(args[3]);
        float az = std::stof(args[4]);
        physics.Spin(idx, { ax, ay, az });
        std::ostringstream o; o << "Body " << idx << " angVel=("
                                << ax << "," << ay << "," << az << ") rad/s";
        console.PrintLn(o.str());
    });

    app.SetMouse(mouse.get());
    app.SetHud(&hud);
    app.SetConsole(&console);
    app.SetPhysics(&physics);

    // Register help with full closure now that we have context
    console.Register("help", "List all available commands", [&console](auto&) {
        console.PrintLn("Available commands:");
        for (const auto& c : console.Commands()) {
            console.PrintLn("  " + c.name + "  -  " + c.help);
        }
    });

    // Wire ascii char handler
    if (raw_window) {
        raw_window->SetCharCallback([&console](char c) {
            if (console.IsOpen()) console.AppendChar(c);
        });
    }

    console.SetOpen(true);
    console.PrintLn("X-Engine V0.11 console.  'help' for commands, '`' or ESC to close.");

    app.Run();
    app.Shutdown();

    XE_LOG_INFO("Done");
    xe::Logger::Shutdown();
    return 0;
}
