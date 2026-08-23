using UnityEditor;
using UnityEngine;
using UnityEditor.SceneManagement;

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
            car.transform.position = new Vector3(0, 0.5f, 0);
            car.transform.localScale = new Vector3(1, 0.5f, 2);
            if (car.GetComponent<CarController>() == null)
            {
                car.AddComponent<CarController>();
            }

            var camera = GameObject.Find("Main Camera");
            if (camera == null)
            {
                camera = new GameObject("Main Camera");
                camera.AddComponent<Camera>();
                camera.tag = "MainCamera";
            }
            camera.transform.position = new Vector3(-8, 4, -8);
            camera.transform.LookAt(Vector3.zero);

            Debug.Log("Scene setup updated.");
        }
    }
}
