using UnityEngine;
using System.IO;

namespace Project0.Unity
{
    [ExecuteAlways]
    public class CarController : MonoBehaviour
    {
        [Header("Telemetry")]
        public string telemetryPath = @"D:\x-racing\data\telemetry\unity_state.csv";
        public bool autoReload = true;

        [Header("Movement")]
        public float speedScale = 1.0f;
        public bool loop = true;

        [Header("Follow Camera")]
        public Camera followCamera;
        public Vector3 cameraOffset = new Vector3(-8f, 4f, -8f);
        public float cameraSmoothTime = 0.2f;

        private TelemetryFrame[] frames;
        private int currentIndex = 0;
        private float elapsedTime = 0f;
        private bool loaded = false;
        private string lastError = "";
        private Vector3 cameraVelocity;

        void Update()
        {
            if (!loaded || frames == null || frames.Length == 0)
            {
                if (autoReload && File.Exists(telemetryPath))
                {
                    LoadTelemetry();
                }
                return;
            }

            elapsedTime += Time.deltaTime * speedScale;

            while (currentIndex < frames.Length - 1 && elapsedTime >= frames[currentIndex + 1].time)
            {
                currentIndex++;
            }

            if (currentIndex >= frames.Length - 1)
            {
                if (loop)
                {
                    currentIndex = 0;
                    elapsedTime = 0f;
                }
                else
                {
                    enabled = false;
                    return;
                }
            }

            var frame = frames[currentIndex];
            transform.position = new Vector3((float)frame.posX, 0.5f, (float)frame.posY);
            transform.eulerAngles = new Vector3(0f, (float)(frame.heading * Mathf.Rad2Deg), 0f);

            if (followCamera != null)
            {
                var targetPos = transform.position + cameraOffset;
                followCamera.transform.position = Vector3.SmoothDamp(followCamera.transform.position, targetPos, ref cameraVelocity, cameraSmoothTime);
                followCamera.transform.LookAt(transform.position + Vector3.up * 0.5f);
            }
        }

        void LoadTelemetry()
        {
            if (!File.Exists(telemetryPath))
            {
                lastError = $"Telemetry file not found: {telemetryPath}";
                Debug.LogWarning(lastError);
                loaded = false;
                return;
            }

            try
            {
                var lines = File.ReadAllLines(telemetryPath);
                var data = new System.Collections.Generic.List<TelemetryFrame>();

                for (int i = 1; i < lines.Length; i++)
                {
                    var line = lines[i].Trim();
                    if (string.IsNullOrEmpty(line)) continue;

                    var parts = line.Split(',');
                    if (parts.Length < 19) continue;

                    try
                    {
                        TelemetryFrame frame = new TelemetryFrame
                        {
                            time = float.Parse(parts[0]),
                            distance = float.Parse(parts[1]),
                            speed = float.Parse(parts[2]),
                            rpm = float.Parse(parts[3]),
                            gear = int.Parse(parts[4]),
                            throttle = float.Parse(parts[5]),
                            brake = float.Parse(parts[6]),
                            steer = float.Parse(parts[7]),
                            slipAngle = float.Parse(parts[8]),
                            slipRatio = float.Parse(parts[9]),
                            posX = float.Parse(parts[10]),
                            posY = float.Parse(parts[11]),
                            velX = float.Parse(parts[12]),
                            velY = float.Parse(parts[13]),
                            accX = float.Parse(parts[14]),
                            accY = float.Parse(parts[15]),
                            heading = float.Parse(parts[16]),
                            lateralG = float.Parse(parts[17]),
                            longitudinalG = float.Parse(parts[18])
                        };
                        data.Add(frame);
                    }
                    catch (System.Exception ex)
                    {
                        Debug.LogWarning($"Failed to parse line {i}: {line} - {ex.Message}");
                    }
                }

                frames = data.ToArray();
                loaded = true;
                lastError = "";
                Debug.Log($"Loaded {frames.Length} telemetry frames from {telemetryPath}");
            }
            catch (System.Exception ex)
            {
                lastError = $"Error reading telemetry: {ex.Message}";
                Debug.LogError(lastError);
                loaded = false;
            }
        }

        void Reset()
        {
            currentIndex = 0;
            elapsedTime = 0f;
        }

        public string GetStatus()
        {
            if (!loaded) return $"Not loaded. {lastError}";
            return $"Frame {currentIndex}/{frames.Length} | t={elapsedTime:F2}s | Speed={(frames[currentIndex].speed * 3.6f):F1} km/h";
        }

        private void OnValidate()
        {
            if (autoReload && !loaded && File.Exists(telemetryPath))
            {
                LoadTelemetry();
            }
        }
    }

    public struct TelemetryFrame
    {
        public float time;
        public float distance;
        public float speed;
        public float rpm;
        public int gear;
        public float throttle;
        public float brake;
        public float steer;
        public float slipAngle;
        public float slipRatio;
        public float posX;
        public float posY;
        public float velX;
        public float velY;
        public float accX;
        public float accY;
        public float heading;
        public float lateralG;
        public float longitudinalG;
    }
}
