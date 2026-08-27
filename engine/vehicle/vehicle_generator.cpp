#include "vehicle_generator.h"
#include <cmath>

// Project 0 — procedural vehicle mesh generator implementation
// Builds a simple sports-car shape from box/cylinder primitives.
namespace p0::vehicle {

// Derive geometry dimensions from physical vehicle parameters.
VehicleGeometry VehicleGenerator::FromParams(const VehicleParams& params) {
    VehicleGeometry geo;
    geo.wheelbase = params.wheelbase;
    geo.track_width = params.track_width;
    geo.wheel_radius = params.wheel_radius;
    geo.ride_height = params.ride_height;

    // Derive body dimensions from wheelbase and track
    geo.body_length = params.wheelbase + params.cg_to_front + params.cg_to_rear;
    geo.body_width = params.track_width + 0.25; // Slightly wider than track
    geo.body_height = 1.15;
    geo.front_overhang = params.cg_to_front;
    geo.rear_overhang = params.cg_to_rear;

    // Cabin dimensions
    geo.cabin_length = params.wheelbase * 0.7;
    geo.cabin_width = params.track_width * 0.85;
    geo.cabin_height = 0.55;

    // Aero elements
    geo.spoiler_height = params.rear_wing_area * 2.0;
    geo.spoiler_width = params.track_width * 0.85;
    geo.splitter_depth = params.front_wing_area * 1.5;

    // Wheel width based on tire specs
    geo.wheel_width = 0.25;

    return geo;
}

// Assemble a complete car mesh from all body and component parts.
MeshData VehicleGenerator::GenerateCar(const VehicleGeometry& geo) {
    MeshData car;

    auto body = GenerateBody(geo);
    auto cabin = GenerateCabin(geo);
    auto spoiler = GenerateSpoiler(geo);
    auto splitter = GenerateSplitter(geo);
    auto mirrors = GenerateSideMirrors(geo);
    auto headlights = GenerateHeadlights(geo);
    auto taillights = GenerateTaillights(geo);

    MergeMesh(car, body);
    MergeMesh(car, cabin);
    MergeMesh(car, spoiler);
    MergeMesh(car, splitter);
    MergeMesh(car, mirrors);
    MergeMesh(car, headlights);
    MergeMesh(car, taillights);

    // Position and merge the four wheels at the correct axle locations.
    // Generate 4 wheels
    double wheel_y = geo.ride_height + geo.wheel_radius;
    double front_x = geo.wheelbase * 0.5 - geo.front_overhang + geo.front_overhang;
    double rear_x = -(geo.wheelbase * 0.5 - geo.rear_overhang + geo.rear_overhang);
    double trackOffset = geo.track_width * 0.5;

    // Front left wheel
    auto flWheel = GenerateWheel(geo);
    for (auto& v : flWheel.vertices) {
        v[0] += front_x;
        v[1] += wheel_y;
        v[2] += trackOffset;
    }
    for (auto& n : flWheel.normals) {
        // No rotation needed for left side
    }
    MergeMesh(car, flWheel);

    // Front right wheel
    auto frWheel = GenerateWheel(geo);
    for (auto& v : frWheel.vertices) {
        v[0] += front_x;
        v[1] += wheel_y;
        v[2] -= trackOffset;
    }
    MergeMesh(car, frWheel);

    // Rear left wheel
    auto rlWheel = GenerateWheel(geo);
    for (auto& v : rlWheel.vertices) {
        v[0] += rear_x;
        v[1] += wheel_y;
        v[2] += trackOffset;
    }
    MergeMesh(car, rlWheel);

    // Rear right wheel
    auto rrWheel = GenerateWheel(geo);
    for (auto& v : rrWheel.vertices) {
        v[0] += rear_x;
        v[1] += wheel_y;
        v[2] -= trackOffset;
    }
    MergeMesh(car, rrWheel);

    return car;
}

// Build the main body: lower section, hood and rear engine cover.
MeshData VehicleGenerator::GenerateBody(const VehicleGeometry& geo) {
    MeshData body;

    // Main body - lower section
    Vec3 lowerCenter(0, geo.ride_height + geo.wheel_radius + geo.body_height * 0.25, 0);
    Vec3 lowerSize(geo.body_length, geo.body_height * 0.5, geo.body_width);
    AddBox(body, lowerCenter, lowerSize);

    // Hood (sloped)
    double hoodZ = geo.body_width * 0.45;
    double hoodHeight = geo.body_height * 0.5 + geo.hood_slope * geo.front_overhang;
    Vec3 hoodCenter(geo.wheelbase * 0.3, geo.ride_height + geo.wheel_radius + hoodHeight * 0.5, 0);
    Vec3 hoodSize(geo.front_overhang + geo.wheelbase * 0.4, hoodHeight, hoodZ * 2);
    AddBox(body, hoodCenter, hoodSize);

    // Rear section (engine cover)
    double rearHeight = geo.body_height * 0.55;
    Vec3 rearCenter(-geo.wheelbase * 0.35, geo.ride_height + geo.wheel_radius + rearHeight * 0.5, 0);
    Vec3 rearSize(geo.rear_overhang + geo.wheelbase * 0.3, rearHeight, geo.body_width * 0.9);
    AddBox(body, rearCenter, rearSize);

    // Side skirts
    double skirtHeight = geo.body_height * 0.2;
    double skirtY = geo.ride_height + geo.wheel_radius + skirtHeight * 0.5;
    Vec3 skirtSize(geo.body_length * 0.7, skirtHeight, 0.1);

    Vec3 skirtLeft(0, skirtY, geo.body_width * 0.5);
    AddBox(body, skirtLeft, skirtSize);

    Vec3 skirtRight(0, skirtY, -geo.body_width * 0.5);
    AddBox(body, skirtRight, skirtSize);

    return body;
}

// Build the passenger cabin with roof and A/C pillars.
MeshData VehicleGenerator::GenerateCabin(const VehicleGeometry& geo) {
    MeshData cabin;

    // Cabin base
    double cabinY = geo.ride_height + geo.wheel_radius + geo.body_height * 0.5 + geo.cabin_height * 0.5;
    Vec3 cabinCenter(0, cabinY, 0);
    Vec3 cabinSize(geo.cabin_length, geo.cabin_height, geo.cabin_width);
    AddBox(cabin, cabinCenter, cabinSize);

    // Roof
    double roofY = geo.ride_height + geo.wheel_radius + geo.body_height * 0.5 + geo.cabin_height;
    Vec3 roofCenter(0, roofY, 0);
    Vec3 roofSize(geo.cabin_length * 0.85, 0.05, geo.cabin_width * 0.95);
    AddBox(cabin, roofCenter, roofSize);

    // A-pillars (windshield frame)
    double pillarHeight = geo.cabin_height * 0.8;
    double pillarWidth = 0.08;
    Vec3 pillarSize(pillarWidth, pillarHeight, pillarWidth);

    double aPillarX = geo.cabin_length * 0.35;
    double aPillarZ = geo.cabin_width * 0.45;
    double aPillarY = geo.ride_height + geo.wheel_radius + geo.body_height * 0.5 + pillarHeight * 0.5;

    Vec3 flPillar(aPillarX, aPillarY, aPillarZ);
    Vec3 frPillar(aPillarX, aPillarY, -aPillarZ);
    AddBox(cabin, flPillar, pillarSize);
    AddBox(cabin, frPillar, pillarSize);

    // C-pillars (rear window frame)
    double cPillarX = -geo.cabin_length * 0.35;
    Vec3 rlPillar(cPillarX, aPillarY, aPillarZ);
    Vec3 rrPillar(cPillarX, aPillarY, -aPillarZ);
    AddBox(cabin, rlPillar, pillarSize);
    AddBox(cabin, rrPillar, pillarSize);

    return cabin;
}

// Build a single wheel: tire and rim as concentric cylinders.
MeshData VehicleGenerator::GenerateWheel(const VehicleGeometry& geo) {
    MeshData wheel;

    // Tire (cylinder)
    Vec3 tireCenter(0, 0, 0);
    AddCylinder(wheel, tireCenter, geo.wheel_radius, geo.wheel_width, 16);

    // Rim (smaller cylinder)
    Vec3 rimCenter(0, 0, 0);
    AddCylinder(wheel, rimCenter, geo.wheel_radius * 0.6, geo.wheel_width * 1.1, 8);

    return wheel;
}

// Build the rear spoiler blade and support struts.
MeshData VehicleGenerator::GenerateSpoiler(const VehicleGeometry& geo) {
    MeshData spoiler;

    // Spoiler blade
    double spoilerY = geo.ride_height + geo.wheel_radius + geo.body_height + geo.spoiler_height;
    double spoilerX = -geo.body_length * 0.45;

    Vec3 bladeCenter(spoilerX, spoilerY, 0);
    Vec3 bladeSize(geo.spoiler_thickness, 0.15, geo.spoiler_width);
    AddBox(spoiler, bladeCenter, bladeSize);

    // Spoiler supports
    Vec3 supportSize(0.08, geo.spoiler_height, 0.08);
    double supportZ = geo.spoiler_width * 0.4;

    Vec3 leftSupport(spoilerX, spoilerY - geo.spoiler_height * 0.4, supportZ);
    Vec3 rightSupport(spoilerX, spoilerY - geo.spoiler_height * 0.4, -supportZ);
    AddBox(spoiler, leftSupport, supportSize);
    AddBox(spoiler, rightSupport, supportSize);

    return spoiler;
}

// Build the front splitter/diffuser lip.
MeshData VehicleGenerator::GenerateSplitter(const VehicleGeometry& geo) {
    MeshData splitter;

    double splitterY = geo.ride_height + geo.wheel_radius - geo.splitter_height;
    double splitterX = geo.body_length * 0.5;

    Vec3 splitterCenter(splitterX, splitterY, 0);
    Vec3 splitterSize(geo.splitter_depth, geo.splitter_height, geo.body_width * 1.1);
    AddBox(splitter, splitterCenter, splitterSize);

    return splitter;
}

// Build side mirrors with stalks.
MeshData VehicleGenerator::GenerateSideMirrors(const VehicleGeometry& geo) {
    MeshData mirrors;

    double mirrorY = geo.ride_height + geo.wheel_radius + geo.body_height * 0.6;
    double mirrorX = geo.cabin_length * 0.2;
    double mirrorZ = geo.body_width * 0.55;

    Vec3 mirrorSize(0.15, 0.1, 0.2);

    Vec3 leftMirror(mirrorX, mirrorY, mirrorZ);
    Vec3 rightMirror(mirrorX, mirrorY, -mirrorZ);
    AddBox(mirrors, leftMirror, mirrorSize);
    AddBox(mirrors, rightMirror, mirrorSize);

    // Mirror stalks
    Vec3 stalkSize(0.04, 0.04, 0.1);
    Vec3 leftStalk(mirrorX, mirrorY, mirrorZ - 0.15);
    Vec3 rightStalk(mirrorX, mirrorY, -mirrorZ + 0.15);
    AddBox(mirrors, leftStalk, stalkSize);
    AddBox(mirrors, rightStalk, stalkSize);

    return mirrors;
}

// Build front headlight blocks.
MeshData VehicleGenerator::GenerateHeadlights(const VehicleGeometry& geo) {
    MeshData lights;

    double lightY = geo.ride_height + geo.wheel_radius + geo.body_height * 0.4;
    double lightX = geo.body_length * 0.48;
    double lightZ = geo.body_width * 0.35;

    Vec3 lightSize(0.1, 0.15, 0.25);

    Vec3 leftLight(lightX, lightY, lightZ);
    Vec3 rightLight(lightX, lightY, -lightZ);
    AddBox(lights, leftLight, lightSize);
    AddBox(lights, rightLight, lightSize);

    return lights;
}

// Build rear taillight blocks.
MeshData VehicleGenerator::GenerateTaillights(const VehicleGeometry& geo) {
    MeshData lights;

    double lightY = geo.ride_height + geo.wheel_radius + geo.body_height * 0.45;
    double lightX = -geo.body_length * 0.48;
    double lightZ = geo.body_width * 0.3;

    Vec3 lightSize(0.08, 0.2, 0.3);

    Vec3 leftLight(lightX, lightY, lightZ);
    Vec3 rightLight(lightX, lightY, -lightZ);
    AddBox(lights, leftLight, lightSize);
    AddBox(lights, rightLight, lightSize);

    return lights;
}

// Append an axis-aligned box primitive to the mesh.
void VehicleGenerator::AddBox(MeshData& mesh, const Vec3& center, const Vec3& size) {
    double hx = size[0] * 0.5;
    double hy = size[1] * 0.5;
    double hz = size[2] * 0.5;

    Vec3 v000(center[0] - hx, center[1] - hy, center[2] - hz);
    Vec3 v100(center[0] + hx, center[1] - hy, center[2] - hz);
    Vec3 v110(center[0] + hx, center[1] + hy, center[2] - hz);
    Vec3 v010(center[0] - hx, center[1] + hy, center[2] - hz);
    Vec3 v001(center[0] - hx, center[1] - hy, center[2] + hz);
    Vec3 v101(center[0] + hx, center[1] - hy, center[2] + hz);
    Vec3 v111(center[0] + hx, center[1] + hy, center[2] + hz);
    Vec3 v011(center[0] - hx, center[1] + hy, center[2] + hz);

    int baseIndex = mesh.vertices.size();

    mesh.vertices.push_back(v000);
    mesh.vertices.push_back(v100);
    mesh.vertices.push_back(v110);
    mesh.vertices.push_back(v010);
    mesh.vertices.push_back(v001);
    mesh.vertices.push_back(v101);
    mesh.vertices.push_back(v111);
    mesh.vertices.push_back(v011);

    for (int i = 0; i < 8; ++i) {
        mesh.normals.push_back(Vec3(0, 1, 0));
        mesh.colors.push_back(Vec3(1.0, 1.0, 1.0));
    }

    for (int i = 0; i < 24; ++i) {
        mesh.uvs.push_back(Vec2(0, 0));
    }

    // Front face
    mesh.indices.push_back(baseIndex + 0);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 2);
    mesh.indices.push_back(baseIndex + 0);
    mesh.indices.push_back(baseIndex + 2);
    mesh.indices.push_back(baseIndex + 3);

