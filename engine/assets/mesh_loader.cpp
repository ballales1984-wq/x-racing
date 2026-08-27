#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>

// Project 0 — OBJ/MTL mesh loader implementation
// Parses Wavefront .obj files and .mtl material libraries.
// Supports vertex colors, per-face materials and MTL diffuse colors.
namespace p0::assets {

// Parse a .mtl material library file and return a Material struct.
// Reads diffuse (Kd), ambient (Ka), specular (Ks), shininess (Ns),
// and transparency (d/Tr) properties. Returns a default gray material
// if the file cannot be opened.
static Material parse_material(const std::string& mtl_path) {
    Material mat;
    mat.name = "default";
    mat.diffuse = Vec3(0.7, 0.7, 0.7);

    std::ifstream file(mtl_path);
    if (!file.is_open()) return mat;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "newmtl") {
            std::string name;
            iss >> name;
            if (!name.empty()) mat.name = name;
        } else if (prefix == "Kd") {
            double r, g, b;
            if (iss >> r >> g >> b) {
                mat.diffuse = Vec3(r, g, b);
            }
        } else if (prefix == "Ka") {
            double r, g, b;
            if (iss >> r >> g >> b) {
                mat.ambient = Vec3(r, g, b);
            }
        } else if (prefix == "Ks") {
            double r, g, b;
            if (iss >> r >> g >> b) {
                mat.specular = Vec3(r, g, b);
            }
        } else if (prefix == "Ns") {
            iss >> mat.shininess;
        } else if (prefix == "d" || prefix == "Tr") {
            double alpha;
            if (iss >> alpha) {
                mat.alpha = alpha;
            }
        }
    }

    return mat;
}

