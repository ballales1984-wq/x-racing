using UnityEngine;
using Newtonsoft.Json;
using System.IO;

/// <summary>
/// Track Setup Manager - Punto di ingresso per controllare/debuggare la pista
/// Allega questo script a un GameObject vuoto nella scena
/// </summary>
public class TrackSetupManager : MonoBehaviour
{
    [Header("Track File")]
    [SerializeField] private string trackJsonFilename = "track_parametric_data.json";

    [Header("Debug Visualization")]
    [SerializeField] private bool enableDirectionArrows = true;
    [SerializeField] private bool enableWaypointDisplay = true;
    [SerializeField] private bool enableSegmentEndpoints = true;
    [SerializeField] private bool enablePitLaneVisualization = true;
    [SerializeField] private bool enableMarshalZones = true;

    [Header("Auto-Fix")]
    [SerializeField] private bool autoFixOnStart = false;

    // Componenti
    private TrackData trackData;
    private TrackVisualizer visualizer;
    private TrackValidator validator;
    private TrackEditor editor;
    private TrackDirection detectedDirection = TrackDirection.Unknown;

    // UI
    private bool showDebugUI = true;
    private bool showSegmentList = false;
    private bool showWaypointList = false;
    private Vector2 segmentScrollPos = Vector2.zero;
    private Vector2 waypointScrollPos = Vector2.zero;

    private void Start()
    {
        LoadTrack();

        if (trackData != null)
        {
            InitializeComponents();

            if (autoFixOnStart)
                AutoFixTrack();
            else
                ValidateTrack();
        }
    }

    /// <summary>
    /// Carica il file JSON della pista
    /// </summary>
    private void LoadTrack()
    {
        // Prova diversi percorsi
        string[] possiblePaths = new string[]
        {
            Path.Combine(Application.streamingAssetsPath, trackJsonFilename),
            Path.Combine(Application.persistentDataPath, trackJsonFilename),
            trackJsonFilename
        };

        foreach (string path in possiblePaths)
        {
            if (File.Exists(path))
            {
                try
                {
                    string json = File.ReadAllText(path);
                    trackData = JsonConvert.DeserializeObject<TrackData>(json);

                    Debug.Log($"✓ Track loaded from: {path}");
                    Debug.Log($"  Name: {trackData.metadata.name}");
                    Debug.Log($"  Length: {trackData.metadata.length_km}km");
                    Debug.Log($"  Segments: {trackData.segments.Count}");
                    Debug.Log($"  Waypoints: {trackData.waypoints.Count}\n");

                    return;
                }
                catch (System.Exception e)
                {
                    Debug.LogError($"Error loading track from {path}: {e.Message}");
                }
            }
        }

        Debug.LogError($"✗ Could not find track file '{trackJsonFilename}'");
        Debug.LogError($"  Searched in:");
        foreach (string path in possiblePaths)
            Debug.LogError($"    - {path}");
    }

    /// <summary>
    /// Inizializza i componenti di visualizzazione/validazione
    /// </summary>
    private void InitializeComponents()
    {
        // Crea/ottieni visualizer
        visualizer = GetComponent<TrackVisualizer>();
        if (visualizer == null)
            visualizer = gameObject.AddComponent<TrackVisualizer>();
        visualizer.Initialize(trackData);

        // Crea/ottieni validator
        validator = GetComponent<TrackValidator>();
        if (validator == null)
            validator = gameObject.AddComponent<TrackValidator>();

        // Crea/ottieni editor
        editor = GetComponent<TrackEditor>();
        if (editor == null)
            editor = gameObject.AddComponent<TrackEditor>();

        Debug.Log("✓ Components initialized\n");
    }

    /// <summary>
    /// Valida la pista (NON modifica i dati)
    /// </summary>
    public void ValidateTrack()
    {
        if (trackData == null)
        {
            Debug.LogError("Track data not loaded");
            return;
        }

        validator.ValidateTrack(trackData);
        detectedDirection = validator.DetermineTrackDirection();
    }

    /// <summary>
    /// Corregge automaticamente i problemi della pista
    /// </summary>
    public void AutoFixTrack()
    {
        if (trackData == null)
        {
            Debug.LogError("Track data not loaded");
            return;
        }

        editor.AutoFixTrack(trackData);
        detectedDirection = validator.DetermineTrackDirection();
    }

