using UnityEditor;
using UnityEngine;
using System.Diagnostics;
using System.IO;

namespace Project0.Unity.Setup
{
    public static class GenerateTelemetry
    {
        [MenuItem("Project0/Generate Telemetry")]
        public static void Generate()
        {
            var exePath = @"D:\x-racing\build\game\Release\auto_drive.exe";
            if (!File.Exists(exePath))
            {
                UnityEngine.Debug.LogError($"auto_drive.exe not found at {exePath}");
                return;
            }

            var csvPath = @"D:\x-racing\data\telemetry\unity_state.csv";
            var dir = Path.GetDirectoryName(csvPath);
            if (!Directory.Exists(dir))
            {
                Directory.CreateDirectory(dir);
            }

            UnityEngine.Debug.Log($"Starting auto_drive.exe...");
            var process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = exePath,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true
                }
            };

            process.Start();
            bool exited = process.WaitForExit(30000);

            if (!exited)
            {
                UnityEngine.Debug.LogWarning("auto_drive.exe did not finish within 30 seconds. Telemetry may be incomplete.");
                process.Kill();
                return;
            }

            UnityEngine.Debug.Log($"auto_drive.exe exited with code {process.ExitCode}");

            if (process.ExitCode == 0)
            {
                UnityEngine.Debug.Log($"Telemetry generated successfully: {csvPath}");
            }
            else
            {
                var error = process.StandardError.ReadToEnd();
                UnityEngine.Debug.LogError($"auto_drive.exe failed with code {process.ExitCode}: {error}");
            }
        }
    }
}
