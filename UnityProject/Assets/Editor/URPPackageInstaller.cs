using UnityEditor;
using UnityEditor.PackageManager;
using UnityEditor.PackageManager.Requests;
using UnityEngine;

namespace Project0.Unity.Setup
{
    public static class URPPackageInstaller
    {
        [MenuItem("X-Racing/Install URP Package")]
        public static void InstallURPPackage()
        {
            EditorApplication.update += Progress;
            AddRequest = Client.Add("com.unity.render-pipelines.universal@17.5.0");
        }

        static AddRequest AddRequest;
        static void Progress()
        {
            if (AddRequest.IsCompleted)
            {
                if (AddRequest.Status == StatusCode.Success)
                    Debug.Log("[X-Racing] URP package installed successfully.");
                else if (AddRequest.Status == StatusCode.Failure)
                    Debug.LogError("[X-Racing] Failed to install URP: " + AddRequest.Error.message);

                EditorApplication.update -= Progress;
            }
        }
    }
}
