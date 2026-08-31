using UnityEngine;
using System.IO;

namespace Project0.Unity
{
    [ExecuteAlways]
    public class CarController : MonoBehaviour
    {
        private void Awake()
        {
            var mf = GetComponent<MeshFilter>();
            if (mf != null && (mf.sharedMesh == null || mf.sharedMesh.name.Contains("Cube")))
            {
                var carMesh = Resources.Load<Mesh>("Models/car");
                if (carMesh != null)
                {
                    mf.sharedMesh = carMesh;
                }
            }
        }
        [Header("Telemetry")]
        public string telemetryPath = @"D:\x-racing\data\telemetry\unity_state.csv";
        public bool autoReload = true;

        [Header("Movement")]
        public float speedScale = 1.0f;
        public bool loop = true;
        public bool useDirectControl = true;
        public bool useSimPlugin = false;

        [Header("Follow Camera")]
        public Camera followCamera;
        public bool firstPersonView = true;
        public float cameraFollowDistance = 6f;
        public float cameraFollowHeight = 2f;
        public float cameraSmoothTime = 0.15f;
        public float cameraRotationSmoothTime = 0.1f;
        public Vector3 firstPersonOffset = new Vector3(0f, 1.5f, -0.5f);
        public KeyCode toggleCameraKey = KeyCode.C;

        [Header("Direct Control Physics")]
        public float maxSpeed = 80f;
        public float acceleration = 8f;
        public float brakeForce = 50f;
        public float steerSpeed = 30f;
        public float maxSteerAngle = 7f;
        public float naturalDeceleration = 5f;

        [Header("Lap Timing")]
        public CarHUD carHUD;
        public Vector3 startLinePosition = Vector3.zero;
        public float startLineThreshold = 8f;
        private bool lapStarted = false;

        private int lastCheckpoint = -1;
        private int checkpointsPassed = 0;
        private bool allCheckpointsPassed = false;
        private int totalCheckpoints = 0;

        private TelemetryFrame[] frames;
        private int currentIndex = 0;
        private float elapsedTime = 0f;
        private bool loaded = false;
        private string lastError = "";
        private Vector3 cameraVelocity;

        public float currentSpeed = 0f;
        private float currentHeading = 0f;
        private float cameraRotationVelocity;

        public float currentRpm = 0f;
        public int currentGear = 1;
        public float currentSteerAngle = 0f;

        void Update()
        {
            if (Input.GetKeyDown(toggleCameraKey))
            {
                firstPersonView = !firstPersonView;
                UpdateCameraMode();
            }

            if (Input.GetKeyDown(KeyCode.R))
            {
                ResetCarPosition();
            }

            if (useSimPlugin)
            {
                UpdateSimPlugin();
            }
            else if (useDirectControl)
            {
                UpdateDirectControl();
            }
            else
            {
                UpdateTelemetryPlayback();
            }

            UpdateCamera();
            CheckStartLineCrossing();
        }

        public void ResetCarPosition()
        {
            // Face the track's start direction (main straight heads east / +X).
            SetStartPose(startLinePosition, Mathf.PI * 0.5f);
        }

        public void SetStartPose(Vector3 position, float heading)
        {
            transform.position = position;
            startLinePosition = position;
            currentHeading = heading;
            transform.eulerAngles = new Vector3(0f, heading * Mathf.Rad2Deg, 0f);
            currentSpeed = 0f;
            currentRpm = 800f;
            currentGear = 1;
            currentSteerAngle = 0f;
            ResetCheckpoints();
        }

        void UpdateCameraMode()
        {
            if (followCamera == null) return;

            if (firstPersonView)
            {
                followCamera.transform.SetParent(transform);
                followCamera.transform.localPosition = firstPersonOffset;
                followCamera.transform.localRotation = Quaternion.Euler(0f, 0f, 0f);
            }
            else
            {
                followCamera.transform.SetParent(null);
            }
        }

