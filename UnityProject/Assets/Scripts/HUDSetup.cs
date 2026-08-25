using UnityEngine;
using UnityEngine.UI;

namespace Project0.Unity
{
    public class HUDSetup : MonoBehaviour
    {
        public CarController carController;
        public CarHUD carHUD;

        public bool autoCreateUI = true;
        public Font hudFont;

        void Start()
        {
            if (autoCreateUI)
            {
                CreateHUD();
            }
        }

        void CreateHUD()
        {
            // Create Canvas
            GameObject canvasObj = new GameObject("HUDCanvas");
            Canvas canvas = canvasObj.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvasObj.AddComponent<CanvasScaler>();
            canvasObj.AddComponent<GraphicRaycaster();

            // Create event system if not present
            if (FindObjectOfType<EventSystem>() == null)
            {
                GameObject eventSystem = new GameObject("EventSystem");
                eventSystem.AddComponent<EventSystem>();
                eventSystem.AddComponent<StandaloneInputModule>();
            }

            // Create HUD elements
            CreateText(canvasObj, "SpeedText", new Vector2(-100, -30), TextAnchor.LowerRight, 32);
            CreateText(canvasObj, "RpmText", new Vector2(-100, -70), TextAnchor.LowerRight, 24);
            CreateText(canvasObj, "GearText", new Vector2(-100, -100), TextAnchor.LowerRight, 28);
            CreateText(canvasObj, "LapTimeText", new Vector2(0, -30), TextAnchor.LowerCenter, 24);
            CreateText(canvasObj, "BestLapText", new Vector2(0, -60), TextAnchor.LowerCenter, 20);
            CreateText(canvasObj, "LapCountText", new Vector2(0, -90), TextAnchor.LowerCenter, 20);
            CreateText(canvasObj, "SlipText", new Vector2(100, -30), TextAnchor.LowerLeft, 18);

            // Setup CarHUD references
            if (carHUD == null)
            {
                carHUD = gameObject.AddComponent<CarHUD>();
            }

            carHUD.carController = carController;

            // Find and assign UI elements
            carHUD.speedText = GameObject.Find("SpeedText").GetComponent<Text>();
            carHUD.rpmText = GameObject.Find("RpmText").GetComponent<Text>();
            carHUD.gearText = GameObject.Find("GearText").GetComponent<Text>();
            carHUD.lapTimeText = GameObject.Find("LapTimeText").GetComponent<Text>();
            carHUD.bestLapText = GameObject.Find("BestLapText").GetComponent<Text>();
            carHUD.lapCountText = GameObject.Find("LapCountText").GetComponent<Text>();
            carHUD.slipText = GameObject.Find("SlipText").GetComponent<Text>();
        }

        void CreateText(GameObject parent, string name, Vector2 position, TextAnchor alignment, int fontSize)
        {
            GameObject textObj = new GameObject(name);
            textObj.transform.SetParent(parent.transform);

            Text text = textObj.AddComponent<Text>();
            text.font = hudFont != null ? hudFont : Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            text.fontSize = fontSize;
            text.alignment = alignment;
            text.color = Color.white;

            RectTransform rectTransform = text.GetComponent<RectTransform>();
            rectTransform.anchorMin = new Vector2(0.5f, 0);
            rectTransform.anchorMax = new Vector2(0.5f, 0);
            rectTransform.pivot = new Vector2(0.5f, 0);
            rectTransform.anchoredPosition = position;
            rectTransform.sizeDelta = new Vector2(200, 40);
        }
    }
}
