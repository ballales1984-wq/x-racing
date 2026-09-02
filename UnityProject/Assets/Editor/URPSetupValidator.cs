using System;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace Project0.Unity.Setup
{
    public static class URPSetupValidator
    {
        [MenuItem("X-Racing/Validate URP Setup")]
        public static void ValidateURPSetup()
        {
            bool hasErrors = false;

            var urpType = Type.GetType("UnityEngine.Rendering.Universal.UniversalRenderPipeline, Unity.RenderPipelines.Universal.Runtime");
            if (urpType == null)
            {
                Debug.LogError("[X-Racing] URP package is not installed or not loaded. Install com.unity.render-pipelines.universal via Package Manager.");
                hasErrors = true;
            }

            var litShader = Shader.Find("Universal Render Pipeline/Lit");
            if (litShader == null)
            {
                Debug.LogError("[X-Racing] URP Lit shader not found. The URP package may be missing or corrupted.");
                hasErrors = true;
            }
            else
            {
                Debug.Log("[X-Racing] URP Lit shader found: " + litShader.name);
            }

            var pipelineAsset = GraphicsSettings.currentRenderPipeline as UniversalRenderPipelineAsset;
            if (pipelineAsset == null)
            {
                Debug.LogError("[X-Racing] No active URP Pipeline Asset assigned to GraphicsSettings. Assign a UniversalRenderPipelineAsset in Project Settings > Graphics.");
                hasErrors = true;
            }
            else
            {
                Debug.Log("[X-Racing] Active URP Pipeline Asset: " + pipelineAsset.name);
            }

            var globalSettingsPath = "Assets/UniversalRenderPipelineGlobalSettings.asset";
            var globalSettingsAsset = AssetDatabase.LoadAssetAtPath<ScriptableObject>(globalSettingsPath);
            if (globalSettingsAsset == null)
            {
                Debug.LogError("[X-Racing] UniversalRenderPipelineGlobalSettings not found at " + globalSettingsPath + ". Create one via right-click > Create > Rendering > URP Global Settings.");
                hasErrors = true;
            }
            else
            {
                Debug.Log("[X-Racing] URP Global Settings found: " + globalSettingsAsset.name);
            }

            if (!hasErrors)
            {
                Debug.Log("[X-Racing] URP setup appears valid.");
            }
            else
            {
                Debug.LogWarning("[X-Racing] URP setup has issues. Fix the errors above, then re-run this validator.");
            }
        }
    }
}