        void UpdateSimPlugin()
        {
            if (!SimPlugin.Initialize())
            {
                // Native plugin unavailable (e.g. sim_plugin.dll not deployed). Fall back
                // to direct control so the car remains drivable instead of freezing.
                UpdateDirectControl();
                return;
            }

            float throttle = 0f;
            float brake = 0f;
            float steer = 0f;

            if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) throttle = 1f;
            if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) brake = 1f;
            if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) steer = 1f;
            if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) steer = -1f;

            var state = SimPlugin.Update(Time.deltaTime, throttle, brake, steer);
            transform.position = new Vector3((float)state.x, 0.6f, (float)state.y);
            float unityHeading = Mathf.Repeat((float)state.heading * Mathf.Rad2Deg + 270f, 360f);
            transform.eulerAngles = new Vector3(0f, unityHeading, 0f);

            currentSpeed = (float)state.speed;
            currentRpm = (float)state.rpm;
            currentGear = state.gear;
            currentSteerAngle = (float)state.steer;
            currentHeading = (float)state.heading;
        }

        void UpdateDirectControl()
        {
            float throttle = 0f;
            float brake = 0f;
            float steerInput = 0f;

            if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) throttle = 1f;
            if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) brake = 1f;
            if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) steerInput = 1f;
            if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) steerInput = -1f;

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
                float steerChange = steerInput * steerSpeed * Time.deltaTime;
                currentSteerAngle += steerChange;
                currentSteerAngle = Mathf.Clamp(currentSteerAngle, -maxSteerAngle * Mathf.Deg2Rad, maxSteerAngle * Mathf.Deg2Rad);
                float effectiveSteer = currentSteerAngle * (1f - steerFactor * 0.7f);
                currentHeading -= effectiveSteer * Time.deltaTime * Mathf.Sign(currentSpeed);
            }
            else if (steerInput == 0f)
            {
                currentSteerAngle = 0f;
            }

            currentHeading = NormalizeAngle(currentHeading);

            Vector3 forward = new Vector3(Mathf.Sin(currentHeading), 0f, Mathf.Cos(currentHeading));
            Vector3 move = forward * currentSpeed * Time.deltaTime;
            transform.position += move;
            transform.eulerAngles = new Vector3(0f, currentHeading * Mathf.Rad2Deg, 0f);

            float speedFrac = Mathf.Abs(currentSpeed) / maxSpeed;
            currentRpm = 800f + speedFrac * 7000f;
            currentGear = Mathf.Clamp(Mathf.FloorToInt(speedFrac * 6f) + 1, 1, 6);
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
            float unityHeading = Mathf.Repeat(frame.heading * Mathf.Rad2Deg + 270f, 360f);
            transform.eulerAngles = new Vector3(0f, unityHeading, 0f);

            CheckProximityCheckpoints();
        }

        void CheckProximityCheckpoints()
        {
            var checkpoints = GameObject.Find("Checkpoints");
            if (checkpoints == null) return;

            if (allCheckpointsPassed) return;

            totalCheckpoints = checkpoints.transform.childCount;
            int nextIndex = lastCheckpoint + 1;
            if (nextIndex >= totalCheckpoints) nextIndex = 0;

            Transform cpTransform = checkpoints.transform.GetChild(nextIndex);
            float distance = Vector3.Distance(transform.position, cpTransform.position);
            var trigger = cpTransform.GetComponent<CheckpointTrigger>();
            float threshold = trigger != null ? trigger.trackWidth * 0.5f : 15f;
            if (distance < threshold)
            {
                OnCheckpointPassed(nextIndex, totalCheckpoints);
            }
        }

        void UpdateCamera()
        {
            if (followCamera == null) return;

            if (firstPersonView)
            {
                followCamera.transform.SetParent(transform);
                followCamera.transform.localPosition = firstPersonOffset;
                followCamera.transform.localRotation = Quaternion.Euler(0f, 0f, 0f);
                return;
            }

            float heading = transform.eulerAngles.y * Mathf.Deg2Rad;
            Vector3 behind = new Vector3(Mathf.Sin(heading + Mathf.PI), 0f, Mathf.Cos(heading + Mathf.PI));
            Vector3 targetPos = transform.position + behind * cameraFollowDistance + Vector3.up * cameraFollowHeight;
            followCamera.transform.position = Vector3.SmoothDamp(followCamera.transform.position, targetPos, ref cameraVelocity, cameraSmoothTime);
            followCamera.transform.LookAt(transform.position + Vector3.up * 0.5f);
        }

        void CheckStartLineCrossing()
        {
            Vector2 carPos2D = new Vector2(transform.position.x, transform.position.z);
            Vector2 startLine2D = new Vector2(startLinePosition.x, startLinePosition.z);
            float distFromStart = Vector2.Distance(carPos2D, startLine2D);

            if (!lapStarted)
            {
                if (distFromStart > startLineThreshold)
                {
                    lapStarted = true;
                    if (carHUD != null) carHUD.BeginLap();
                }
            }
            else
            {
                if (distFromStart <= startLineThreshold && allCheckpointsPassed)
                {
                    bool valid = true;
                    if (carHUD != null)
                    {
                        carHUD.EndLap(valid);
                    }
                    lapStarted = false;
                    allCheckpointsPassed = false;
                    lastCheckpoint = -1;
                    checkpointsPassed = 0;
                }
            }
        }

        public void OnCheckpointPassed(int checkpointIndex, int totalCheckpoints)
        {
            if (checkpointIndex == lastCheckpoint + 1 || (lastCheckpoint == totalCheckpoints - 1 && checkpointIndex == 0))
            {
                lastCheckpoint = checkpointIndex;
                checkpointsPassed++;

                if (checkpointsPassed >= totalCheckpoints)
                {
                    allCheckpointsPassed = true;
                }
            }
        }

        public void ResetCheckpoints()
        {
            lastCheckpoint = -1;
            checkpointsPassed = 0;
            allCheckpointsPassed = false;
            totalCheckpoints = 0;
            lapStarted = false;
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
                    if (parts.Length < 20) continue;

                    try
                    {
                        TelemetryFrame frame = new TelemetryFrame
                        {
                            time = float.Parse(parts[0]),
                            lapNumber = int.Parse(parts[1]),
                            distance = float.Parse(parts[2]),
                            speed = float.Parse(parts[3]),
                            rpm = float.Parse(parts[4]),
                            gear = int.Parse(parts[5]),
                            throttle = float.Parse(parts[6]),
                            brake = float.Parse(parts[7]),
                            steer = float.Parse(parts[8]),
                            slipAngle = float.Parse(parts[9]),
                            slipRatio = float.Parse(parts[10]),
                            posX = float.Parse(parts[11]),
                            posY = float.Parse(parts[12]),
                            velX = float.Parse(parts[13]),
                            velY = float.Parse(parts[14]),
                            accX = float.Parse(parts[15]),
                            accY = float.Parse(parts[16]),
                            heading = float.Parse(parts[17]),
                            lateralG = float.Parse(parts[18]),
                            longitudinalG = float.Parse(parts[19])
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
            lapStarted = false;

            if (useSimPlugin)
            {
                SimPlugin.Initialize();
            }

            UpdateCameraMode();
        }

        public string GetStatus()
        {
            if (useSimPlugin)
            {
                return $"Sim Plugin | Speed={currentSpeed * 3.6f:F1} km/h | Gear={currentGear} | RPM={currentRpm:F0}";
            }
            if (useDirectControl)
            {
                return $"Direct Control | Speed={currentSpeed * 3.6f:F1} km/h | Gear={currentGear} | RPM={currentRpm:F0}";
            }
            if (!loaded) return $"Not loaded. {lastError}";
            return $"Frame {currentIndex}/{frames.Length} | t={elapsedTime:F2}s | Speed={(frames[currentIndex].speed * 3.6f):F1} km/h";
        }

        private void OnValidate()
        {
            if (autoReload && !loaded && !useDirectControl && !useSimPlugin)
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
        public int lapNumber;
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
