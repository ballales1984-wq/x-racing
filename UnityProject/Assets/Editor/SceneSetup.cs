using UnityEditor;
using UnityEngine;
using UnityEditor.SceneManagement;
using UnityEngine.UI;

namespace Project0.Unity.Setup
{
    public static class SceneSetup
    {
        [MenuItem("Project0/Setup Scene")]
        public static void SetupScene()
        {
            var car = GameObject.Find("Car");
            if (car == null)
            {
                car = GameObject.CreatePrimitive(PrimitiveType.Cube);
                car.name = "Car";
            }

            car.transform.position = new Vector3(0f, 0.9f, 0f);
            car.transform.localScale = new Vector3(1.9f, 1.1f, 5.3f);
            car.transform.eulerAngles = new Vector3(0f, 270f, 0f);

            var carRenderer = car.GetComponent<Renderer>();
            if (carRenderer != null)
            {
                Shader shader = Shader.Find("Universal Render Pipeline/Lit");
                if (shader == null) shader = Shader.Find("Universal Render Pipeline/Unlit");
                if (shader == null) shader = Shader.Find("Standard");
                Material carMat = shader != null ? new Material(shader) : new Material(Shader.Find("Standard"));

                if (carMat != null)
                {
                    Color redColor = new Color(0.9f, 0.1f, 0.1f);
                    if (carMat.HasProperty("_BaseColor")) carMat.SetColor("_BaseColor", redColor);
                    if (carMat.HasProperty("_Color")) carMat.SetColor("_Color", redColor);
                    carMat.color = redColor;
                    carRenderer.sharedMaterial = carMat;
                }
            }

            var carController = car.GetComponent<CarController>();
            if (carController == null)
            {
                carController = car.AddComponent<CarController>();
            }

            var camera = GameObject.Find("Main Camera");
            if (camera == null)
            {
                camera = new GameObject("Main Camera");
                camera.AddComponent<Camera>();
                camera.tag = "MainCamera";
            }

            camera.transform.SetParent(car.transform, false);
            camera.transform.localPosition = new Vector3(0f, 1.05f, 1.4f);
            camera.transform.localRotation = Quaternion.identity;

            carController.followCamera = camera.GetComponent<Camera>();

            var hud = GameObject.Find("HUD");
            if (hud == null)
            {
                hud = new GameObject("HUD");
                var canvas = hud.AddComponent<Canvas>();
                canvas.renderMode = RenderMode.ScreenSpaceOverlay;
                hud.AddComponent<CanvasScaler>();
                hud.AddComponent<GraphicRaycaster>();

                var parentPanel = new GameObject("Panel");
                parentPanel.transform.SetParent(hud.transform, false);
                var panelRect = parentPanel.AddComponent<RectTransform>();
                panelRect.anchorMin = new Vector2(0.05f, 0.05f);
                panelRect.anchorMax = new Vector2(0.3f, 0.3f);
                panelRect.offsetMin = Vector2.zero;
                panelRect.offsetMax = Vector2.zero;

                CreateHUDText("SpeedText", "0 km/h", parentPanel.transform, new Vector2(0, 30));
                CreateHUDText("RPMText", "800 RPM", parentPanel.transform, new Vector2(0, 0));
                CreateHUDText("GearText", "N", parentPanel.transform, new Vector2(0, -30));
                CreateHUDText("LapTimeText", "00:00.00", parentPanel.transform, new Vector2(0, -60));
                CreateHUDText("BestLapText", "Best: --:--", parentPanel.transform, new Vector2(0, -90));
            }

            var carHUD = hud.GetComponent<CarHUD>();
            if (carHUD == null)
            {
                carHUD = hud.AddComponent<CarHUD>();
            }
            carHUD.carController = carController;
            carController.carHUD = carHUD;

            foreach (var text in hud.GetComponentsInChildren<Text>())
            {
                if (text.name == "SpeedText") carHUD.speedText = text;
                else if (text.name == "RPMText") carHUD.rpmText = text;
                else if (text.name == "GearText") carHUD.gearText = text;
                else if (text.name == "LapTimeText") carHUD.lapTimeText = text;
                else if (text.name == "BestLapText") carHUD.bestLapText = text;
            }

            Debug.Log("Scene setup updated.");
        }

        private static void CreateHUDText(string name, string defaultText, Transform parent, Vector2 anchoredPosition)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            var text = go.AddComponent<Text>();
            text.text = defaultText;
            text.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            text.fontSize = 24;
            text.color = Color.white;
            text.alignment = TextAnchor.MiddleLeft;
            var rect = go.GetComponent<RectTransform>();
            rect.anchorMin = new Vector2(0, 0);
            rect.anchorMax = new Vector2(1, 1);
            rect.offsetMin = new Vector2(anchoredPosition.x, anchoredPosition.y - 15);
            rect.offsetMax = new Vector2(anchoredPosition.x + 200, anchoredPosition.y + 15);
        }
    }
}
