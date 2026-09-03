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

            // Apply selection highlight (tint) to scene objects linked to bodies 0..N-1.
            if (physics_) {
                int sel = physics_->Selected();
                for (int i = 0; i < physics_->Size() && i < static_cast<int>(scene.objects.size()); ++i) {
                    if (i == sel) {
                        scene.objects[i].instance.tint = { 1.0f, 0.9f, 0.2f, 1.0f };  // yellow highlight
                    } else {
                        scene.objects[i].instance.tint = { 1.0f, 1.0f, 1.0f, 1.0f };
                    }
                }
            }

            // Drag / pick with left mouse button.
            if (physics_ && mouse_) {
                const auto& mstate = mouse_->GetState();
                auto fwd = controller_.GetForward();
                xe::Ray r;
                r.origin = { px, py, pz };
                r.dir = { fwd.x, fwd.y, fwd.z };

                if (mstate.WasPressed(xe::MouseButton::Left) && cursor_captured_) {
                    auto hit = physics_->RayCast(r, 200.0f);
                    if (hit.body >= 0 && physics_->BeginDrag(hit.body, hit.point)) {
                        drag_plane_dist_ = hit.t;
                        if (console_) {
                            std::ostringstream o; o << "Drag begin: body " << hit.body
                                                    << " at t=" << hit.t;
                            console_->PrintLn(o.str());
                        }
                    } else if (hit.body >= 0) {
                        // Hit a static body — just select it.
                        physics_->SetSelected(hit.body);
                        if (console_) console_->PrintLn("Selected static body (no drag).");
                    } else {
                        physics_->SetSelected(-1);
                        if (console_) console_->PrintLn("Picked: (nothing)");
                    }
                } else if (mstate.IsDown(xe::MouseButton::Left) && physics_->IsDragging()) {
                    // Update anchor: intersection of camera ray with plane
                    // perpendicular to forward at the original hit distance.
                    xe::Vec3 planePoint = { r.origin.x + r.dir.x * drag_plane_dist_,
                                             r.origin.y + r.dir.y * drag_plane_dist_,
                                             r.origin.z + r.dir.z * drag_plane_dist_ };
                    physics_->UpdateDragAnchor(planePoint);
                }
                if (mstate.WasReleased(xe::MouseButton::Left) && physics_->IsDragging()) {
                    if (console_) {
                        std::ostringstream o; o << "Drag end: body " << physics_->DragIndex();
                        console_->PrintLn(o.str());
                    }
                    physics_->EndDrag();
                }

                // RMB rotation drag: pick a body, then drag to spin it.
                if (mstate.WasPressed(xe::MouseButton::Right) && cursor_captured_) {
                    auto fwd2 = controller_.GetForward();
                    xe::Ray r2;
                    r2.origin = { px, py, pz };
                    r2.dir = { fwd2.x, fwd2.y, fwd2.z };
                    auto hit = physics_->RayCast(r2, 200.0f);
                    if (hit.body >= 0 && hit.body < physics_->Size()) {
                        const auto& b = physics_->Get(hit.body);
                        if (b.dynamic) {
                            rot_drag_body_ = hit.body;
                            physics_->SetSelected(hit.body);
                            if (console_) {
                                std::ostringstream o; o << "Rot-drag begin: body " << hit.body;
                                console_->PrintLn(o.str());
                            }
                        }
                    }
                }
                if (rot_drag_body_ >= 0 && mstate.IsDown(xe::MouseButton::Right)) {
                    // Map mouse delta to world Y and right axes (camera-relative).
                    // dx -> spin about world up; dy -> spin about camera right.
                    float sensitivity = 6.0f;
                    auto fwd2 = controller_.GetForward();
                    xe::Vec3 right = { -fwd2.z, 0, fwd2.x };
                    float rl = std::sqrt(right.x*right.x + right.y*right.y + right.z*right.z);
                    if (rl > 1e-6f) { right.x /= rl; right.y /= rl; right.z /= rl; }
                    xe::Vec3 up = { 0, 1, 0 };
                    xe::Vec3 dAngVel = {
                        up.x * (ms.dy * sensitivity) + right.x * (-ms.dx * sensitivity),
                        up.y * (ms.dy * sensitivity) + right.y * (-ms.dx * sensitivity),
                        up.z * (ms.dy * sensitivity) + right.z * (-ms.dx * sensitivity),
                    };
                    physics_->AddAngVel(rot_drag_body_, dAngVel);
                }
                if (mstate.WasReleased(xe::MouseButton::Right) && rot_drag_body_ >= 0) {
                    if (console_) {
                        std::ostringstream o; o << "Rot-drag end: body " << rot_drag_body_;
                        console_->PrintLn(o.str());
                    }
                    rot_drag_body_ = -1;
                }
            }

            if (hud_) {
                hud_->BeginDraw();
                std::wostringstream ss;
                ss << L"X-Engine V0.17  |  FPS: " << static_cast<int>(1.0f / std::max(dt, 1e-6f))
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
                if (physics_ && physics_->IsDragging()) {
                    ss.str(L"");
                    ss << L"DRAGGING body " << physics_->DragIndex();
                    hud_->DrawText(10, 70, ss.str(), RGB(255, 100, 100));
                } else if (rot_drag_body_ >= 0) {
                    ss.str(L"");
                    ss << L"ROT-DRAGGING body " << rot_drag_body_;
                    hud_->DrawText(10, 70, ss.str(), RGB(255, 200, 100));
                } else if (physics_) {
                    ss.str(L"");
                    ss << L"Selected: " << physics_->Selected()
                       << L"   |  LMB=move  RMB=rotate  `=console  ESC=release";
                    hud_->DrawText(10, 70, ss.str(), RGB(180, 180, 220));
                }
                if (physics_) {
                    ss.str(L"");
                    ss << L"Gravity: " << (physics_->GravityEnabled() ? L"ON " : L"OFF")
                       << L" (" << physics_->Gravity().x << L","
                       << physics_->Gravity().y << L","
                       << physics_->Gravity().z << L")";
                    hud_->DrawText(10, 90, ss.str(),
                                   physics_->GravityEnabled() ? RGB(255, 200, 100) : RGB(150, 150, 170));
                    ss.str(L"");
                    ss << L"Sleep: " << (physics_->SleepingEnabled() ? L"ON " : L"OFF")
                       << L"  sleeping=" << physics_->SleepingCount() << L"/" << physics_->Size()
                       << L"  constraints=" << physics_->NumConstraints()
                       << L"  joints=" << (physics_->NumBallJoints() + physics_->NumHingeJoints());
                    hud_->DrawText(10, 110, ss.str(), RGB(180, 200, 220));
                }
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
    float drag_plane_dist_ = 0.0f;
    int   rot_drag_body_ = -1;     // body being rotated via RMB drag
};

}  // namespace

