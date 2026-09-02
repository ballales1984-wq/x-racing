using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace Project0.Unity
{
    public class HUDSetup : MonoBehaviour
    {
        public CarController carController;
        public CarHUD carHUD;

        public bool autoCreateUI = true;
        public TMP_FontAsset hudFont;

        void Start()
        {
            if (autoCreateUI)
            {
                CreateHUD();
            }
        }

        void CreateHUD()
        {
            GameObject canvasObj = new GameObject("HUDCanvas");
            Canvas canvas = canvasObj.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvasObj.AddComponent<CanvasScaler>();
            canvasObj.AddComponent<GraphicRaycaster>();

            if (FindAnyObjectByType<EventSystem>() == null)
            {
                GameObject eventSystem = new GameObject("EventSystem");
                eventSystem.AddComponent<EventSystem>();
                InputCompat.EnsureUIInputModule(eventSystem);
            }

            CreateText(canvasObj, "SpeedText", new Vector2(-100, -30), TextAnchor.LowerRight, 32);
            CreateText(canvasObj, "RpmText", new Vector2(-100, -70), TextAnchor.LowerRight, 24);
            CreateText(canvasObj, "GearText", new Vector2(-100, -100), TextAnchor.LowerRight, 28);
            CreateText(canvasObj, "LapTimeText", new Vector2(0, -30), TextAnchor.LowerCenter, 24);
            CreateText(canvasObj, "BestLapText", new Vector2(0, -60), TextAnchor.LowerCenter, 20);
            CreateText(canvasObj, "LapCountText", new Vector2(0, -90), TextAnchor.LowerCenter, 20);
            CreateText(canvasObj, "SlipText", new Vector2(100, -30), TextAnchor.LowerLeft, 18);

            if (carHUD == null)
            {
                carHUD = gameObject.AddComponent<CarHUD>();
            }

            carHUD.carController = carController;

            carHUD.speedText = GameObject.Find("SpeedText").GetComponent<TextMeshProUGUI>();
            carHUD.rpmText = GameObject.Find("RpmText").GetComponent<TextMeshProUGUI>();
            carHUD.gearText = GameObject.Find("GearText").GetComponent<TextMeshProUGUI>();
            carHUD.lapTimeText = GameObject.Find("LapTimeText").GetComponent<TextMeshProUGUI>();
            carHUD.bestLapText = GameObject.Find("BestLapText").GetComponent<TextMeshProUGUI>();
            carHUD.lapCountText = GameObject.Find("LapCountText").GetComponent<TextMeshProUGUI>();
            carHUD.slipText = GameObject.Find("SlipText").GetComponent<TextMeshProUGUI>();
        }

        void CreateText(GameObject parent, string name, Vector2 position, TextAnchor alignment, int fontSize)
        {
            GameObject textObj = new GameObject(name);
            textObj.transform.SetParent(parent.transform);

            TextMeshProUGUI text = textObj.AddComponent<TextMeshProUGUI>();
            text.font = hudFont;
            text.fontSize = fontSize;
            text.alignment = AnchorToAlignment(alignment);
            text.color = Color.white;

            RectTransform rectTransform = text.GetComponent<RectTransform>();
            rectTransform.anchorMin = new Vector2(0.5f, 0);
            rectTransform.anchorMax = new Vector2(0.5f, 0);
            rectTransform.pivot = new Vector2(0.5f, 0);
            rectTransform.anchoredPosition = position;
            rectTransform.sizeDelta = new Vector2(200, 40);
        }

        static TextAlignmentOptions AnchorToAlignment(TextAnchor anchor)
        {
            return anchor switch
            {
                TextAnchor.UpperLeft => TextAlignmentOptions.TopLeft,
                TextAnchor.UpperCenter => TextAlignmentOptions.Top,
                TextAnchor.UpperRight => TextAlignmentOptions.TopRight,
                TextAnchor.MiddleLeft => TextAlignmentOptions.Left,
                TextAnchor.MiddleCenter => TextAlignmentOptions.Center,
                TextAnchor.MiddleRight => TextAlignmentOptions.Right,
                TextAnchor.LowerLeft => TextAlignmentOptions.BottomLeft,
                TextAnchor.LowerCenter => TextAlignmentOptions.Bottom,
                TextAnchor.LowerRight => TextAlignmentOptions.BottomRight,
                _ => TextAlignmentOptions.TopLeft,
            };
        }
    }
}