    /// <summary>
    /// Salva la pista dopo le modifiche
    /// </summary>
    public void SaveTrack()
    {
        if (trackData == null)
        {
            Debug.LogError("Track data not loaded");
            return;
        }

        try
        {
            string path = Path.Combine(Application.persistentDataPath, $"{trackData.metadata.name}_fixed.json");
            string json = JsonConvert.SerializeObject(trackData, Formatting.Indented);
            File.WriteAllText(path, json);

            Debug.Log($"✓ Track saved to: {path}");
        }
        catch (System.Exception e)
        {
            Debug.LogError($"Error saving track: {e.Message}");
        }
    }

    private void OnGUI()
    {
        if (trackData == null) return;

        GUILayout.BeginArea(new Rect(10, 10, 500, Screen.height - 20));

        DrawMainPanel();

        GUILayout.EndArea();
    }

    private void DrawMainPanel()
    {
        // Header
        GUILayout.Label("═══ TRACK DEBUG MANAGER ═══", new GUIStyle(GUI.skin.label)
        {
            fontSize = 14,
            fontStyle = FontStyle.Bold,
            normal = { textColor = Color.yellow }
        });

        GUILayout.Space(10);

        // Track Info
        GUILayout.Label("▼ TRACK INFO", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });
        GUILayout.Label($"Name: {trackData.metadata.name}");
        GUILayout.Label($"Type: {trackData.metadata.type}");
        GUILayout.Label($"Length: {trackData.metadata.length_km:F2}km");
        GUILayout.Label($"Width: {trackData.metadata.width_m}m");
        GUILayout.Label($"Segments: {trackData.segments.Count}");
        GUILayout.Label($"Waypoints: {trackData.waypoints.Count}");

        GUILayout.Space(10);

