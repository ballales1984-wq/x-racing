using UnityEditor;
using UnityEngine;
using UnityEditor.SceneManagement;
using TMPro;
using UnityEngine.UI;

namespace Project0.Unity.Setup
{
    public static class SceneSetup
    {
        [MenuItem("X-Racing/Setup Scene")]
        public static void SetupScene()
        {
            if (EditorApplication.isPlaying)
            {
                Debug.LogError("Scene setup cannot run during play mode. Exit play mode and run the menu item again.");
                return;
            }

            var car = GameObject.Find("Car");
            if (car != null)
            {
                Object.DestroyImmediate(car);
            }

            string[] fbxPaths = new string[]
            {
                "Assets/Models/car.fbx"
            };

            GameObject carModel = null;
            foreach (var fbxPath in fbxPaths)
            {
                carModel = AssetDatabase.LoadAssetAtPath<GameObject>(fbxPath);
                if (carModel != null)
                {
                    Debug.Log($"Loaded car model from: {fbxPath}");
                    break;
                }
            }

            if (carModel != null)
            {
                car = Object.Instantiate(carModel);
                car.name = "Car";
            }
            else
            {
                car = GameObject.CreatePrimitive(PrimitiveType.Cube);
                car.name = "Car";
                Debug.LogWarning("No car FBX found, using placeholder cube");
            }

            car.transform.position = new Vector3(0f, 0.5f, 0f);
            car.transform.localScale = new Vector3(500f, 500f, 500f);
            car.transform.eulerAngles = new Vector3(0f, 90f, 0f);

            var carRenderers = car.GetComponentsInChildren<Renderer>();
            Shader shader = Shader.Find("Standard");
            foreach (var carRenderer in carRenderers)
            {
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
            camera.transform.localPosition = new Vector3(0f, 0.0018f, 0.0006f);
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

            foreach (var text in hud.GetComponentsInChildren<TMP_Text>())
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
            var text = go.AddComponent<TextMeshProUGUI>();
            text.text = defaultText;
            text.fontSize = 24;
            text.color = Color.white;
            text.alignment = TextAlignmentOptions.Left;
            var rect = go.GetComponent<RectTransform>();
            rect.anchorMin = new Vector2(0, 0);
            rect.anchorMax = new Vector2(1, 1);
            rect.offsetMin = new Vector2(anchoredPosition.x, anchoredPosition.y - 15);
            rect.offsetMax = new Vector2(anchoredPosition.x + 200, anchoredPosition.y + 15);
        }
    }
}
