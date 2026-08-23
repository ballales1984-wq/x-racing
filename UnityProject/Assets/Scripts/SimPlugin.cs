using System;
using System.Runtime.InteropServices;

namespace Project0.Unity
{
    public struct VehicleState
    {
        public double x;
        public double y;
        public double vx;
        public double vy;
        public double heading;
        public double speed;
        public double rpm;
        public int gear;
        public double throttle;
        public double brake;
        public double steer;
    }

    public static class SimPlugin
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct NativeVehicleState
        {
            public double x;
            public double y;
            public double vx;
            public double vy;
            public double heading;
            public double speed;
            public double rpm;
            public int gear;
            public double throttle;
            public double brake;
            public double steer;
        }

        [DllImport("sim_plugin", CallingConvention = CallingConvention.Cdecl)]
        private static extern int SimPlugin_Initialize();

        [DllImport("sim_plugin", CallingConvention = CallingConvention.Cdecl)]
        private static extern void SimPlugin_Update(double dt, double throttle, double brake, double steer);

        [DllImport("sim_plugin", CallingConvention = CallingConvention.Cdecl)]
        private static extern void SimPlugin_GetVehicleState(out NativeVehicleState state);

        private static bool s_initialized = false;

        public static bool Initialize()
        {
            if (s_initialized) return true;
            int result = SimPlugin_Initialize();
            s_initialized = (result == 0);
            return s_initialized;
        }

        public static VehicleState Update(double dt, double throttle, double brake, double steer)
        {
            SimPlugin_Update(dt, throttle, brake, steer);
            NativeVehicleState nativeState;
            SimPlugin_GetVehicleState(out nativeState);
            
            return new VehicleState
            {
                x = nativeState.x,
                y = nativeState.y,
                vx = nativeState.vx,
                vy = nativeState.vy,
                heading = nativeState.heading,
                speed = nativeState.speed,
                rpm = nativeState.rpm,
                gear = nativeState.gear,
                throttle = nativeState.throttle,
                brake = nativeState.brake,
                steer = nativeState.steer
            };
        }
    }
}
