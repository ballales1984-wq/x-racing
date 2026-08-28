using UnityEditor;
using UnityEngine;
using UnityEngine.UI;
using TMPro;
using UnityEditor.SceneManagement;

namespace Project0.Unity.Setup
{
    public static class XRSceneSetup
    {
        [MenuItem("X-Racing/Setup Complete Scene")]
        public static void SetupCompleteScene()
        {
            CreateOrFind("Main Camera", (cam) =>
            {
                cam.tag = "MainCamera";
                cam.AddComponent<Camera>();
                cam.transform.position = new Vector3(0f, 5f, -10f);
                cam.transform.LookAt(Vector3.zero);
            });

            CreateOrFind("Directional Light", (light) =>
            {
                light.transform.rotation = Quaternion.Euler(50f, -30f, 0f);
                var l = light.GetComponent<Light>();
                if (l == null) l = light.AddComponent<Light>();
                l.type = LightType.Directional;
                l.intensity = 1f;
            });

            CreateOrFind("XRTrackGenerator", (gen) =>
            {
                if (gen.GetComponent<XRTrackGenerator>() == null)
                    gen.AddComponent<XRTrackGenerator>();
            });

            CreateOrFind("Car", (car) =>
            {
                if (car.GetComponent<CarController>() == null)
                    car.AddComponent<CarController>();
                if (car.GetComponent<XRRaceManager>() == null)
                    car.AddComponent<XRRaceManager>();
                car.transform.position = new Vector3(0f, 0.5f, 0f);
                car.transform.localScale = Vector3.one;
            });

            CreateOrFind("XRGameBootstrap", (bs) =>
            {
                if (bs.GetComponent<XRGameBootstrap>() == null)
                    bs.AddComponent<XRGameBootstrap>();
            });

            CreateUI("XRUIManager", (ui) =>
            {
                if (ui.GetComponent<XRUIManager>() == null)
                    ui.AddComponent<XRUIManager>();
            });

            CreateHUD();

            EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene());
            Debug.Log("X-Racing scene setup complete! Use XRGameBootstrap to auto-wire on play.");
        }

        [MenuItem("X-Racing/Deploy Sim Plugin DLL")]
        public static void DeploySimPlugin()
        {
            string src = System.IO.Path.Combine(System.IO.Directory.GetParent(Application.dataPath).FullName, "build", "engine", "Release", "sim_plugin.dll");
            string dst = System.IO.Path.Combine(Application.dataPath, "Plugins", "sim_plugin.dll");

            if (System.IO.File.Exists(src))
            {
                System.IO.File.Copy(src, dst, true);
                AssetDatabase.Refresh();
                Debug.Log($"sim_plugin.dll deployed to: {dst}");
            }
            else
            {
                Debug.LogError($"sim_plugin.dll not found at: {src}\nBuild the C++ project first (cmake --build build --target sim_plugin)");
            }
        }

        static void CreateOrFind(string name, System.Action<GameObject> setup)
        {
            var obj = GameObject.Find(name);
            if (obj == null)
            {
                obj = new GameObject(name);
            }
            setup(obj);
        }

        static void CreateUI(string name, System.Action<GameObject> setup)
        {
            var obj = GameObject.Find(name);
            if (obj == null)
            {
                obj = new GameObject(name);
            }
            var canvas = obj.GetComponent<Canvas>();
            if (canvas == null)
            {
                canvas = obj.AddComponent<Canvas>();
                canvas.renderMode = RenderMode.ScreenSpaceOverlay;
                obj.AddComponent<CanvasScaler>();
                obj.AddComponent<GraphicRaycaster>();
            }
            setup(obj);
        }

        static void CreateHUD()
        {
            var hudObj = GameObject.Find("HUD");
            if (hudObj == null)
            {
                hudObj = new GameObject("HUD");
                var canvas = hudObj.AddComponent<Canvas>();
                canvas.renderMode = RenderMode.ScreenSpaceOverlay;
                hudObj.AddComponent<CanvasScaler>();
                hudObj.AddComponent<GraphicRaycaster>();
            }

            var parentPanel = hudObj.transform.Find("Panel");
            if (parentPanel == null)
            {
                var panelGO = new GameObject("Panel");
                panelGO.transform.SetParent(hudObj.transform, false);
                var rect = panelGO.AddComponent<RectTransform>();
                rect.anchorMin = new Vector2(0.02f, 0.05f);
                rect.anchorMax = new Vector2(0.25f, 0.35f);
                rect.offsetMin = Vector2.zero;
                rect.offsetMax = Vector2.zero;
                parentPanel = panelGO.transform;
            }

            CreateHUDText("SpeedText", "0 km/h", parentPanel, new Vector2(0, 80));
            CreateHUDText("RPMText", "800 RPM", parentPanel, new Vector2(0, 50));
            CreateHUDText("GearText", "N", parentPanel, new Vector2(0, 20));
            CreateHUDText("LapTimeText", "00:00.000", parentPanel, new Vector2(0, -10));
            CreateHUDText("BestLapText", "Best: --:--.---", parentPanel, new Vector2(0, -40));
            CreateHUDText("LapCountText", "Lap 0/3", parentPanel, new Vector2(0, -70));

            if (hudObj.GetComponent<CarHUD>() == null)
            {
                hudObj.AddComponent<CarHUD>();
            }
        }

        static void CreateHUDText(string name, string defaultText, Transform parent, Vector2 anchoredPosition)
        {
            var existing = parent.Find(name);
            if (existing != null) return;

            var go = new GameObject(name);
            go.transform.SetParent(parent, false);
            var text = go.AddComponent<TextMeshProUGUI>();
            text.text = defaultText;
            text.fontSize = 20;
            text.color = Color.white;
            text.alignment = TextAlignmentOptions.Left;
            var rect = go.GetComponent<RectTransform>();
            rect.anchorMin = new Vector2(0, 0);
            rect.anchorMax = new Vector2(1, 1);
            rect.offsetMin = new Vector2(anchoredPosition.x, anchoredPosition.y - 15);
            rect.offsetMax = new Vector2(anchoredPosition.x + 250, anchoredPosition.y + 15);
        }
    }
}