// Load a Wavefront OBJ file into a Mesh structure.
// Supports v (vertex position + optional color), vn (normal), vt (UV),
// f (triangular face with v/vt/vn indices), usemtl (material selection),
// and mtllib (external material library).
// The function expands indexed faces into a flat triangle list so the
// output mesh can be rendered directly without index rebinding.
bool MeshLoader::LoadOBJ(const std::string& filename, Mesh& out_mesh, std::vector<Material>& out_materials) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open mesh file: " << filename << std::endl;
        return false;
    }

    out_mesh = Mesh();
    out_materials.clear();

    // Temporary storage for parsed attributes.
    std::vector<Vec3> temp_vertices;
    std::vector<Vec3> temp_normals;
    std::vector<Vec2> temp_uvs;
    std::vector<Vec3> temp_colors;
    std::vector<int> vertex_indices;
    std::vector<int> normal_indices;
    std::vector<int> uv_indices;
    std::vector<int> material_indices;

    std::string current_material;
    Material default_mat;
    default_mat.name = "default";
    default_mat.diffuse = Vec3(0.7, 0.7, 0.7);
    out_materials.push_back(default_mat);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            Vec3 v;
            iss >> v.x() >> v.y() >> v.z();
            temp_vertices.push_back(v);
            // OBJ vertex colors are optional (r g b after x y z).
            Vec3 c(1.0, 1.0, 1.0);
            if (iss >> c.x() >> c.y() >> c.z()) {
                temp_colors.push_back(c);
            } else {
                temp_colors.push_back(Vec3(1.0, 1.0, 1.0));
            }
        } else if (prefix == "vn") {
            Vec3 n;
            iss >> n.x() >> n.y() >> n.z();
            temp_normals.push_back(n);
        } else if (prefix == "vt") {
            Vec2 uv;
            iss >> uv.x() >> uv.y();
            temp_uvs.push_back(uv);
        } else if (prefix == "f") {
            std::string face_data;
            int vi, ni, ui;

            // Parse three vertices per face (triangular only).
            for (int i = 0; i < 3; ++i) {
                if (!(iss >> face_data)) break;

                std::istringstream fss(face_data);
                vi = ni = ui = 0;

                // Parse v/vt/vn format (e.g., "1/2/3" or "1//3" or "1").
                fss >> vi;
                if (fss.peek() == '/') {
                    fss.ignore();
                    if (fss.peek() != '/') {
                        fss >> ui;
                    }
                    if (fss.peek() == '/') {
                        fss.ignore();
                        fss >> ni;
                    }
                }

                // OBJ indices are 1-based; convert to 0-based.
                vertex_indices.push_back(vi - 1);
                uv_indices.push_back(ui - 1);
                normal_indices.push_back(ni - 1);
                material_indices.push_back(static_cast<int>(out_materials.size() - 1));
            }
        } else if (prefix == "usemtl") {
            std::string mtl_name;
            iss >> mtl_name;
            current_material = mtl_name;

            // Track which materials are referenced by faces.
            bool found = false;
            for (size_t i = 0; i < out_materials.size(); ++i) {
                if (out_materials[i].name == mtl_name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                Material mat;
                mat.name = mtl_name;
                out_materials.push_back(mat);
            }
        } else if (prefix == "mtllib") {
            std::string mtllib;
            iss >> mtllib;

            // Resolve the MTL path relative to the OBJ file directory.
            size_t slash = filename.find_last_of("\\/");
            std::string mtl_path;
            if (slash != std::string::npos) {
                mtl_path = filename.substr(0, slash + 1) + mtllib;
            } else {
                mtl_path = mtllib;
            }

            // Parse the MTL file and merge its material definitions.
            Material mtl_mat = parse_material(mtl_path);
            bool found = false;
            for (size_t i = 0; i < out_materials.size(); ++i) {
                if (out_materials[i].name == mtl_mat.name) {
                    out_materials[i] = mtl_mat;
                    found = true;
                    break;
                }
            }
            if (!found) {
                out_materials.push_back(mtl_mat);
            }
        }
    }

    if (vertex_indices.empty()) {
        std::cerr << "No faces found in mesh file: " << filename << std::endl;
        return false;
    }

    // Expand indexed face data into flat arrays for direct rendering.
    size_t face_count = vertex_indices.size() / 3;
    out_mesh.vertices.reserve(face_count * 3);
    out_mesh.normals.reserve(face_count * 3);
    out_mesh.uvs.reserve(face_count * 3);
    out_mesh.colors.reserve(face_count * 3);
    out_mesh.indices.reserve(face_count * 3);

    for (size_t i = 0; i < vertex_indices.size(); ++i) {
        int vi = vertex_indices[i];
        int ni = normal_indices[i];
        int ui = uv_indices[i];
        int mi = material_indices[i];

        if (vi >= 0 && vi < static_cast<int>(temp_vertices.size())) {
            out_mesh.vertices.push_back(temp_vertices[vi]);
        }
        if (ni >= 0 && ni < static_cast<int>(temp_normals.size())) {
            out_mesh.normals.push_back(temp_normals[ni]);
        }
        if (ui >= 0 && ui < static_cast<int>(temp_uvs.size())) {
            out_mesh.uvs.push_back(temp_uvs[ui]);
        }

        // Determine vertex color: prefer material diffuse, fall back to vertex color.
        Vec3 color(1.0, 1.0, 1.0);
        if (vi >= 0 && vi < static_cast<int>(temp_colors.size())) {
            color = temp_colors[vi];
        }
        if (mi >= 0 && mi < static_cast<int>(out_materials.size())) {
            const Material& mat = out_materials[mi];
            color = mat.diffuse;
        }
        out_mesh.colors.push_back(color);

        out_mesh.indices.push_back(static_cast<int>(i));
    }

    if (!out_materials.empty()) {
        out_mesh.material = out_materials[0].name;
    }

    std::cout << "Loaded mesh: " << filename << std::endl;
    std::cout << "  Vertices: " << out_mesh.vertices.size() << std::endl;
    std::cout << "  Triangles: " << out_mesh.indices.size() / 3 << std::endl;
    std::cout << "  Materials: " << out_materials.size() << std::endl;

    return true;
}

}
