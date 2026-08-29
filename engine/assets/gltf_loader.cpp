#include "gltf_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace p0::assets {

namespace {

struct GLBHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t length;
};

struct GLBChunkHeader {
    uint32_t length;
    uint32_t type;
};

Vec3 ReadVec3(const char* data, size_t offset) {
    return Vec3(*reinterpret_cast<const float*>(data + offset),
                *reinterpret_cast<const float*>(data + offset + 4),
                *reinterpret_cast<const float*>(data + offset + 8));
}

Vec2 ReadVec2(const char* data, size_t offset) {
    return Vec2(*reinterpret_cast<const float*>(data + offset),
                *reinterpret_cast<const float*>(data + offset + 4));
}

GLTFMaterial ParseMaterial(const std::string& json, size_t start, size_t end) {
    GLTFMaterial mat;
    mat.name = "default";
    mat.base_color = Vec3(0.7, 0.7, 0.7);

    size_t name_pos = json.find("\"name\"", start);
    if (name_pos != std::string::npos && name_pos < end) {
        size_t colon = json.find(':', name_pos);
        if (colon != std::string::npos) {
            size_t q1 = json.find('"', colon);
            if (q1 != std::string::npos) {
                size_t q2 = json.find('"', q1 + 1);
                if (q2 != std::string::npos) mat.name = json.substr(q1 + 1, q2 - q1 - 1);
            }
        }
    }

    size_t bcf_pos = json.find("\"baseColorFactor\"", start);
    if (bcf_pos != std::string::npos && bcf_pos < end) {
        size_t bracket = json.find('[', bcf_pos);
        if (bracket != std::string::npos && bracket < end) {
            size_t end_bracket = json.find(']', bracket);
            if (end_bracket != std::string::npos) {
                float r, g, b, a;
                if (sscanf_s(json.substr(bracket + 1, end_bracket - bracket - 1).c_str(), "%f %f %f %f", &r, &g, &b, &a) == 4) {
                    mat.base_color = Vec3(r, g, b);
                    mat.alpha = a;
                }
            }
        }
    }

    size_t metallic_pos = json.find("\"metallicFactor\"", start);
    if (metallic_pos != std::string::npos && metallic_pos < end) {
        size_t colon = json.find(':', metallic_pos);
        if (colon != std::string::npos) mat.metallic = std::stof(json.substr(colon + 1));
    }

    size_t roughness_pos = json.find("\"roughnessFactor\"", start);
    if (roughness_pos != std::string::npos && roughness_pos < end) {
        size_t colon = json.find(':', roughness_pos);
        if (colon != std::string::npos) mat.roughness = std::stof(json.substr(colon + 1));
    }

    return mat;
}

size_t GetBufferViewByteOffset(const std::string& buffer_views_str, int bv_idx) {
    size_t bv_search = 0;
    size_t bv_byte_offset = 0;
    int current_bv = 0;
    while (current_bv < bv_idx) {
        size_t bv_start = buffer_views_str.find('{', bv_search);
        if (bv_start == std::string::npos) break;
        size_t bv_end = buffer_views_str.find('}', bv_start);
        if (bv_end == std::string::npos) break;

        size_t byte_len_pos = buffer_views_str.find("\"byteLength\"", bv_start);
        if (byte_len_pos != std::string::npos && byte_len_pos < bv_end) {
            size_t colon = buffer_views_str.find(':', byte_len_pos);
            if (colon != std::string::npos) {
                size_t len_end = buffer_views_str.find(',', colon);
                if (len_end == std::string::npos) len_end = buffer_views_str.find('}', colon);
                bv_byte_offset += std::stoi(buffer_views_str.substr(colon + 1, len_end - colon - 1));
            }
        }

        bv_search = bv_end + 1;
        current_bv++;
    }
    return bv_byte_offset;
}

}