        // Track Direction
        GUILayout.Label("▼ TRACK DIRECTION", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

        string directionColor = detectedDirection switch
        {
            TrackDirection.Clockwise => "<color=orange>⟳ CLOCKWISE</color>",
            TrackDirection.CounterClockwise => "<color=green>⟲ COUNTER-CLOCKWISE</color>",
            _ => "<color=red>UNKNOWN</color>"
        };

        GUILayout.Label(directionColor);
        GUILayout.Label("Direction shows which way the circuit loops.");
        GUILayout.Label("Most racing circuits are counter-clockwise.");

        GUILayout.Space(10);

        // START/FINISH
        GUILayout.Label("▼ START/FINISH LINE", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

        Vector3 startPos = trackData.segments[0].start_pos;
        GUILayout.Label($"Position: ({startPos.x:F1}, {startPos.y:F1}, {startPos.z:F1})");
        GUILayout.Label($"Segment: {trackData.segments[0].name}");

        if (GUILayout.Button("Place Camera at Start/Finish", GUILayout.Height(25)))
            PlaceCameraAtStartFinish();

        GUILayout.Space(10);

        // PIT LANE
        GUILayout.Label("▼ PIT LANE", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

        PitLane pit = trackData.pitLane;
        GUILayout.Label($"Entry: ({pit.entry_pos.x:F1}, {pit.entry_pos.y:F1}, {pit.entry_pos.z:F1})");
        GUILayout.Label($"Exit: ({pit.exit_pos.x:F1}, {pit.exit_pos.y:F1}, {pit.exit_pos.z:F1})");
        GUILayout.Label($"Pit Boxes: {pit.pit_box_positions.Count}");
        GUILayout.Label($"Speed Limit: {pit.speed_limit_kmh} km/h");

        GUILayout.Space(10);

        // ACTIONS
        GUILayout.Label("▼ ACTIONS", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

        if (GUILayout.Button("VALIDATE TRACK", GUILayout.Height(30)))
            ValidateTrack();

        if (GUILayout.Button("AUTO FIX TRACK", GUILayout.Height(30)))
        {
            if (EditorUtility.DisplayDialog("Confirm", "This will modify track data. Continue?", "Yes", "Cancel"))
                AutoFixTrack();
        }

        if (GUILayout.Button("SAVE TRACK", GUILayout.Height(30)))
            SaveTrack();

        GUILayout.Space(10);

        // VISUALIZATION TOGGLES
        GUILayout.Label("▼ VISUALIZATION", new GUIStyle(GUI.skin.label) { fontStyle = FontStyle.Bold });

        enableDirectionArrows = GUILayout.Toggle(enableDirectionArrows, "Show Direction Arrows");
        enableSegmentEndpoints = GUILayout.Toggle(enableSegmentEndpoints, "Show Segment Endpoints");
        enableWaypointDisplay = GUILayout.Toggle(enableWaypointDisplay, "Show Waypoints");
        enablePitLaneVisualization = GUILayout.Toggle(enablePitLaneVisualization, "Show Pit Lane");
        enableMarshalZones = GUILayout.Toggle(enableMarshalZones, "Show Marshal Zones");

        GUILayout.Space(10);

        // SEGMENT LIST
        if (GUILayout.Button(showSegmentList ? "▼ SEGMENTS" : "► SEGMENTS", GUILayout.Height(25)))
            showSegmentList = !showSegmentList;

        if (showSegmentList)
        {
            segmentScrollPos = GUILayout.BeginScrollView(segmentScrollPos, GUILayout.Height(200));

            foreach (TrackSegment seg in trackData.segments)
            {
                GUILayout.BeginHorizontal(GUI.skin.box);

                GUILayout.Label($"[{seg.id}] {seg.name}", GUILayout.Width(200));
                GUILayout.Label($"{seg.type}", GUILayout.Width(80));
                GUILayout.Label($"{seg.length_m:F0}m", GUILayout.Width(50));

                GUILayout.EndHorizontal();
            }

            GUILayout.EndScrollView();
        }

        GUILayout.Space(10);

        // WAYPOINT LIST
        if (GUILayout.Button(showWaypointList ? "▼ WAYPOINTS" : "► WAYPOINTS", GUILayout.Height(25)))
            showWaypointList = !showWaypointList;

        if (showWaypointList)
        {
            waypointScrollPos = GUILayout.BeginScrollView(waypointScrollPos, GUILayout.Height(200));

            foreach (Waypoint wp in trackData.waypoints)
            {
                string flags = "";
                if (wp.brake_point) flags += "🛑 ";
                if (wp.is_apex) flags += "🎯 ";

                GUILayout.Label($"[{wp.id}] Seg{wp.segment_id} {wp.target_speed_kmh}km/h {flags}");
            }

            GUILayout.EndScrollView();
        }
    }

    private void PlaceCameraAtStartFinish()
    {
        Camera cam = Camera.main;
        if (cam != null)
        {
            Vector3 startPos = trackData.segments[0].start_pos;
            cam.transform.position = startPos + Vector3.up * 10 + Vector3.back * 30;
            cam.transform.LookAt(startPos + Vector3.up * 5);

            Debug.Log("✓ Camera placed at start/finish");
        }
    }
}

/// <summary>
/// Utility per fare debug delle coordinate in runtime
/// </summary>
public class TrackDebugGizmos : MonoBehaviour
{
    public TrackData trackData;

    private void OnDrawGizmos()
    {
        if (trackData == null) return;

        // Disegna linea del circuito con start/finish in rosso
        for (int i = 0; i < trackData.segments.Count; i++)
        {
            TrackSegment seg = trackData.segments[i];

            if (i == 0)
            {
                Gizmos.color = Color.red;
                Gizmos.DrawWireSphere(seg.start_pos, 2f);
            }

            Gizmos.color = Color.white;
            Gizmos.DrawLine(seg.start_pos, seg.end_pos);
        }

        // Disegna pit lane in giallo
        Gizmos.color = Color.yellow;
        Gizmos.DrawLine(trackData.pitLane.entry_pos, trackData.pitLane.exit_pos);
    }
}

#if UNITY_EDITOR
using UnityEditor;

[CustomEditor(typeof(TrackSetupManager))]
public class TrackSetupManagerEditor : Editor
{
    public override void OnInspectorGUI()
    {
        DrawDefaultInspector();

        TrackSetupManager manager = (TrackSetupManager)target;

        GUILayout.Space(20);
        GUILayout.Label("Quick Actions", EditorStyles.boldLabel);

        if (GUILayout.Button("Validate Track", GUILayout.Height(35)))
            manager.ValidateTrack();

        if (GUILayout.Button("Auto Fix Track", GUILayout.Height(35)))
            manager.AutoFixTrack();

        if (GUILayout.Button("Save Track", GUILayout.Height(35)))
            manager.SaveTrack();
    }
}
#endif