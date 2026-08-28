using UnityEngine;
using UnityEngine.UI;
using TMPro;
using System.IO;

namespace Project0.Unity
{
    public enum GameState { MENU, COUNTDOWN, RACING, RESULTS }

    public class XRRaceManager : MonoBehaviour
    {
        [Header("References")]
        public CarController carController;
        public Camera followCamera;
        public TMP_Text countdownText;
        public GameObject countdownPanel;
        public GameObject resultsPanel;
        public GameObject menuPanel;
        public TMP_Text resultsBestLap;
        public TMP_Text resultsTotalTime;
        public TMP_Text resultsLapsText;

        [Header("Session")]
        public int lapCount = 3;
        public int trackType = 0;
        public string trackName = "Default Circuit";

        [Header("Countdown")]
        public float countdownDuration = 3f;

        private GameState state = GameState.MENU;
        private float countdownTimer = 0f;
        private int lastCountdownNumber = -1;
        private bool raceStarted = false;

        private float bestLapTime = float.MaxValue;
        private float currentLapTime = 0f;
        private int completedLaps = 0;
        private float totalRaceTime = 0f;
        private float[] lapTimes;
        private bool[] lapValid;

        private string savePath;

        void Start()
        {
            savePath = Path.Combine(Application.persistentDataPath, "best_times.json");
            lapTimes = new float[lapCount];
            lapValid = new bool[lapCount];

            SetState(GameState.MENU);
        }

        void Update()
        {
            switch (state)
            {
                case GameState.MENU:
                    UpdateMenu();
                    break;
                case GameState.COUNTDOWN:
                    UpdateCountdown();
                    break;
                case GameState.RACING:
                    UpdateRacing();
                    break;
                case GameState.RESULTS:
                    UpdateResults();
                    break;
            }
        }

        void UpdateMenu()
        {
            if (Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.Space))
            {
                StartCountdown();
            }
        }

        void StartCountdown()
        {
            SetState(GameState.COUNTDOWN);
            countdownTimer = 0f;
            lastCountdownNumber = -1;
            raceStarted = false;

            carController.ResetCarPosition();
            carController.enabled = true;

            bestLapTime = float.MaxValue;
            currentLapTime = 0f;
            completedLaps = 0;
            totalRaceTime = 0f;
            lapTimes = new float[lapCount];
            lapValid = new bool[lapCount];

            carController.ResetCheckpoints();
        }

        void UpdateCountdown()
        {
            countdownTimer += Time.deltaTime;
            int number = Mathf.CeilToInt(countdownDuration - countdownTimer);

            if (number > 3) number = 3;
            if (number < 0) number = 0;

            if (number != lastCountdownNumber)
            {
                lastCountdownNumber = number;
                if (number > 0)
                {
                    countdownText.text = number.ToString();
                }
                else
                {
                    countdownText.text = "GO!";
                }
            }

            if (countdownTimer >= countdownDuration + 0.5f)
            {
                SetState(GameState.RACING);
                countdownPanel.SetActive(false);
            }
        }

        void UpdateRacing()
        {
            totalRaceTime += Time.deltaTime;
            currentLapTime += Time.deltaTime;

            if (Input.GetKeyDown(KeyCode.Escape))
            {
                SetState(GameState.MENU);
            }
        }

        void UpdateResults()
        {
            if (Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.Space))
            {
                SetState(GameState.MENU);
            }
        }

        public void OnLapCompleted(bool valid)
        {
            if (completedLaps < lapCount)
            {
                lapTimes[completedLaps] = currentLapTime;
                lapValid[completedLaps] = valid;

                if (valid && currentLapTime < bestLapTime)
                {
                    bestLapTime = currentLapTime;
                }

                completedLaps++;
                currentLapTime = 0f;
                carController.ResetCheckpoints();
            }

            if (completedLaps >= lapCount)
            {
                EndRace();
            }
        }

        void EndRace()
        {
            SetState(GameState.RESULTS);

            resultsBestLap.text = bestLapTime < float.MaxValue ? FormatTime(bestLapTime) : "--:--.---";
            resultsTotalTime.text = FormatTime(totalRaceTime);
            resultsLapsText.text = $"Laps: {completedLaps}/{lapCount}";

            SaveBestTimes();
        }

        void SaveBestTimes()
        {
            try
            {
                using (StreamWriter writer = new StreamWriter(savePath))
                {
                    writer.WriteLine("{");
                    writer.WriteLine($"  \"bestLapTime\": {(bestLapTime < float.MaxValue ? bestLapTime.ToString() : "null")},");
                    writer.WriteLine($"  \"totalTime\": {totalRaceTime},");
                    writer.WriteLine($"  \"completedLaps\": {completedLaps},");
                    writer.WriteLine($"  \"track\": \"{trackName}\",");
                    writer.WriteLine($"  \"lapCount\": {lapCount},");
                    writer.Write("  \"lapTimes\": [");
                    for (int i = 0; i < lapTimes.Length; i++)
                    {
                        if (i > 0) writer.Write(", ");
                        writer.Write(lapTimes[i].ToString());
                    }
                    writer.WriteLine("]");
                    writer.WriteLine("}");
                }
                Debug.Log($"Best times saved to: {savePath}");
            }
            catch (System.Exception e)
            {
                Debug.LogError($"Failed to save best times: {e.Message}");
            }
        }

        public void StartRace()
        {
            StartCountdown();
        }

        public void BackToMenu()
        {
            SetState(GameState.MENU);
            carController.enabled = false;
        }

        void SetState(GameState newState)
        {
            state = newState;

            menuPanel.SetActive(state == GameState.MENU);
            countdownPanel.SetActive(state == GameState.COUNTDOWN);
            resultsPanel.SetActive(state == GameState.RESULTS);

            if (carController != null)
            {
                carController.enabled = (state == GameState.COUNTDOWN || state == GameState.RACING);
            }
        }

        public float CurrentLapTime => currentLapTime;
        public float BestLapTime => bestLapTime;
        public int CompletedLaps => completedLaps;
        public GameState CurrentState => state;

        string FormatTime(float time)
        {
            int minutes = (int)(time / 60f);
            float seconds = time % 60f;
            return $"{minutes:00}:{seconds:00.000}";
        }
    }
}
