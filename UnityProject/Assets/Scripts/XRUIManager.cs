using UnityEngine;
using TMPro;
using UnityEngine.SceneManagement;

namespace Project0.Unity
{
    public class XRUIManager : MonoBehaviour
    {
        [Header("Menu Panel")]
        public GameObject menuPanel;
        public TMP_Text trackNameText;
        public TMP_Text lapCountText;
        public TMP_Text startPromptText;

        [Header("References")]
        public XRRaceManager raceManager;
        public CarController carController;

        private string[] trackNames = { "Default Circuit", "Pit Circuit" };
        private int selectedTrack = 0;
        private int[] lapOptions = { 1, 3, 5, 10 };
        private int selectedLapIndex = 1;

        void Start()
        {
            if (raceManager == null)
            {
                raceManager = FindObjectOfType<XRRaceManager>();
            }
            if (carController == null)
            {
                carController = FindObjectOfType<CarController>();
            }

            UpdateMenuDisplay();
        }

        void Update()
        {
            if (raceManager == null) return;

            if (raceManager.CurrentState == GameState.MENU)
            {
                HandleMenuInput();
            }
        }

        void HandleMenuInput()
        {
            if (Input.GetKeyDown(KeyCode.W) || Input.GetKeyDown(KeyCode.UpArrow))
            {
                selectedTrack = (selectedTrack + 1) % trackNames.Length;
                UpdateMenuDisplay();
            }
            if (Input.GetKeyDown(KeyCode.S) || Input.GetKeyDown(KeyCode.DownArrow))
            {
                selectedTrack = (selectedTrack - 1 + trackNames.Length) % trackNames.Length;
                UpdateMenuDisplay();
            }

            if (Input.GetKeyDown(KeyCode.A) || Input.GetKeyDown(KeyCode.LeftArrow))
            {
                selectedLapIndex = (selectedLapIndex - 1 + lapOptions.Length) % lapOptions.Length;
                UpdateMenuDisplay();
            }
            if (Input.GetKeyDown(KeyCode.D) || Input.GetKeyDown(KeyCode.RightArrow))
            {
                selectedLapIndex = (selectedLapIndex + 1) % lapOptions.Length;
                UpdateMenuDisplay();
            }

            if (Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.Space))
            {
                StartRace();
            }
        }

        void UpdateMenuDisplay()
        {
            if (trackNameText != null)
            {
                trackNameText.text = $"> {trackNames[selectedTrack]} <";
            }
            if (lapCountText != null)
            {
                lapCountText.text = $"> {lapOptions[selectedLapIndex]} Laps <";
            }
            if (startPromptText != null)
            {
                startPromptText.text = "Press ENTER to Start\nW/S = Track | A/D = Laps";
            }
        }

        void StartRace()
        {
            if (raceManager != null)
            {
                raceManager.trackName = trackNames[selectedTrack];
                raceManager.trackType = selectedTrack;
                raceManager.lapCount = lapOptions[selectedLapIndex];
                raceManager.StartRace();
            }

            if (menuPanel != null)
            {
                menuPanel.SetActive(false);
            }
        }

        public void OnBackToMenu()
        {
            if (raceManager != null)
            {
                raceManager.BackToMenu();
            }
            if (menuPanel != null)
            {
                menuPanel.SetActive(true);
            }
            UpdateMenuDisplay();
        }

        public void QuitGame()
        {
            Application.Quit();
        }
    }
}
