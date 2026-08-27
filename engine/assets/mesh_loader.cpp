#include "mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>

// Project 0 — OBJ mesh loader implementation
// Parses Wavefront .obj files (vertices, normals, UVs, triangular faces).
namespace p0::assets {

// Load an OBJ file into a Mesh. Returns false on I/O or parse error.
// The OBJ format uses 1-based indices; this loader converts them to 0-based.
bool MeshLoader::LoadOBJ(const std::string& filename, Mesh& out_mesh) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open mesh file: " << filename << std::endl;
        return false;
    }

    out_mesh = Mesh();
    std::vector<Vec3> temp_vertices;
    std::vector<Vec3> temp_normals;
    std::vector<Vec2> temp_uvs;
    std::vector<Vec3> temp_colors;
    std::vector<int> vertex_indices;
    std::vector<int> normal_indices;
    std::vector<int> uv_indices;

    // Parse the OBJ file line by line.
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        // Vertex position (x, y, z) with optional color (r, g, b)
        if (prefix == "v") {
            Vec3 v;
            iss >> v.x() >> v.y() >> v.z();
            temp_vertices.push_back(v);
            Vec3 c(1.0, 1.0, 1.0);
            if (iss >> c.x() >> c.y() >> c.z()) {
                temp_colors.push_back(c);
            } else {
                temp_colors.push_back(c);
            }
        // Vertex normal (nx, ny, nz)
        } else if (prefix == "vn") {
            Vec3 n;
            iss >> n.x() >> n.y() >> n.z();
            temp_normals.push_back(n);
        // Texture coordinate (u, v)
        } else if (prefix == "vt") {
            Vec2 uv;
            iss >> uv.x() >> uv.y();
            temp_uvs.push_back(uv);
        // Triangular face: supports v, v/vt, and v/vt/vn formats
        } else if (prefix == "f") {
            std::string face_data;
            int vi, ni, ui;
            char slash;

            for (int i = 0; i < 3; ++i) {
                if (!(iss >> face_data)) break;

                std::istringstream fss(face_data);
                vi = ni = ui = 0;

                fss >> vi;
                if (fss.peek() == '/') {
                    fss >> slash;
                    if (fss.peek() != '/') {
                        fss >> ui;
                    }
                    if (fss.peek() == '/') {
                        fss >> slash;
                        fss >> ni;
                    }
                }

                vertex_indices.push_back(vi - 1);
                uv_indices.push_back(ui - 1);
                normal_indices.push_back(ni - 1);
            }
        }
    }

    // Expand indexed OBJ data into per-vertex arrays for the renderer.
    if (vertex_indices.empty()) {
        std::cerr << "No faces found in mesh file: " << filename << std::endl;
        return false;
    }

    size_t face_count = vertex_indices.size() / 3;
    out_mesh.vertices.reserve(face_count * 3);
    out_mesh.normals.reserve(face_count * 3);
    out_mesh.uvs.reserve(face_count * 3);
    out_mesh.colors.reserve(face_count * 3);
    out_mesh.indices.reserve(face_count * 3);

    // Copy expanded vertex data into the output mesh, guarding against
    // out-of-range indices (OBJ indices are 1-based, so -1 converts to 0-based).
    for (size_t i = 0; i < vertex_indices.size(); ++i) {
        int vi = vertex_indices[i];
        int ni = normal_indices[i];
        int ui = uv_indices[i];

        if (vi >= 0 && vi < static_cast<int>(temp_vertices.size())) {
            out_mesh.vertices.push_back(temp_vertices[vi]);
        }
        if (ni >= 0 && ni < static_cast<int>(temp_normals.size())) {
            out_mesh.normals.push_back(temp_normals[ni]);
        }
        if (ui >= 0 && ui < static_cast<int>(temp_uvs.size())) {
            out_mesh.uvs.push_back(temp_uvs[ui]);
        }
        if (vi >= 0 && vi < static_cast<int>(temp_colors.size())) {
            out_mesh.colors.push_back(temp_colors[vi]);
        } else {
            out_mesh.colors.push_back(Vec3(1.0, 1.0, 1.0));
        }

        out_mesh.indices.push_back(static_cast<int>(i));
    }

    std::cout << "Loaded mesh: " << filename << std::endl;
    std::cout << "  Vertices: " << out_mesh.vertices.size() << std::endl;
    std::cout << "  Triangles: " << out_mesh.indices.size() / 3 << std::endl;

    return true;
}

}
