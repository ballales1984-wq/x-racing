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

protected:
    void OnUpdate(float dt) override {
        const float t = GetClock().GetTotalTime();

        // Cursor grab: ESC toggles capture; click captures again.
        const bool esc_pressed = GetInput().IsKeyPressed(xe::Key::Escape);
        if (esc_pressed) {
            cursor_captured_ = !cursor_captured_;
            GetWindow().SetCursorCapture(cursor_captured_);
        }
        if (!cursor_captured_ && mouse_ && mouse_->GetState().WasPressed(xe::MouseButton::Left)) {
            cursor_captured_ = true;
            GetWindow().SetCursorCapture(true);
        }

        // Drive fly camera
        const auto& ms = mouse_ ? mouse_->GetState() : xe::MouseState{};
        xe::Input& inp = GetInput();
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

        // Push camera state into scene so the renderer uses it
        float px, py, pz;
        controller_.GetPosition(px, py, pz);
        scene.camera_position = { px, py, pz };
        xe::Camera cam;
        controller_.ApplyTo(cam);
        scene.camera_target = { cam.GetView().m[0][0] * 0.0f + cam.GetView().m[0][3] * 0.0f, 0.0f, 0.0f };
        // Simpler: use GetForward
        auto fwd = controller_.GetForward();
        scene.camera_target = { px + fwd.x * 5.0f, py + fwd.y * 5.0f, pz + fwd.z * 5.0f };
        scene.camera_up = { 0.0f, 1.0f, 0.0f };

        // Animate the first 3 cubes around in a circle (independent of camera)
        if (scene.objects.size() >= 3) {
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

        // Orbiting sun (direction points FROM surface TOWARD the light)
        const float sa = t * 0.3f;
        sun.direction = {
            std::cos(sa) * 0.5f,
            1.0f,
            std::sin(sa) * 0.5f
        };
        sun.color = { 1.0f, 0.95f, 0.85f };
        sun.intensity = 1.2f + 0.3f * std::sin(t * 0.5f);

        if (auto* r = dynamic_cast<xe::DX12Renderer*>(GetRenderer())) {
            r->SetScene(&scene);
            r->SetLight(&sun);
            r->SetCameraPosition(px, py, pz);
            r->SetClearColor(0.04f, 0.06f, 0.10f, 1.0f);
        }

        // Billboard sprite toward camera
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
            ss << L"X-Engine V0.8  |  FPS: " << static_cast<int>(1.0f / std::max(dt, 1e-6f))
               << L"  |  Objs: " << scene.objects.size()
               << L"  |  t=" << t;
            hud_->DrawText(10, 10, ss.str(), RGB(255, 255, 255));

            ss.str(L"");
            ss << L"Cam pos: (" << px << ", " << py << ", " << pz << ")"
               << L"  yaw=" << controller_.GetYaw()
               << L"  pitch=" << controller_.GetPitch();
            hud_->DrawText(10, 30, ss.str(), RGB(180, 240, 200));

            ss.str(L"");
            ss << L"Cursor: " << (cursor_captured_ ? L"CAPTURED" : L"FREE  ")
               << L"   ESC to toggle, click to recapture";
            hud_->DrawText(10, 50, ss.str(),
                           cursor_captured_ ? RGB(255, 220, 120) : RGB(180, 180, 200));

            ss.str(L"");
            ss << L"WASD=move  Space/Ctrl=up/down  Mouse=look";
            hud_->DrawText(10, 70, ss.str(), RGB(150, 200, 220));

            hud_->EndDraw();
        }
    }

private:
    xe::Scene scene = []{
        xe::Scene s;
        xe::SceneObject o1;
        o1.name = "cube_a"; o1.instance.position = { 1.0f, 0.0f, 0.0f };
        s.objects.push_back(o1);
        xe::SceneObject o2;
        o2.name = "cube_b"; o2.instance.position = { -1.0f, 0.0f, 0.0f };
        s.objects.push_back(o2);
        xe::SceneObject o3;
        o3.name = "cube_c"; o3.instance.position = { 0.0f, 0.0f, 1.0f };
        s.objects.push_back(o3);
        xe::SceneObject o4;
        o4.name = "ground"; o4.instance.position = { 0.0f, -1.0f, 0.0f };
        o4.instance.scale = { 4.0f, 0.1f, 4.0f };
        o4.instance.tint = { 0.6f, 0.6f, 0.65f, 1.0f };
        s.objects.push_back(o4);

        // Textured sprite — if texture exists, it'll show; otherwise still draws
        xe::SceneObject tex;
        tex.name = "sprite";
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
    bool cursor_captured_ = true;
};

}  // namespace

int main() {
    xe::Logger::Init();
    XE_LOG_INFO("X-Engine V0.8");

    auto window   = std::make_unique<xe::Win32Window>();
    auto input    = std::make_unique<xe::Win32Input>();
    auto mouse    = std::make_unique<xe::Win32Mouse>();
    auto renderer = std::make_unique<xe::DX12Renderer>();

    SceneApp app(std::move(window), std::move(input), std::move(renderer));

    if (!app.Create("X-Engine V0.8 — Fly Camera + Input", 1280, 720)) {
        XE_LOG_ERROR("Failed to initialize engine");
        xe::Logger::Shutdown();
        return -1;
    }

    xe::HudOverlay hud;
    HWND hwnd = reinterpret_cast<HWND>(app.GetWindow().GetNativeHandle());
    hud.Initialize(hwnd);

    app.SetMouse(mouse.get());
    app.SetHud(&hud);

    app.Run();
    app.Shutdown();

    XE_LOG_INFO("Done");
    xe::Logger::Shutdown();
    return 0;
}