    // Back face
    mesh.indices.push_back(baseIndex + 5);
    mesh.indices.push_back(baseIndex + 4);
    mesh.indices.push_back(baseIndex + 7);
    mesh.indices.push_back(baseIndex + 5);
    mesh.indices.push_back(baseIndex + 7);
    mesh.indices.push_back(baseIndex + 6);

    // Top face
    mesh.indices.push_back(baseIndex + 3);
    mesh.indices.push_back(baseIndex + 2);
    mesh.indices.push_back(baseIndex + 6);
    mesh.indices.push_back(baseIndex + 3);
    mesh.indices.push_back(baseIndex + 6);
    mesh.indices.push_back(baseIndex + 7);

    // Bottom face
    mesh.indices.push_back(baseIndex + 4);
    mesh.indices.push_back(baseIndex + 5);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 4);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 0);

    // Right face
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 5);
    mesh.indices.push_back(baseIndex + 6);
    mesh.indices.push_back(baseIndex + 1);
    mesh.indices.push_back(baseIndex + 6);
    mesh.indices.push_back(baseIndex + 2);

    // Left face
    mesh.indices.push_back(baseIndex + 4);
    mesh.indices.push_back(baseIndex + 0);
    mesh.indices.push_back(baseIndex + 3);
    mesh.indices.push_back(baseIndex + 4);
    mesh.indices.push_back(baseIndex + 3);
    mesh.indices.push_back(baseIndex + 7);
}

