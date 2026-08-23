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
            camera.transform.position = new Vector3(-12f, 6f, -12f);
            camera.transform.LookAt(car.transform.position + Vector3.up * 0.5f);

            carController.followCamera = camera.GetComponent<Camera>();

            Debug.Log("Scene setup updated.");
        }
    }
}
