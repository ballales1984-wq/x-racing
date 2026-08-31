using TMPro;
using UnityEngine;

namespace Project0.Unity
{
    public class CarHUD : MonoBehaviour
    {
        [Header("References")]
        public CarController carController;
        public XRRaceManager raceManager;
        public TMP_Text speedText;
        public TMP_Text rpmText;
        public TMP_Text gearText;
        public TMP_Text lapTimeText;
        public TMP_Text bestLapText;
        public TMP_Text lapCountText;
        public TMP_Text positionText;
        public TMP_Text slipText;

        [Header("Settings")]
        public float updateInterval = 0.1f;

        private float _timer;
        private float _currentLapTime;
        private float _bestLapTime = float.MaxValue;
        private bool _lapStarted = false;
        private int _lapCount = 0;

        void Start()
        {
            if (raceManager == null)
            {
                raceManager = FindAnyObjectByType<XRRaceManager>();
            }
            if (carController == null)
            {
                carController = FindAnyObjectByType<CarController>();
            }
        }

        void Update()
        {
            if (carController == null) return;

            _timer += Time.deltaTime;
            if (_timer >= updateInterval)
            {
                _timer = 0f;
                UpdateHUD();
            }

            if (_lapStarted && raceManager != null && raceManager.CurrentState == GameState.RACING)
            {
                _currentLapTime = raceManager.CurrentLapTime;
            }
        }

        void UpdateHUD()
        {
            if (speedText != null && carController != null)
            {
                float speedKmh = carController.currentSpeed * 3.6f;
                speedText.text = $"{(int)speedKmh} km/h";
            }

            if (rpmText != null && carController != null)
            {
                rpmText.text = $"{(int)carController.currentRpm} RPM";
            }

            if (gearText != null && carController != null)
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
                float displayBest = _bestLapTime < float.MaxValue ? _bestLapTime : (raceManager != null ? raceManager.BestLapTime : float.MaxValue);
                bestLapText.text = displayBest < float.MaxValue ? $"Best: {FormatTime(displayBest)}" : "Best: --:--.---";
            }

            if (lapCountText != null)
            {
                int displayLaps = raceManager != null ? raceManager.CompletedLaps : _lapCount;
                int totalLaps = raceManager != null ? raceManager.lapCount : 3;
                lapCountText.text = $"Lap {displayLaps}/{totalLaps}";
            }
        }

        public void BeginLap()
        {
            _currentLapTime = 0f;
            _lapStarted = true;
        }

        public void EndLap(bool valid)
        {
            if (_currentLapTime > 5f)
            {
                if (valid && _currentLapTime < _bestLapTime)
                {
                    _bestLapTime = _currentLapTime;
                }
                _lapCount++;
            }
            _lapStarted = false;
        }

        string FormatTime(float time)
        {
            int minutes = (int)(time / 60f);
            float seconds = time % 60f;
            return $"{minutes:00}:{seconds:00.000}";
        }
    }
}