bool GLTFLoader::Load(const std::string& filename, Mesh& out_mesh, std::vector<Material>& out_materials) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open GLTF file: " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> file_data(file_size);
    file.read(file_data.data(), file_size);
    file.close();

    std::string json_data;
    std::vector<char> bin_data;

    if (file_size >= 4 && std::string(file_data.data(), 4) == "glTF") {
        if (file_size < sizeof(GLBHeader)) return false;

        GLBHeader header;
        std::memcpy(&header, file_data.data(), sizeof(GLBHeader));

        size_t offset = sizeof(GLBHeader);
        while (offset + sizeof(GLBChunkHeader) <= file_size) {
            GLBChunkHeader chunk;
            std::memcpy(&chunk, file_data.data() + offset, sizeof(GLBChunkHeader));
            offset += sizeof(GLBChunkHeader);

            if (offset + static_cast<size_t>(chunk.length) > file_size) break;

            if (chunk.type == 0x4E4F534A) {
                json_data = std::string(file_data.data() + offset, static_cast<size_t>(chunk.length));
            } else if (chunk.type == 0x004E4942) {
                bin_data = std::vector<char>(static_cast<char*>(file_data.data()) + offset, static_cast<char*>(file_data.data()) + offset + static_cast<size_t>(chunk.length));
            }
            offset += chunk.length;
        }
    } else {
        json_data = std::string(file_data.data(), file_size);
    }

    out_mesh = Mesh();
    out_materials.clear();

    size_t materials_pos = json_data.find("\"materials\"");
    if (materials_pos != std::string::npos) {
        size_t array_start = json_data.find('[', materials_pos);
        if (array_start != std::string::npos) {
            size_t array_end = json_data.find(']', array_start);
            if (array_end != std::string::npos) {
                std::string materials_str = json_data.substr(array_start + 1, array_end - array_start - 1);
                size_t search_pos = 0;
                while (true) {
                    size_t obj_start = materials_str.find('{', search_pos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = materials_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;

                    GLTFMaterial gltf_mat = ParseMaterial(materials_str, obj_start, obj_end);
                    Material mat;
                    mat.name = gltf_mat.name;
                    mat.diffuse = gltf_mat.base_color;
                    mat.ambient = gltf_mat.base_color * 0.2;
                    mat.specular = Vec3(0.0, 0.0, 0.0);
                    mat.shininess = 0.0;
                    mat.alpha = gltf_mat.alpha;
                    out_materials.push_back(mat);

                    search_pos = obj_end + 1;
                }
            }
        }
    }

    if (out_materials.empty()) {
        Material default_mat;
        default_mat.name = "default";
        default_mat.diffuse = Vec3(0.7, 0.7, 0.7);
        out_materials.push_back(default_mat);
    }

    size_t meshes_pos = json_data.find("\"meshes\"");
    if (meshes_pos == std::string::npos) {
        std::cerr << "No meshes found in GLTF" << std::endl;
        return false;
    }

    size_t mesh_array_start = json_data.find('[', meshes_pos);
    size_t mesh_array_end = json_data.find(']', mesh_array_start);
    std::string meshes_str = json_data.substr(mesh_array_start + 1, mesh_array_end - mesh_array_start - 1);

    size_t buffer_views_pos = json_data.find("\"bufferViews\"");
    size_t buffer_views_start = json_data.find('[', buffer_views_pos);
    size_t buffer_views_end = json_data.find(']', buffer_views_start);
    std::string buffer_views_str = json_data.substr(buffer_views_start + 1, buffer_views_end - buffer_views_start - 1);

    size_t accessors_pos = json_data.find("\"accessors\"");
    size_t accessors_start = json_data.find('[', accessors_pos);
    size_t accessors_end = json_data.find(']', accessors_start);
    std::string accessors_str = json_data.substr(accessors_start + 1, accessors_end - accessors_start - 1);

    size_t mesh_search = 0;
    while (true) {
        size_t mesh_start = meshes_str.find('{', mesh_search);
        if (mesh_start == std::string::npos) break;
        size_t mesh_end = meshes_str.find('}', mesh_start);
        if (mesh_end == std::string::npos) break;

        std::string mesh_str = meshes_str.substr(mesh_start, mesh_end - mesh_start + 1);

        size_t primitives_pos = mesh_str.find("\"primitives\"");
        if (primitives_pos == std::string::npos) {
            mesh_search = mesh_end + 1;
            continue;
        }

        size_t primitives_start = mesh_str.find('[', primitives_pos);
        size_t primitives_end = mesh_str.find(']', primitives_start);
        std::string primitives_str = mesh_str.substr(primitives_start + 1, primitives_end - primitives_start - 1);

        size_t prim_search = 0;
        while (true) {
            size_t prim_start = primitives_str.find('{', prim_search);
            if (prim_start == std::string::npos) break;
            size_t prim_end = primitives_str.find('}', prim_start);
            if (prim_end == std::string::npos) break;

            std::string prim_str = primitives_str.substr(prim_start, prim_end - prim_start + 1);

            int material_index = 0;
            size_t mat_pos = prim_str.find("\"material\"");
            if (mat_pos != std::string::npos) {
                size_t colon = prim_str.find(':', mat_pos);
                if (colon != std::string::npos) {
                    material_index = std::stoi(prim_str.substr(colon + 1));
                }
            }

            int pos_accessor = -1, normal_accessor = -1, uv_accessor = -1, index_accessor = -1;

            size_t attr_pos = prim_str.find("\"attributes\"");
            if (attr_pos != std::string::npos) {
                size_t pos_pos = prim_str.find("\"POSITION\"", attr_pos);
                if (pos_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', pos_pos);
                    if (colon != std::string::npos) pos_accessor = std::stoi(prim_str.substr(colon + 1));
                }
                size_t norm_pos = prim_str.find("\"NORMAL\"", attr_pos);
                if (norm_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', norm_pos);
                    if (colon != std::string::npos) normal_accessor = std::stoi(prim_str.substr(colon + 1));
                }
                size_t uv_pos = prim_str.find("\"TEXCOORD_0\"", attr_pos);
                if (uv_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', uv_pos);
                    if (colon != std::string::npos) uv_accessor = std::stoi(prim_str.substr(colon + 1));
                }
            }

            size_t indices_pos = prim_str.find("\"indices\"");
            if (indices_pos != std::string::npos) {
                size_t colon = prim_str.find(':', indices_pos);
                if (colon != std::string::npos) index_accessor = std::stoi(prim_str.substr(colon + 1));
            }

            size_t vertex_offset = out_mesh.vertices.size();

            if (pos_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == pos_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; ++i) {
                                size_t byte_pos = bv_offset + byte_offset + i * 12;
                                if (byte_pos + 12 <= bin_data.size()) {
                                    out_mesh.vertices.push_back(ReadVec3(bin_data.data(), byte_pos));
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            if (normal_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == normal_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; ++i) {
                                size_t byte_pos = bv_offset + byte_offset + i * 12;
                                if (byte_pos + 12 <= bin_data.size()) {
                                    out_mesh.normals.push_back(ReadVec3(bin_data.data(), byte_pos));
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            for (size_t i = out_mesh.uvs.size(); i < out_mesh.vertices.size() - vertex_offset; ++i) {
                out_mesh.uvs.emplace_back(0.0, 0.0);
            }

            Vec3 mesh_color = out_materials.empty() ? Vec3(0.7, 0.7, 0.7) : out_materials[std::min(material_index, (int)out_materials.size() - 1)].diffuse;
            for (size_t i = 0; i < out_mesh.vertices.size() - vertex_offset; ++i) {
                out_mesh.colors.push_back(mesh_color);
            }

            if (index_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == index_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; i += 3) {
                                size_t byte_pos = bv_offset + byte_offset + i * 4;
                                if (byte_pos + 12 <= bin_data.size()) {
                                    uint32_t i0 = *reinterpret_cast<const uint32_t*>(bin_data.data() + byte_pos);
                                    uint32_t i1 = *reinterpret_cast<const uint32_t*>(bin_data.data() + byte_pos + 4);
                                    uint32_t i2 = *reinterpret_cast<const uint32_t*>(bin_data.data() + byte_pos + 8);
                                    out_mesh.indices.push_back(static_cast<int>(vertex_offset + i0));
                                    out_mesh.indices.push_back(static_cast<int>(vertex_offset + i1));
                                    out_mesh.indices.push_back(static_cast<int>(vertex_offset + i2));
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            prim_search = prim_end + 1;
        }

        mesh_search = mesh_end + 1;
    }

    if (!out_materials.empty()) {
        out_mesh.material = out_materials[0].name;
    }

    std::cout << "Loaded GLTF: " << filename << std::endl;
    std::cout << "  Vertices: " << out_mesh.vertices.size() << std::endl;
    std::cout << "  Triangles: " << out_mesh.indices.size() / 3 << std::endl;
    std::cout << "  Materials: " << out_materials.size() << std::endl;

    return true;
}

bool GLTFLoader::LoadSkinned(const std::string& filename, GLTFSkinnedMesh& out_mesh) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open GLTF file: " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> file_data(file_size);
    file.read(file_data.data(), file_size);
    file.close();

    std::string json_data;
    std::vector<char> bin_data;

    if (file_size >= 4 && std::string(file_data.data(), 4) == "glTF") {
        if (file_size < sizeof(GLBHeader)) return false;

        GLBHeader header;
        std::memcpy(&header, file_data.data(), sizeof(GLBHeader));

        size_t offset = sizeof(GLBHeader);
        while (offset + sizeof(GLBChunkHeader) <= file_size) {
            GLBChunkHeader chunk;
            std::memcpy(&chunk, file_data.data() + offset, sizeof(GLBChunkHeader));
            offset += sizeof(GLBChunkHeader);

            if (offset + static_cast<size_t>(chunk.length) > file_size) break;

            if (chunk.type == 0x4E4F534A) {
                json_data = std::string(file_data.data() + offset, static_cast<size_t>(chunk.length));
            } else if (chunk.type == 0x004E4942) {
                bin_data = std::vector<char>(static_cast<char*>(file_data.data()) + offset, static_cast<char*>(file_data.data()) + offset + static_cast<size_t>(chunk.length));
            }
            offset += chunk.length;
        }
    } else {
        json_data = std::string(file_data.data(), file_size);
    }

    out_mesh = GLTFSkinnedMesh();

    size_t meshes_pos = json_data.find("\"meshes\"");
    if (meshes_pos == std::string::npos) {
        std::cerr << "No meshes found in GLTF" << std::endl;
        return false;
    }

    size_t mesh_array_start = json_data.find('[', meshes_pos);
    size_t mesh_array_end = json_data.find(']', mesh_array_start);
    std::string meshes_str = json_data.substr(mesh_array_start + 1, mesh_array_end - mesh_array_start - 1);

    size_t buffer_views_pos = json_data.find("\"bufferViews\"");
    size_t buffer_views_start = json_data.find('[', buffer_views_pos);
    size_t buffer_views_end = json_data.find(']', buffer_views_start);
    std::string buffer_views_str = json_data.substr(buffer_views_start + 1, buffer_views_end - buffer_views_start - 1);

    size_t accessors_pos = json_data.find("\"accessors\"");
    size_t accessors_start = json_data.find('[', accessors_pos);
    size_t accessors_end = json_data.find(']', accessors_start);
    std::string accessors_str = json_data.substr(accessors_start + 1, accessors_end - accessors_start - 1);

    size_t mesh_search = 0;
    while (true) {
        size_t mesh_start = meshes_str.find('{', mesh_search);
        if (mesh_start == std::string::npos) break;
        size_t mesh_end = meshes_str.find('}', mesh_start);
        if (mesh_end == std::string::npos) break;

        std::string mesh_str = meshes_str.substr(mesh_start, mesh_end - mesh_start + 1);

        size_t primitives_pos = mesh_str.find("\"primitives\"");
        if (primitives_pos == std::string::npos) {
            mesh_search = mesh_end + 1;
            continue;
        }

        size_t primitives_start = mesh_str.find('[', primitives_pos);
        size_t primitives_end = mesh_str.find(']', primitives_start);
        std::string primitives_str = mesh_str.substr(primitives_start + 1, primitives_end - primitives_start - 1);

        size_t prim_search = 0;
        while (true) {
            size_t prim_start = primitives_str.find('{', prim_search);
            if (prim_start == std::string::npos) break;
            size_t prim_end = primitives_str.find('}', prim_start);
            if (prim_end == std::string::npos) break;

            std::string prim_str = primitives_str.substr(prim_start, prim_end - prim_start + 1);

            int pos_accessor = -1, normal_accessor = -1, uv_accessor = -1, index_accessor = -1;
            int joints_accessor = -1, weights_accessor = -1;

            size_t attr_pos = prim_str.find("\"attributes\"");
            if (attr_pos != std::string::npos) {
                size_t pos_pos = prim_str.find("\"POSITION\"", attr_pos);
                if (pos_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', pos_pos);
                    if (colon != std::string::npos) pos_accessor = std::stoi(prim_str.substr(colon + 1));
                }
                size_t norm_pos = prim_str.find("\"NORMAL\"", attr_pos);
                if (norm_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', norm_pos);
                    if (colon != std::string::npos) normal_accessor = std::stoi(prim_str.substr(colon + 1));
                }
                size_t uv_pos = prim_str.find("\"TEXCOORD_0\"", attr_pos);
                if (uv_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', uv_pos);
                    if (colon != std::string::npos) uv_accessor = std::stoi(prim_str.substr(colon + 1));
                }
                size_t joints_pos = prim_str.find("\"JOINTS_0\"", attr_pos);
                if (joints_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', joints_pos);
                    if (colon != std::string::npos) joints_accessor = std::stoi(prim_str.substr(colon + 1));
                }
                size_t weights_pos = prim_str.find("\"WEIGHTS_0\"", attr_pos);
                if (weights_pos != std::string::npos) {
                    size_t colon = prim_str.find(':', weights_pos);
                    if (colon != std::string::npos) weights_accessor = std::stoi(prim_str.substr(colon + 1));
                }
            }

            size_t indices_pos = prim_str.find("\"indices\"");
            if (indices_pos != std::string::npos) {
                size_t colon = prim_str.find(':', indices_pos);
                if (colon != std::string::npos) index_accessor = std::stoi(prim_str.substr(colon + 1));
            }

            size_t vertex_offset = out_mesh.positions.size() / 3;

            if (pos_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == pos_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; ++i) {
                                size_t byte_pos = bv_offset + byte_offset + i * 12;
                                if (byte_pos + 12 <= bin_data.size()) {
                                    Vec3 v = ReadVec3(bin_data.data(), byte_pos);
                                    out_mesh.positions.push_back(v.x());
                                    out_mesh.positions.push_back(v.y());
                                    out_mesh.positions.push_back(v.z());
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            if (normal_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == normal_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; ++i) {
                                size_t byte_pos = bv_offset + byte_offset + i * 12;
                                if (byte_pos + 12 <= bin_data.size()) {
                                    Vec3 n = ReadVec3(bin_data.data(), byte_pos);
                                    out_mesh.normals.push_back(n.x());
                                    out_mesh.normals.push_back(n.y());
                                    out_mesh.normals.push_back(n.z());
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            for (size_t i = out_mesh.uvs.size(); i < (out_mesh.positions.size() / 3 - vertex_offset) * 2; ++i) {
                out_mesh.uvs.push_back(0.0f);
            }

            if (joints_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == joints_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; ++i) {
                                for (int j = 0; j < 4; ++j) {
                                    size_t byte_pos = bv_offset + byte_offset + (i * 4 + j) * 2;
                                    if (byte_pos + 2 <= bin_data.size()) {
                                        uint16_t joint = *reinterpret_cast<const uint16_t*>(bin_data.data() + byte_pos);
                                        out_mesh.bone_ids.push_back(static_cast<int>(joint));
                                    } else {
                                        out_mesh.bone_ids.push_back(0);
                                    }
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            if (weights_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == weights_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; ++i) {
                                for (int j = 0; j < 4; ++j) {
                                    size_t byte_pos = bv_offset + byte_offset + (i * 4 + j) * 4;
                                    if (byte_pos + 4 <= bin_data.size()) {
                                        float weight = *reinterpret_cast<const float*>(bin_data.data() + byte_pos);
                                        out_mesh.bone_weights.push_back(weight);
                                    } else {
                                        out_mesh.bone_weights.push_back(0.0f);
                                    }
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            if (out_mesh.bone_ids.size() < (out_mesh.positions.size() / 3) * 4) {
                size_t existing = out_mesh.bone_ids.size() / 4;
                for (size_t i = existing; i < out_mesh.positions.size() / 3; ++i) {
                    out_mesh.bone_ids.push_back(0);
                    out_mesh.bone_ids.push_back(0);
                    out_mesh.bone_ids.push_back(0);
                    out_mesh.bone_ids.push_back(0);
                    out_mesh.bone_weights.push_back(1.0f);
                    out_mesh.bone_weights.push_back(0.0f);
                    out_mesh.bone_weights.push_back(0.0f);
                    out_mesh.bone_weights.push_back(0.0f);
                }
            }

            if (index_accessor >= 0 && !bin_data.empty()) {
                size_t apos = 0;
                int current = 0;
                while (true) {
                    size_t obj_start = accessors_str.find('{', apos);
                    if (obj_start == std::string::npos) break;
                    size_t obj_end = accessors_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    if (current == index_accessor) {
                        size_t count_pos = json_data.find("\"count\"", obj_start);
                        int count = 0;
                        if (count_pos != std::string::npos && count_pos < obj_end) {
                            size_t colon = json_data.find(':', count_pos);
                            if (colon != std::string::npos) count = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t bv_pos = json_data.find("\"bufferView\"", obj_start);
                        int bv_idx = -1;
                        if (bv_pos != std::string::npos && bv_pos < obj_end) {
                            size_t colon = json_data.find(':', bv_pos);
                            if (colon != std::string::npos) bv_idx = std::stoi(json_data.substr(colon + 1));
                        }

                        size_t byte_offset_pos = json_data.find("\"byteOffset\"", obj_start);
                        int byte_offset = 0;
                        if (byte_offset_pos != std::string::npos && byte_offset_pos < obj_end) {
                            size_t colon = json_data.find(':', byte_offset_pos);
                            if (colon != std::string::npos) byte_offset = std::stoi(json_data.substr(colon + 1));
                        }

                        if (bv_idx >= 0) {
                            size_t bv_offset = GetBufferViewByteOffset(buffer_views_str, bv_idx);
                            for (int i = 0; i < count; i += 3) {
                                size_t byte_pos = bv_offset + byte_offset + i * 4;
                                if (byte_pos + 12 <= bin_data.size()) {
                                    uint32_t i0 = *reinterpret_cast<const uint32_t*>(bin_data.data() + byte_pos);
                                    uint32_t i1 = *reinterpret_cast<const uint32_t*>(bin_data.data() + byte_pos + 4);
                                    uint32_t i2 = *reinterpret_cast<const uint32_t*>(bin_data.data() + byte_pos + 8);
                                    out_mesh.indices.push_back(static_cast<int>(vertex_offset + i0));
                                    out_mesh.indices.push_back(static_cast<int>(vertex_offset + i1));
                                    out_mesh.indices.push_back(static_cast<int>(vertex_offset + i2));
                                }
                            }
                        }
                        break;
                    }
                    apos = obj_end + 1;
                    current++;
                }
            }

            prim_search = prim_end + 1;
        }

        mesh_search = mesh_end + 1;
    }

    size_t nodes_pos = json_data.find("\"nodes\"");
    if (nodes_pos != std::string::npos) {
        size_t nodes_start = json_data.find('[', nodes_pos);
        size_t nodes_end = json_data.find(']', nodes_start);
        std::string nodes_str = json_data.substr(nodes_start + 1, nodes_end - nodes_start - 1);

        size_t node_search = 0;
        while (true) {
            size_t node_start = nodes_str.find('{', node_search);
            if (node_start == std::string::npos) break;
            size_t node_end = nodes_str.find('}', node_start);
            if (node_end == std::string::npos) break;

            std::string node_str = nodes_str.substr(node_start, node_end - node_start + 1);
            GLTFBone bone;
            bone.parent_index = -1;

            size_t name_pos = node_str.find("\"name\"");
            if (name_pos != std::string::npos) {
                size_t colon = node_str.find(':', name_pos);
                if (colon != std::string::npos) {
                    size_t q1 = node_str.find('"', colon);
                    if (q1 != std::string::npos) {
                        size_t q2 = node_str.find('"', q1 + 1);
                        if (q2 != std::string::npos) bone.name = node_str.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
            }

            size_t children_pos = node_str.find("\"children\"");
            if (children_pos != std::string::npos) {
                size_t bracket = node_str.find('[', children_pos);
                if (bracket != std::string::npos) {
                    size_t end_bracket = node_str.find(']', bracket);
                    if (end_bracket != std::string::npos) {
                        std::string children_str = node_str.substr(bracket + 1, end_bracket - bracket - 1);
                        for (size_t i = 0; i < children_str.size(); ++i) {
                            if (children_str[i] >= '0' && children_str[i] <= '9') {
                                int child_idx = std::stoi(children_str.substr(i));
                                if (child_idx >= 0 && child_idx < (int)out_mesh.bones.size()) {
                                    out_mesh.bones[child_idx].parent_index = out_mesh.bones.size();
                                }
                                break;
                            }
                        }
                    }
                }
            }

            size_t matrix_pos = node_str.find("\"matrix\"");
            if (matrix_pos != std::string::npos && matrix_pos < node_end) {
                size_t bracket = node_str.find('[', matrix_pos);
                if (bracket != std::string::npos && bracket < node_end) {
                    size_t end_bracket = node_str.find(']', bracket);
                    if (end_bracket != std::string::npos) {
                        std::string matrix_str = node_str.substr(bracket + 1, end_bracket - bracket - 1);
                        std::istringstream iss(matrix_str);
                        for (int i = 0; i < 16; ++i) {
                            float val;
                            if (iss >> val) {
                                bone.local_transform.push_back(val);
                            } else {
                                bone.local_transform.push_back(0.0f);
                            }
                        }
                    }
                }
            }

            if (bone.local_transform.empty()) {
                bone.local_transform = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            }
            bone.offset_matrix = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

            out_mesh.bones.push_back(bone);
            node_search = node_end + 1;
        }
    }

    size_t anims_pos = json_data.find("\"animations\"");
    if (anims_pos != std::string::npos) {
        size_t anims_start = json_data.find('[', anims_pos);
        size_t anims_end = json_data.find(']', anims_start);
        std::string anims_str = json_data.substr(anims_start + 1, anims_end - anims_start - 1);

        size_t anim_search = 0;
        while (true) {
            size_t anim_start = anims_str.find('{', anim_search);
            if (anim_start == std::string::npos) break;
            size_t anim_end = anims_str.find('}', anim_start);
            if (anim_end == std::string::npos) break;

            std::string anim_str = anims_str.substr(anim_start, anim_end - anim_start + 1);
            GLTFAnimation anim;

            size_t name_pos = anim_str.find("\"name\"");
            if (name_pos != std::string::npos) {
                size_t colon = anim_str.find(':', name_pos);
                if (colon != std::string::npos) {
                    size_t q1 = anim_str.find('"', colon);
                    if (q1 != std::string::npos) {
                        size_t q2 = anim_str.find('"', q1 + 1);
                        if (q2 != std::string::npos) anim.name = anim_str.substr(q1 + 1, q2 - q1 - 1);
                    }
                }
            }

            size_t duration_pos = anim_str.find("\"duration\"");
            if (duration_pos != std::string::npos) {
                size_t colon = anim_str.find(':', duration_pos);
                if (colon != std::string::npos) {
                    anim.duration_seconds = std::stof(anim_str.substr(colon + 1));
                }
            }

            size_t channels_pos = anim_str.find("\"channels\"");
            if (channels_pos != std::string::npos) {
                size_t channels_start = anim_str.find('[', channels_pos);
                size_t channels_end = anim_str.find(']', channels_start);
                std::string channels_str = anim_str.substr(channels_start + 1, channels_end - channels_start - 1);

                size_t ch_search = 0;
                while (true) {
                    size_t ch_start = channels_str.find('{', ch_search);
                    if (ch_start == std::string::npos) break;
                    size_t ch_end = channels_str.find('}', ch_start);
                    if (ch_end == std::string::npos) break;

                    std::string ch_str = channels_str.substr(ch_start, ch_end - ch_start + 1);
                    GLTFTrack track;

                    size_t node_pos = ch_str.find("\"node\"");
                    if (node_pos != std::string::npos) {
                        size_t colon = ch_str.find(':', node_pos);
                        if (colon != std::string::npos) {
                            int node_idx = std::stoi(ch_str.substr(colon + 1));
                            if (node_idx >= 0 && node_idx < (int)out_mesh.bones.size()) {
                                track.bone_name = out_mesh.bones[node_idx].name;
                            }
                        }
                    }

                    size_t path_pos = ch_str.find("\"path\"");
                    if (path_pos != std::string::npos) {
                        size_t colon = ch_str.find(':', path_pos);
                        if (colon != std::string::npos) {
                            size_t q1 = ch_str.find('"', colon);
                            if (q1 != std::string::npos) {
                                size_t q2 = ch_str.find('"', q1 + 1);
                                if (q2 != std::string::npos) {
                                    std::string path = ch_str.substr(q1 + 1, q2 - q1 - 1);
                                if (path == "translation") {
                                    track.positions.reserve(10);
                                } else if (path == "rotation") {
                                    track.rotations.reserve(10);
                                } else if (path == "scale") {
                                    track.scales.reserve(10);
                                }
                                }
                            }
                        }
                    }

                    anim.tracks.push_back(track);
                    ch_search = ch_end + 1;
                }
            }

            out_mesh.animations.push_back(anim);
            anim_search = anim_end + 1;
        }
    }

    std::cout << "Loaded skinned GLTF: " << filename << std::endl;
    std::cout << "  Vertices: " << out_mesh.positions.size() / 3 << std::endl;
    std::cout << "  Triangles: " << out_mesh.indices.size() / 3 << std::endl;
    std::cout << "  Bones: " << out_mesh.bones.size() << std::endl;
    std::cout << "  Animations: " << out_mesh.animations.size() << std::endl;

    return true;
}

}