// Append a cylinder primitive (approximated by segments) to the mesh.
void VehicleGenerator::AddCylinder(MeshData& mesh, const Vec3& center, double radius, double height, int segments) {
    int baseIndex = mesh.vertices.size();
    double halfHeight = height * 0.5;

    // Create vertices for top and bottom circles
    for (int i = 0; i < segments; i++) {
        double angle = 2.0 * kPi * i / segments;
        double x = radius * cos(angle);
        double z = radius * sin(angle);

        mesh.vertices.push_back(Vec3(center[0] + x, center[1] - halfHeight, center[2] + z));
        mesh.normals.push_back(Vec3(cos(angle), 0, sin(angle)));
        mesh.colors.push_back(Vec3(1.0, 1.0, 1.0));
        mesh.uvs.push_back(Vec2((double)i / segments, 0));

        mesh.vertices.push_back(Vec3(center[0] + x, center[1] + halfHeight, center[2] + z));
        mesh.normals.push_back(Vec3(cos(angle), 0, sin(angle)));
        mesh.colors.push_back(Vec3(1.0, 1.0, 1.0));
        mesh.uvs.push_back(Vec2((double)i / segments, 1));
    }

    // Bottom center
    mesh.vertices.push_back(Vec3(center[0], center[1] - halfHeight, center[2]));
    mesh.normals.push_back(Vec3(0, -1, 0));
    mesh.colors.push_back(Vec3(1.0, 1.0, 1.0));
    mesh.uvs.push_back(Vec2(0.5, 0.5));

    // Top center
    mesh.vertices.push_back(Vec3(center[0], center[1] + halfHeight, center[2]));
    mesh.normals.push_back(Vec3(0, 1, 0));
    mesh.colors.push_back(Vec3(1.0, 1.0, 1.0));
    mesh.uvs.push_back(Vec2(0.5, 0.5));

    int capBaseIndex = mesh.vertices.size() - 2;

    // Create side faces
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;
        int bl = baseIndex + i * 2;      // bottom left
        int tl = baseIndex + i * 2 + 1;  // top left
        int br = baseIndex + next * 2;   // bottom right
        int tr = baseIndex + next * 2 + 1; // top right

        mesh.indices.push_back(bl);
        mesh.indices.push_back(br);
        mesh.indices.push_back(tl);

        mesh.indices.push_back(tl);
        mesh.indices.push_back(br);
        mesh.indices.push_back(tr);
    }

    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;

        // Bottom cap
        mesh.indices.push_back(capBaseIndex);
        mesh.indices.push_back(capBaseIndex + 2 + next * 2);
        mesh.indices.push_back(capBaseIndex + 2 + i * 2);

        // Top cap
        mesh.indices.push_back(capBaseIndex + 1);
        mesh.indices.push_back(capBaseIndex + 2 + i * 2 + 1);
        mesh.indices.push_back(capBaseIndex + 2 + next * 2 + 1);
    }
}

// Merge source mesh vertices/indices into target mesh, reindexing indices.
void VehicleGenerator::MergeMesh(MeshData& target, const MeshData& source) {
    int baseIndex = target.vertices.size();

    for (const auto& v : source.vertices) {
        target.vertices.push_back(v);
    }
    for (const auto& n : source.normals) {
        target.normals.push_back(n);
    }
    for (const auto& uv : source.uvs) {
        target.uvs.push_back(uv);
    }
    for (const auto& c : source.colors) {
        target.colors.push_back(c);
    }
    for (const auto& idx : source.indices) {
        target.indices.push_back(baseIndex + idx);
    }
}

} // namespace p0::vehicle
