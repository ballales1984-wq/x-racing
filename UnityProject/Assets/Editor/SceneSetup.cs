using UnityEditor;
using UnityEngine;
using UnityEditor.SceneManagement;

namespace Project0.Unity.Setup
{
    public static class SceneSetup
    {
        public static void CreateScene()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene);

            var ground = GameObject.CreatePrimitive(PrimitiveType.Plane);
            ground.name = "Ground";
            ground.transform.position = Vector3.zero;
            ground.transform.localScale = new Vector3(50, 1, 50);

            var car = GameObject.CreatePrimitive(PrimitiveType.Cube);
            car.name = "Car";
            car.transform.position = new Vector3(0, 0.5f, 0);
            car.transform.localScale = new Vector3(1, 0.5f, 2);
            car.AddComponent<CarController>();

            var camera = GameObject.Find("Main Camera");
            if (camera != null)
            {
                camera.transform.position = new Vector3(-10, 8, -10);
                camera.transform.LookAt(Vector3.zero);
            }

            EditorSceneManager.SaveScene(scene, "Assets/Scenes/MainScene.unity");
            Debug.Log("Scene created successfully!");
        }
    }
}
