using UnityEngine;
using UnityEngine.UI;

namespace Project0.Unity
{
    public class CarHUD : MonoBehaviour
    {
        [Header("References")]
        public CarController carController;
        public Text speedText;
        public Text rpmText;
        public Text gearText;
        public Text lapTimeText;
        public Text bestLapText;
        public Text lapCountText;
        public Text slipText;

        [Header("Settings")]
        public float updateInterval = 0.1f;

        private float _timer;
        private float _currentLapTime;
        private float _bestLapTime = float.MaxValue;
        private bool _lapStarted = false;
        private int _lapCount = 0;

        void Update()
        {
            if (carController == null) return;

            _timer += Time.deltaTime;
            if (_timer >= updateInterval)
            {
                _timer = 0f;
                UpdateHUD();
            }

            if (_lapStarted)
            {
                _currentLapTime += Time.deltaTime;
            }
        }

        void UpdateHUD()
        {
            if (speedText != null)
            {
                float speedKmh = carController.currentSpeed * 3.6f;
                speedText.text = $"{(int)speedKmh} km/h";
            }

            if (rpmText != null)
            {
                rpmText.text = $"{(int)carController.currentRpm} RPM";
            }

            if (gearText != null)
            {
                int gear = carController.currentGear;
                gearText.text = gear > 0 ? gear.ToString() : (carController.currentSpeed > 0.1f ? "R" : "N");
            }

            if (lapTimeText != null)
            {
                lapTimeText.text = FormatTime(_currentLapTime);
            }

            if (bestLapText != null)
            {
                bestLapText.text = _bestLapTime < float.MaxValue ? FormatTime(_bestLapTime) : "--:--";
            }

            if (lapCountText != null)
            {
                lapCountText.text = $"Lap {_lapCount}";
            }

            if (slipText != null)
            {
                slipText.text = $"Steer: {carController.currentSteerAngle * Mathf.Rad2Deg:F1}°";
            }
        }

        public void StartLap()
        {
            if (_lapStarted && _currentLapTime > 5f)
            {
                if (_currentLapTime < _bestLapTime)
                {
                    _bestLapTime = _currentLapTime;
                }
                _lapCount++;
            }
            _currentLapTime = 0f;
            _lapStarted = true;
        }

        string FormatTime(float time)
        {
            int minutes = (int)(time / 60);
            float seconds = time % 60;
            return $"{minutes:00}:{seconds:00.00}";
        }
    }
}
