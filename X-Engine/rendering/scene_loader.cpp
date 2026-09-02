#include "rendering/scene_loader.h"

#include <fstream>
#include <sstream>
#include <cctype>

namespace xe {

namespace {

void SkipWhitespace(const std::string& s, size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
}

bool Consume(const std::string& s, size_t& i, char c) {
    SkipWhitespace(s, i);
    if (i < s.size() && s[i] == c) { ++i; return true; }
    return false;
}

bool ConsumeWord(const std::string& s, size_t& i, const std::string& word) {
    SkipWhitespace(s, i);
    if (s.compare(i, word.size(), word) == 0) { i += word.size(); return true; }
    return false;
}

bool ParseFloat(const std::string& s, size_t& i, float& out) {
    SkipWhitespace(s, i);
    size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) ++i;
    if (start == i) return false;
    out = std::stof(s.substr(start, i - start));
    return true;
}

bool ParseVec3(const std::string& s, size_t& i, Vec3& v) {
    if (!Consume(s, i, '[')) return false;
    if (!ParseFloat(s, i, v.x)) return false;
    if (!Consume(s, i, ',')) return false;
    if (!ParseFloat(s, i, v.y)) return false;
    if (!Consume(s, i, ',')) return false;
    if (!ParseFloat(s, i, v.z)) return false;
    if (!Consume(s, i, ']')) return false;
    return true;
}

bool ParseString(const std::string& s, size_t& i, std::string& out) {
    SkipWhitespace(s, i);
    if (i >= s.size() || s[i] != '"') return false;
    ++i;
    std::string r;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) {
            r += s[i + 1];
            i += 2;
        } else {
            r += s[i++];
        }
    }
    if (i >= s.size()) return false;
    ++i;
    out = std::move(r);
    return true;
}

bool ParseTint(const std::string& s, size_t& i, std::array<float, 4>& t) {
    if (!Consume(s, i, '[')) return false;
    for (int k = 0; k < 4; ++k) {
        if (!ParseFloat(s, i, t[k])) return false;
        if (k < 3) { if (!Consume(s, i, ',')) return false; }
    }
    return Consume(s, i, ']');
}

}  // namespace

std::string SceneLoader::ToJson(const Scene& scene) {
    std::ostringstream os;
    os << "{\n";
    os << "  \"camera_position\": [" << scene.camera_position.x << ", "
                                   << scene.camera_position.y << ", "
                                   << scene.camera_position.z << "],\n";
    os << "  \"camera_target\":   [" << scene.camera_target.x << ", "
                                   << scene.camera_target.y << ", "
                                   << scene.camera_target.z << "],\n";
    os << "  \"camera_up\":       [" << scene.camera_up.x << ", "
                                   << scene.camera_up.y << ", "
                                   << scene.camera_up.z << "],\n";
    os << "  \"camera_fov\":      " << scene.camera_fov_y << ",\n";
    os << "  \"objects\": [\n";
    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const auto& o = scene.objects[i];
        os << "    {\"name\":\"" << o.name << "\",";
        os << "\"mesh\":";
        switch (o.instance.mesh) {
            case MeshKind::Triangle: os << "\"triangle\""; break;
            case MeshKind::Cube:     os << "\"cube\""; break;
            case MeshKind::Quad:     os << "\"quad\""; break;
        }
        os << ",\"position\":[" << o.instance.position.x << ","
                                << o.instance.position.y << ","
                                << o.instance.position.z << "]";
        os << ",\"rotation\":[" << o.instance.rotation_rad.x << ","
                                << o.instance.rotation_rad.y << ","
                                << o.instance.rotation_rad.z << "]";
        os << ",\"scale\":["    << o.instance.scale.x << ","
                                << o.instance.scale.y << ","
                                << o.instance.scale.z << "]";
        os << ",\"tint\":["  << o.instance.tint[0] << ","
                            << o.instance.tint[1] << ","
                            << o.instance.tint[2] << ","
                            << o.instance.tint[3] << "]";
        if (!o.instance.texture_path.empty()) {
            os << ",\"texture\":\"" << o.instance.texture_path << "\"";
        }
        os << "}";
        if (i + 1 < scene.objects.size()) os << ",";
        os << "\n";
    }
    os << "  ]\n}\n";
    return os.str();
}

Scene SceneLoader::FromJson(const std::string& text) {
    Scene scene;
    size_t i = 0;
    if (!Consume(text, i, '{')) return scene;

    while (i < text.size()) {
        if (Consume(text, i, '}')) break;
        std::string key;
        if (!ParseString(text, i, key)) break;
        if (!Consume(text, i, ':')) break;

        if (key == "camera_position") { ParseVec3(text, i, scene.camera_position); }
        else if (key == "camera_target")   { ParseVec3(text, i, scene.camera_target); }
        else if (key == "camera_up")       { ParseVec3(text, i, scene.camera_up); }
        else if (key == "camera_fov")      { ParseFloat(text, i, scene.camera_fov_y); }
        else if (key == "objects") {
            if (!Consume(text, i, '[')) break;
            while (i < text.size()) {
                if (Consume(text, i, ']')) break;
                if (!Consume(text, i, '{')) break;
                SceneObject obj;
                while (i < text.size()) {
                    if (Consume(text, i, '}')) break;
                    std::string fk;
                    if (!ParseString(text, i, fk)) break;
                    if (!Consume(text, i, ':')) break;
                    if (fk == "name")         { ParseString(text, i, obj.name); }
                    else if (fk == "mesh")    {
                        std::string m;
                        ParseString(text, i, m);
                        if (m == "triangle")      obj.instance.mesh = MeshKind::Triangle;
                        else if (m == "quad")     obj.instance.mesh = MeshKind::Quad;
                        else                       obj.instance.mesh = MeshKind::Cube;
                    }
                    else if (fk == "position") { ParseVec3(text, i, obj.instance.position); }
                    else if (fk == "rotation") { ParseVec3(text, i, obj.instance.rotation_rad); }
                    else if (fk == "scale")    { ParseVec3(text, i, obj.instance.scale); }
                    else if (fk == "tint")     { ParseTint(text, i, obj.instance.tint); }
                    else if (fk == "texture")  { ParseString(text, i, obj.instance.texture_path); }
                    else {
                        SkipWhitespace(text, i);
                        int depth = 0;
                        while (i < text.size() && (depth > 0 || text[i] != ',')) {
                            if (text[i] == '[' || text[i] == '{') ++depth;
                            else if (text[i] == ']' || text[i] == '}') --depth;
                            ++i;
                            if (depth < 0) break;
                        }
                    }
                    Consume(text, i, ',');
                }
                Consume(text, i, ',');
                scene.objects.push_back(std::move(obj));
            }
        }
        Consume(text, i, ',');
    }
    return scene;
}

Scene SceneLoader::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return Scene{};
    std::stringstream ss;
    ss << f.rdbuf();
    return FromJson(ss.str());
}

bool SceneLoader::SaveToFile(const Scene& scene, const std::string& path) {
    std::ofstream f(path);
    if (!f) return false;
    f << ToJson(scene);
    return f.good();
}

}  // namespace xe