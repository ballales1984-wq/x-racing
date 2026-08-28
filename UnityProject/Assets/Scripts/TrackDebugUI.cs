using UnityEngine;
using System.Collections.Generic;

namespace Project0.Unity
{
    /// <summary>
    /// GUI Manager - Interfaccia per visualizzare/controllare la pista in Unity Editor
    /// </summary>
    public class TrackDebugUI : MonoBehaviour
    {
        private TrackData trackData;
        private TrackVisualizer visualizer;
        private TrackValidator validator;
        private TrackEditor editor;

        private void Start()
        {
            // Carica track
            TextAsset trackJson = Resources.Load<TextAsset>("track_parametric_data");
            if (trackJson != null)
            {
                trackData = JsonUtility.FromJson<TrackData>(trackJson.text);

                // Inizializza componenti
                visualizer = gameObject.AddComponent<TrackVisualizer>();
                visualizer.Initialize(trackData);

                validator = gameObject.AddComponent<TrackValidator>();
                editor = gameObject.AddComponent<TrackEditor>();

                // Popola il data interno del validator cosi DetermineTrackDirection funzioni.
                validator.ValidateTrack(trackData);
            }
            else
            {
                Debug.LogWarning("TrackDebugUI: 'track_parametric_data' non trovato in Resources.");
            }
        }

        private void OnGUI()
        {
            if (trackData == null) return;

            GUILayout.BeginArea(new Rect(10, 10, 400, 600));

            GUILayout.Label("=== TRACK DEBUG ===", new GUIStyle(GUI.skin.label) { fontSize = 16, fontStyle = FontStyle.Bold });

            GUILayout.Space(10);
            GUILayout.Label($"Segments: {trackData.segments.Count}");
            GUILayout.Label($"Waypoints: {trackData.waypoints.Count}");
            GUILayout.Label($"Marshal zones: {trackData.marshals.Count}");

            GUILayout.Space(15);
            GUILayout.Label("=== ACTIONS ===", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

            if (GUILayout.Button("Validate Track", GUILayout.Height(30)))
            {
                validator.ValidateTrack(trackData);
            }

            if (GUILayout.Button("Auto Fix Track", GUILayout.Height(30)))
            {
                editor.AutoFixTrack(trackData);
            }

            if (GUILayout.Button("Show Track Path in Scene", GUILayout.Height(30)))
            {
                VisualizeTrackPath();
            }

            GUILayout.Space(15);
            GUILayout.Label("=== START/FINISH ===", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

            if (trackData.segments.Count > 0)
                GUILayout.Label($"Position: {trackData.segments[0].start_pos}");

            if (GUILayout.Button("Align Pit Lane to Track", GUILayout.Height(30)))
            {
                AlignPitLane();
            }

            GUILayout.Space(15);
            GUILayout.Label("=== TRACK DIRECTION ===", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

            TrackDirection dir = validator.DetermineTrackDirection();
            GUILayout.Label($"Direction: {dir}");

            if (dir == TrackDirection.CounterClockwise)
                GUILayout.Label("(Counter-Clockwise - OK for most circuits)");
            else if (dir == TrackDirection.Clockwise)
                GUILayout.Label("(Clockwise - verify this is intended)");

            GUILayout.Space(15);
            GUILayout.Label("=== SEGMENT INFO ===", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

            for (int i = 0; i < Mathf.Min(trackData.segments.Count, 5); i++)
            {
                TrackSegment seg = trackData.segments[i];
                float length = Vector3.Distance(seg.start_pos, seg.end_pos);
                GUILayout.Label($"[{i}] {seg.type} - {length:F0}m");
            }

            if (trackData.segments.Count > 5)
                GUILayout.Label($"... and {trackData.segments.Count - 5} more segments");

            GUILayout.EndArea();
        }

        private void VisualizeTrackPath()
        {
            // Crea GameObject per visualizzare il path
            GameObject pathVisualizer = new GameObject("TrackPath");
            LineRenderer lr = pathVisualizer.AddComponent<LineRenderer>();

            List<Vector3> pathPoints = new List<Vector3>();

            foreach (TrackSegment seg in trackData.segments)
            {
                pathPoints.Add(seg.start_pos);
            }
            if (trackData.segments.Count > 0)
                pathPoints.Add(trackData.segments[0].start_pos); // Chiudi circuito

            lr.positionCount = pathPoints.Count;
            lr.SetPositions(pathPoints.ToArray());

            lr.material = new Material(Shader.Find("Sprites/Default"));
            lr.startColor = Color.green;
            lr.endColor = Color.green;
            lr.startWidth = 2f;
            lr.endWidth = 2f;
        }

        private void AlignPitLane()
        {
            if (trackData.pitLane == null || trackData.segments.Count == 0) return;

            // Pit lane entry dovrebbe essere vicino a start/finish
            TrackSegment firstSeg = trackData.segments[0];

            // Posiziona pit entry 50m prima di start/finish
            Vector3 toStart = (firstSeg.start_pos - trackData.pitLane.entry_pos).normalized;
            trackData.pitLane.entry_pos = firstSeg.start_pos - toStart * 50f;

            Debug.Log("✓ Pit lane entry aligned");
        }
    }
}
