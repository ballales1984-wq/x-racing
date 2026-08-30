using UnityEditor;
using UnityEngine;
using System.Diagnostics;
using System.IO;

namespace Project0.Unity.Setup
{
    public static class GenerateTelemetry
    {
        [MenuItem("X-Racing/Generate Telemetry")]
        public static void Generate()
        {
            string unityProjectDir = System.IO.Directory.GetParent(Application.dataPath).FullName;
            string projectRoot = System.IO.Directory.GetParent(unityProjectDir).FullName;
            string[] buildDirs = { "build", "build2", "build3", "build4" };
            string[] exeSubPaths = { "game/Release/gen_telemetry.exe", "game/gen_telemetry.exe" };

            string exePath = null;
            foreach (var bd in buildDirs)
            {
                foreach (var sp in exeSubPaths)
                {
                    string candidate = System.IO.Path.Combine(projectRoot, bd, sp);
                    if (File.Exists(candidate))
                    {
                        exePath = candidate;
                        break;
                    }
                }
                if (exePath != null) break;
            }

            if (exePath == null)
            {
                UnityEngine.Debug.LogError($"gen_telemetry.exe not found in any build directory under: {projectRoot}\nBuild the C++ project first (cmake --build build --target gen_telemetry)");
                return;
            }

            var csvPath = System.IO.Path.Combine(projectRoot, "data", "telemetry", "unity_state.csv");
            var dir = Path.GetDirectoryName(csvPath);
            if (!Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }

            UnityEngine.Debug.Log($"Starting gen_telemetry.exe...");
            var process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = exePath,
                    WorkingDirectory = projectRoot,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true
                }
            };

            process.Start();
            bool exited = process.WaitForExit(120000);

            if (!exited)
            {
                UnityEngine.Debug.Log($"gen_telemetry.exe did not finish within 120 seconds. Telemetry may be incomplete.");
                process.Kill();
                return;
            }

            UnityEngine.Debug.Log($"gen_telemetry.exe exited with code {process.ExitCode}");

            if (process.ExitCode == 0)
            {
                UnityEngine.Debug.Log($"Telemetry generated successfully: {csvPath}");
            }
            else
            {
                var error = process.StandardError.ReadToEnd();
                UnityEngine.Debug.LogError($"gen_telemetry.exe failed with code {process.ExitCode}: {error}");
            }
        }
    }
}
