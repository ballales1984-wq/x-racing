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
        public bool useDirectControl = true;

        [Header("Follow Camera")]
        public Camera followCamera;
        public bool firstPersonView = true;
        public float cameraFollowDistance = 6f;
        public float cameraFollowHeight = 2f;
        public float cameraSmoothTime = 0.15f;
        public float cameraRotationSmoothTime = 0.1f;
        public Vector3 firstPersonOffset = new Vector3(0f, 1.2f, 0.3f);
        public KeyCode toggleCameraKey = KeyCode.C;

        [Header("Direct Control Physics")]
        public float maxSpeed = 80f;
        public float acceleration = 8f;
        public float brakeForce = 50f;
        public float steerSpeed = 30f;
        public float maxSteerAngle = 7f;
        public float naturalDeceleration = 5f;

        private TelemetryFrame[] frames;
        private int currentIndex = 0;
        private float elapsedTime = 0f;
        private bool loaded = false;
        private string lastError = "";
        private Vector3 cameraVelocity;

        private float currentSpeed = 0f;
        private float currentHeading = 0f;
        private float cameraRotationVelocity;

        void Update()
        {
            if (Input.GetKeyDown(toggleCameraKey))
            {
                firstPersonView = !firstPersonView;
                UpdateCameraMode();
            }

            if (useDirectControl)
            {
                UpdateDirectControl();
            }
            else
            {
                UpdateTelemetryPlayback();
            }

            UpdateCamera();
        }

        void UpdateCameraMode()
        {
            if (followCamera == null) return;

            if (firstPersonView)
            {
                followCamera.transform.SetParent(transform);
                followCamera.transform.localPosition = firstPersonOffset;
                followCamera.transform.localRotation = Quaternion.identity;
            }
            else
            {
                followCamera.transform.SetParent(null);
            }
        }

        void UpdateDirectControl()
        {
            float throttle = 0f;
            float brake = 0f;
            float steer = 0f;

            if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow))
            {
                throttle = 1f;
            }
            if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow))
            {
                brake = 1f;
            }
            if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow))
            {
                steer = 1f;
            }
            if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow))
            {
                steer = -1f;
            }

            if (throttle > 0f)
            {
                currentSpeed += acceleration * Time.deltaTime;
            }
            else if (brake > 0f)
            {
                currentSpeed -= brakeForce * Time.deltaTime;
            }
            else
            {
                if (currentSpeed > 0f)
                {
                    currentSpeed -= naturalDeceleration * Time.deltaTime;
                    if (currentSpeed < 0f) currentSpeed = 0f;
                }
                else if (currentSpeed < 0f)
                {
                    currentSpeed += naturalDeceleration * Time.deltaTime;
                    if (currentSpeed > 0f) currentSpeed = 0f;
                }
            }

            currentSpeed = Mathf.Clamp(currentSpeed, -maxSpeed * 0.3f, maxSpeed);

            if (Mathf.Abs(currentSpeed) > 0.1f)
            {
                float steerFactor = Mathf.Clamp01(Mathf.Abs(currentSpeed) / (maxSpeed * 0.5f));
                float effectiveSteer = steer * maxSteerAngle * (1f - steerFactor * 0.7f);
                currentHeading -= effectiveSteer * Time.deltaTime * Mathf.Sign(currentSpeed);
            }

            currentHeading = NormalizeAngle(currentHeading);

            Vector3 forward = new Vector3(Mathf.Sin(currentHeading), 0f, Mathf.Cos(currentHeading));
            Vector3 move = forward * currentSpeed * Time.deltaTime;

            transform.position += move;
            transform.eulerAngles = new Vector3(0f, currentHeading * Mathf.Rad2Deg, 0f);
        }

        void UpdateTelemetryPlayback()
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
        }

        void UpdateCamera()
        {
            if (followCamera == null) return;

            if (firstPersonView)
            {
                followCamera.transform.localPosition = firstPersonOffset;
                followCamera.transform.localRotation = Quaternion.identity;
                return;
            }

            float heading = transform.eulerAngles.y * Mathf.Deg2Rad;
            Vector3 behind = new Vector3(Mathf.Sin(heading + Mathf.PI), 0f, Mathf.Cos(heading + Mathf.PI));
            Vector3 targetPos = transform.position + behind * cameraFollowDistance + Vector3.up * cameraFollowHeight;
            followCamera.transform.position = Vector3.SmoothDamp(followCamera.transform.position, targetPos, ref cameraVelocity, cameraSmoothTime);
            followCamera.transform.LookAt(transform.position + Vector3.up * 0.5f);
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
            currentSpeed = 0f;
            currentHeading = transform.eulerAngles.y * Mathf.Deg2Rad;
            UpdateCameraMode();
        }

        public string GetStatus()
        {
            if (useDirectControl)
            {
                return $"Direct Control | Speed={currentSpeed:F1} km/h | Heading={currentHeading * Mathf.Rad2Deg:F1} deg";
            }
            if (!loaded) return $"Not loaded. {lastError}";
            return $"Frame {currentIndex}/{frames.Length} | t={elapsedTime:F2}s | Speed={(frames[currentIndex].speed * 3.6f):F1} km/h";
        }

        private void OnValidate()
        {
            if (autoReload && !loaded && File.Exists(telemetryPath) && !useDirectControl)
            {
                LoadTelemetry();
            }
        }

        private float NormalizeAngle(float angle)
        {
            while (angle > Mathf.PI) angle -= 2f * Mathf.PI;
            while (angle < -Mathf.PI) angle += 2f * Mathf.PI;
            return angle;
        }

        private void Start()
        {
            UpdateCameraMode();
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