int main() {
    xe::Logger::Init();
    XE_LOG_INFO("X-Engine V0.17");

    auto window   = std::make_unique<xe::Win32Window>();
    xe::Win32Window* raw_window = window.get();
    auto input    = std::make_unique<xe::Win32Input>();
    auto mouse    = std::make_unique<xe::Win32Mouse>();
    auto renderer = std::make_unique<xe::DX12Renderer>();

    SceneApp app(std::move(window), std::move(input), std::move(renderer));

    if (!app.Create("X-Engine V0.17 — Fly Camera + Console + Physics", 1280, 720)) {
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
    physics.SetGravity({ 0, -9.81f, 0 });
    physics.SetGravityEnabled(false);  // off by default; toggle via console

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
    console.Register("pick", "Select body N (or 'clear') for highlight",
                     [&physics, &console](const auto& args) {
        if (args.size() < 2) {
            console.PrintLn("Usage: pick <idx>|clear");
            return;
        }
        if (args[1] == "clear") {
            physics.SetSelected(-1);
            console.PrintLn("Selection cleared.");
            return;
        }
        int idx = std::stoi(args[1]);
        if (idx < 0 || idx >= physics.Size()) {
            console.PrintLn("Invalid body index.");
            return;
        }
        physics.SetSelected(idx);
        const auto& b = physics.Get(idx);
        std::ostringstream o; o << "Selected body " << idx
                                << " pos=(" << b.position.x << "," << b.position.y << "," << b.position.z << ")"
                                << " vel=(" << b.velocity.x << "," << b.velocity.y << "," << b.velocity.z << ")";
        console.PrintLn(o.str());
    });
    console.Register("gravity", "Toggle gravity (on|off) or set value (set y)",
                     [&physics, &console](const auto& args) {
        if (args.size() < 2) {
            std::ostringstream o; o << "Gravity: "
                                    << (physics.GravityEnabled() ? "ON" : "OFF")
                                    << "  (" << physics.Gravity().x << ","
                                    << physics.Gravity().y << ","
                                    << physics.Gravity().z << ")";
            console.PrintLn(o.str());
            return;
        }
        if (args[1] == "on")  { physics.SetGravityEnabled(true);  console.PrintLn("Gravity ON");  return; }
        if (args[1] == "off") { physics.SetGravityEnabled(false); console.PrintLn("Gravity OFF"); return; }
        if (args[1] == "set" && args.size() >= 4) {
            float gx = std::stof(args[2]);
            float gy = std::stof(args[3]);
            float gz = (args.size() >= 5) ? std::stof(args[4]) : 0.0f;
            physics.SetGravity({ gx, gy, gz });
            std::ostringstream o; o << "Gravity set to (" << gx << "," << gy << "," << gz << ")";
            console.PrintLn(o.str());
            return;
        }
        console.PrintLn("Usage: gravity [on|off|set <x> <y> <z>]");
    });
    console.Register("sleep", "Sleep/wake bodies (on|off|all|list|listall)",
                     [&physics, &console](const auto& args) {
        if (args.size() < 2) {
            std::ostringstream o; o << "Sleep: "
                                    << (physics.SleepingEnabled() ? "ON" : "OFF")
                                    << "  sleeping=" << physics.SleepingCount()
                                    << "/" << physics.Size();
            console.PrintLn(o.str());
            return;
        }
        if (args[1] == "on")  { physics.SetSleepingEnabled(true);  console.PrintLn("Sleep ON");  return; }
        if (args[1] == "off") { physics.SetSleepingEnabled(false); console.PrintLn("Sleep OFF"); return; }
        if (args[1] == "all") {
            for (int i = 0; i < physics.Size(); ++i) physics.SleepBody(i);
            std::ostringstream o; o << "All bodies sleeping: " << physics.SleepingCount();
            console.PrintLn(o.str());
            return;
        }
        if (args[1] == "list" || args[1] == "listall") {
            for (int i = 0; i < physics.Size(); ++i) {
                std::ostringstream o; o << "  body " << i << ": "
                                        << (physics.Get(i).awake ? "awake" : "sleeping");
                console.PrintLn(o.str());
            }
            return;
        }
        console.PrintLn("Usage: sleep [on|off|all|list|listall]");
    });
    console.Register("link", "Link two bodies with distance constraint (link A B [L])",
                     [&physics, &console](const auto& args) {
        if (args.size() < 3) {
            console.PrintLn("Usage: link <body_a> <body_b> [rest_length]");
            return;
        }
        int a = std::stoi(args[1]);
        int b = std::stoi(args[2]);
        if (a < 0 || a >= physics.Size() || b < 0 || b >= physics.Size()) {
            console.PrintLn("Invalid body index.");
            return;
        }
        const auto& A = physics.Get(a);
        const auto& B = physics.Get(b);
        float dx = B.position.x - A.position.x;
        float dy = B.position.y - A.position.y;
        float dz = B.position.z - A.position.z;
        float L = (args.size() >= 4) ? std::stof(args[3]) : std::sqrt(dx*dx + dy*dy + dz*dz);
        xe::DistanceConstraint c;
        c.a = a; c.b = b; c.restLength = L; c.stiffness = 1.0f;
        int id = physics.AddConstraint(c);
        std::ostringstream o; o << "Linked " << a << " <-> " << b
                                << " rest=" << L << "  constraint id=" << id;
        console.PrintLn(o.str());
    });
    console.Register("unlink", "Remove all constraints",
                     [&physics, &console](auto&) {
        int n = physics.NumConstraints();
        physics.ClearConstraints();
        std::ostringstream o; o << "Removed " << n << " constraint(s).";
        console.PrintLn(o.str());
    });
    console.Register("pin", "Pin two bodies with a ball joint (pin A B)",
                     [&physics, &console](const auto& args) {
        if (args.size() < 3) {
            console.PrintLn("Usage: pin <a> <b>");
            return;
        }
        int a = std::stoi(args[1]);
        int b = std::stoi(args[2]);
        if (a < 0 || a >= physics.Size() || b < 0 || b >= physics.Size()) {
            console.PrintLn("Invalid body index.");
            return;
        }
        xe::BallJoint j;
        j.a = a; j.b = b;
        j.localA = { 0, 0, 0 };
        j.localB = { 0, 0, 0 };
        j.stiffness = 1.0f;
        int id = physics.AddBallJoint(j);
        std::ostringstream o; o << "Pinned " << a << " <-> " << b
                                << "  ball joint id=" << id;
        console.PrintLn(o.str());
    });
    console.Register("hinge", "Hinge two bodies about a local axis (hinge A B [ax ay az])",
                     [&physics, &console](const auto& args) {
        if (args.size() < 3) {
            console.PrintLn("Usage: hinge <a> <b> [ax ay az]   (default axis = +Z)");
            return;
        }
        int a = std::stoi(args[1]);
        int b = std::stoi(args[2]);
        if (a < 0 || a >= physics.Size() || b < 0 || b >= physics.Size()) {
            console.PrintLn("Invalid body index.");
            return;
        }
        xe::HingeJoint j;
        j.a = a; j.b = b;
        j.localA = { 0, 0, 0 };
        j.localB = { 0, 0, 0 };
        if (args.size() >= 6) {
            j.localAxisA = { std::stof(args[3]), std::stof(args[4]), std::stof(args[5]) };
        } else {
            j.localAxisA = { 0, 0, 1 };
        }
        j.stiffness = 1.0f;
        int id = physics.AddHingeJoint(j);
        std::ostringstream o; o << "Hinged " << a << " <-> " << b
                                << " axis=(" << j.localAxisA.x << "," << j.localAxisA.y << "," << j.localAxisA.z << ")"
                                << "  id=" << id;
        console.PrintLn(o.str());
    });
    console.Register("unpin", "Remove all joints",
                     [&physics, &console](auto&) {
        int nb = physics.NumBallJoints();
        int nh = physics.NumHingeJoints();
        physics.ClearJoints();
        std::ostringstream o; o << "Removed " << (nb + nh) << " joint(s).";
        console.PrintLn(o.str());
    });
    console.Register("spawn", "Spawn a body (sphere|box|rope) at a position",
                     [&physics, &console](const auto& args) {
        if (args.size() < 2) {
            console.PrintLn("Usage:");
            console.PrintLn("  spawn sphere <x> <y> <z> <radius> [mass]");
            console.PrintLn("  spawn box    <x> <y> <z> <hx> <hy> <hz> [mass]");
            console.PrintLn("  spawn rope   <x> <y> <z> <n> <segLen> <beadR> [mass]");
            console.PrintLn("  spawn ground <x> <y> <z> <hx> <hy> <hz>    (static)");
            return;
        }
        std::string kind = args[1];
        if (kind == "sphere" && args.size() >= 6) {
            float x = std::stof(args[2]), y = std::stof(args[3]), z = std::stof(args[4]);
            float r = std::stof(args[5]);
            float m = (args.size() >= 7) ? std::stof(args[6]) : 1.0f;
            int id = physics.AddSphere({ x, y, z }, r, m);
            std::ostringstream o; o << "Spawned sphere id=" << id << " pos=(" << x << "," << y << "," << z << ") r=" << r;
            console.PrintLn(o.str());
        } else if (kind == "box" && args.size() >= 8) {
            float x = std::stof(args[2]), y = std::stof(args[3]), z = std::stof(args[4]);
            float hx = std::stof(args[5]), hy = std::stof(args[6]), hz = std::stof(args[7]);
            float m  = (args.size() >= 9) ? std::stof(args[8]) : 1.0f;
            int id = physics.AddBox({ x, y, z }, { hx, hy, hz }, m);
            std::ostringstream o; o << "Spawned box id=" << id << " pos=(" << x << "," << y << "," << z << ") half=(" << hx << "," << hy << "," << hz << ")";
            console.PrintLn(o.str());
        } else if (kind == "ground" && args.size() >= 8) {
            float x = std::stof(args[2]), y = std::stof(args[3]), z = std::stof(args[4]);
            float hx = std::stof(args[5]), hy = std::stof(args[6]), hz = std::stof(args[7]);
            int id = physics.AddStaticBox({ x, y, z }, { hx, hy, hz });
            std::ostringstream o; o << "Spawned ground (static) id=" << id;
            console.PrintLn(o.str());
        } else if (kind == "rope" && args.size() >= 7) {
            float x = std::stof(args[2]), y = std::stof(args[3]), z = std::stof(args[4]);
            int   n       = std::stoi(args[5]);
            float segLen  = std::stof(args[6]);
            float beadR   = std::stof(args[7]);
            float mass    = (args.size() >= 9) ? std::stof(args[8]) : 0.2f;
            int first = physics.BuildRope(-1, { x, y, z }, n, segLen, beadR, mass);
            std::ostringstream o; o << "Spawned rope: " << n << " beads, first id=" << first
                                    << " last id=" << (first + n - 1);
            console.PrintLn(o.str());
        } else {
            console.PrintLn("Bad arguments. Try: spawn sphere|box|rope|ground");
        }
    });
    console.Register("reset_all", "Clear all bodies and reset to a clean state",
                     [&physics, &console](auto&) {
        physics.Clear();
        physics.ClearConstraints();
        physics.ClearJoints();
        console.PrintLn("World cleared.");
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
    console.PrintLn("X-Engine V0.17 console.  'help' for commands, '`' or ESC to close.");

    app.Run();
    app.Shutdown();

    XE_LOG_INFO("Done");
    xe::Logger::Shutdown();
    return 0;
}
