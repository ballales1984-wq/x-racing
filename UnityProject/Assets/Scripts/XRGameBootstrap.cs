using UnityEngine;

namespace Project0.Unity
{
    public class XRGameBootstrap : MonoBehaviour
    {
        [Header("Scene References")]
        public XRTrackGenerator trackGenerator;
        public XRRaceManager raceManager;
        public XRUIManager uiManager;
        public CarController carController;
        public CarHUD carHUD;

        void Start()
        {
            if (trackGenerator == null)
            {
                trackGenerator = FindAnyObjectByType<XRTrackGenerator>();
            }
            if (raceManager == null)
            {
                raceManager = FindAnyObjectByType<XRRaceManager>();
            }

            if (raceManager != null && carController != null)
            {
                raceManager.carController = carController;
            }
            if (raceManager != null && trackGenerator != null)
            {
                raceManager.trackGenerator = trackGenerator;
            }
            if (raceManager != null && carHUD != null)
            {
                raceManager.carHUD = carHUD;
            }
            if (carController != null && carHUD != null)
            {
                carController.carHUD = carHUD;
            }
            if (carHUD != null && raceManager != null)
            {
                carHUD.raceManager = raceManager;
            }

            var mainCamera = Camera.main;
            if (mainCamera != null && raceManager != null)
            {
                raceManager.followCamera = mainCamera;
            }
            if (mainCamera != null && carController != null)
            {
                carController.followCamera = mainCamera;
            }

            if (carController != null)
            {
                carController.enabled = false;
            }

            // XRTrackGenerator.Start() handles initial track generation,
            // so we don't call GenerateTrack() here to avoid double generation.
        }
    }
}
