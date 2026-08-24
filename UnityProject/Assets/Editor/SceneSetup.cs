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

            car.transform.position = new Vector3(0f, 0.9f, 0f);
            car.transform.localScale = new Vector3(1.9f, 1.1f, 5.3f);
            car.transform.eulerAngles = new Vector3(0f, 90f, 0f);

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

            Debug.Log("Scene setup updated.");
        }
    }
}
