#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        double x;
        double y;
        double vx;
        double vy;
        double heading;
        double speed;
        double rpm;
        int gear;
        double throttle;
        double brake;
        double steer;
    } VehicleState;

    __declspec(dllexport) int SimPlugin_Initialize();
    __declspec(dllexport) void SimPlugin_Update(double dt, double throttle, double brake, double steer);
    __declspec(dllexport) void SimPlugin_GetVehicleState(VehicleState* state);

#ifdef __cplusplus
}
#endif
