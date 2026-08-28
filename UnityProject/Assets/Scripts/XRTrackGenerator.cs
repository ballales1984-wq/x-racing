using UnityEngine;
using System.Collections.Generic;

namespace Project0.Unity
{
    public enum TrackType { Default = 0, PitCircuit = 1 }

    public class XRTrackGenerator : MonoBehaviour
    {
        [Header("Track Settings")]
        public TrackType trackType = TrackType.Default;
        public float defaultWidth = 12f;
        public float boxLaneWidth = 3.5f;
        public int segmentsPerStraight = 150;
        public int segmentsPerCurve = 75;

        [Header("Materials")]
        public Material trackMaterial;
        public Material borderMaterial;
        public Material grassMaterial;
        public Material startLineMaterial;
        public Material boxLaneMaterial;
        public Material pitBoxMaterial;

        [Header("Pit Boxes")]
        public float pitBoxLength = 6f;
        public float pitBoxWidth = 3f;
        public float pitBoxHeight = 3f;

        private Mesh trackMesh;
        private Mesh borderLeftMesh;
        private Mesh borderRightMesh;
        private Mesh boxLaneMesh;
        private Mesh pitBoxMesh;
        private Mesh grassMesh;

        void Start()
        {
            try
            {
                GenerateTrack();
            }
            catch (System.Exception e)
            {
                Debug.LogError($"Track generation failed: {e}");
            }
        }

        public void GenerateTrack()
        {
            var trackObj = GameObject.Find("Track");
            if (trackObj == null)
            {
                trackObj = new GameObject("Track");
            }

            var pointData = GenerateTrackPoints();
            generatedPoints_ = pointData;
            var widths = new List<float>(pointData.Count);
            foreach (var pd in pointData) widths.Add(pd.width);

            BuildTrackMesh(trackObj, pointData, widths);
            CreateGrass();
            CreateBoxLane(trackObj, pointData);
            CreatePitBoxes(trackObj, pointData);
            CreateStartLine(pointData);
            CreateCheckpoints(pointData, widths);

            cachedTrackData_ = null;

            Debug.Log($"Track generated ({trackType}): {pointData.Count} points, length ~{CalculateLength(pointData):F0}m");
        }

        // Last generated centerline, used by the TrackVisualizer gizmo overlay.
        private List<TrackPointData> generatedPoints_;
        private TrackData cachedTrackData_;

        public TrackData GetTrackData()
        {
            if (cachedTrackData_ != null) return cachedTrackData_;
            cachedTrackData_ = (generatedPoints_ != null)
                ? BuildTrackData(generatedPoints_)
                : new TrackData();
            return cachedTrackData_;
        }

        // Start/finish placement: position and travel direction of the first
        // centerline point, so the car can be spawned aligned with the track.
        public Vector3 GetStartPosition()
        {
            if (generatedPoints_ != null && generatedPoints_.Count > 0) return generatedPoints_[0].position;
            return Vector3.zero;
        }

        public float GetStartHeading()
        {
            if (generatedPoints_ == null || generatedPoints_.Count < 2) return Mathf.PI * 0.5f;
            Vector3 d = generatedPoints_[1].position - generatedPoints_[0].position;
            return Mathf.Atan2(d.x, d.z); // matches CarController forward (sin h, 0, cos h)
        }

        // Builds the dev/debug TrackData consumed by TrackVisualizer from the
        // generated centerline (segments as fine straights, pit lane from the box
        // band, apex/brake waypoints at detected corners).
        TrackData BuildTrackData(List<TrackPointData> points)
        {
            var data = new TrackData();
            for (int i = 0; i < points.Count - 1; i++)
            {
                var seg = new TrackSegment
                {
                    type = "straight",
                    start_pos = points[i].position,
                    end_pos = points[i + 1].position
                };
                data.segments.Add(seg);
            }

            int firstBand = -1, lastBand = -1;
            for (int i = 0; i < points.Count; i++)
            {
                if (points[i].hasBoxLane) { if (firstBand < 0) firstBand = i; lastBand = i; }
            }
            if (firstBand >= 0)
            {
                var fp = points[firstBand];
                var lp = points[lastBand];
                data.pitLane.entry_pos = fp.position + fp.normal * (fp.width * 0.5f + fp.boxLaneWidth * 0.5f);
                data.pitLane.exit_pos = lp.position + lp.normal * (lp.width * 0.5f + lp.boxLaneWidth * 0.5f);
                data.pitLane.width_m = fp.boxLaneWidth;

                const int boxCount = 4;
                for (int b = 1; b <= boxCount; b++)
                {
                    int idx = firstBand + (lastBand - firstBand) * b / (boxCount + 1);
                    var p = points[idx];
                    data.pitLane.pit_box_positions.Add(new PitBox
                    {
                        pos = p.position + p.normal * (p.width * 0.5f + p.boxLaneWidth * 0.5f)
                    });
                }
            }

            int apexCount = 0;
            float prevHeading = (points.Count > 1) ? HeadingOf(points[0], points[1]) : 0f;
            for (int i = 1; i < points.Count - 1; i++)
            {
                float h = HeadingOf(points[i], points[i + 1]);
                float delta = Mathf.Abs(Mathf.DeltaAngle(prevHeading * Mathf.Rad2Deg, h * Mathf.Rad2Deg)) * Mathf.Deg2Rad;
                if (delta > 5f * Mathf.Deg2Rad)
                {
                    data.waypoints.Add(new Waypoint
                    {
                        position = points[i].position,
                        is_apex = true,
                        corner_number = ++apexCount
                    });
                    int brakeIdx = Mathf.Max(0, i - 3);
                    data.waypoints.Add(new Waypoint
                    {
                        position = points[brakeIdx].position,
                        brake_point = true
                    });
                }
                prevHeading = h;
            }
            return data;
        }

        static float HeadingOf(TrackPointData a, TrackPointData b)
        {
            Vector3 d = b.position - a.position;
            return Mathf.Atan2(d.x, d.z);
        }

        struct TrackPointData
        {
            public Vector3 position;
            public Vector3 tangent;
            public Vector3 normal;
            public float width;
            public float boxLaneWidth;
            public bool hasBoxLane;
            public float distance;
        }

        List<TrackPointData> GenerateTrackPoints()
        {
            var points = new List<TrackPointData>();
            bool isPit = trackType == TrackType.PitCircuit;

            float straightLength = isPit ? 500f : 300f;
            float curveRadius = isPit ? 90f : 100f;
            float mainWidth = defaultWidth;
            float boxLaneW = boxLaneWidth;

            // Section 1: main straight heading east (0,0) -> (L,0).
            Vector3 t1 = Vector3.right;
            Vector3 n1 = Vector3.forward; // left of +x
            AppendStraight(points, Vector3.zero, t1, n1, straightLength, segmentsPerStraight,
                mainWidth, (!isPit ? boxLaneW : 0f), !isPit);

            // Section 2: right-hand semicircle, center (L, -R), clockwise.
            Vector3 center2 = new Vector3(straightLength, 0f, -curveRadius);
            AppendRightSemicircle(points, center2, curveRadius, segmentsPerCurve,
                mainWidth, 0f, false);

            // Section 3: return straight heading west (L,-2R) -> (0,-2R).
            Vector3 pos3 = new Vector3(straightLength, 0f, -2f * curveRadius);
            Vector3 t3 = Vector3.left;
            Vector3 n3 = Vector3.back; // left of -x
            bool thirdHasBox = isPit; // PitCircuit: dedicated pit straight with box lane
            float thirdBoxLaneW = isPit ? boxLaneW : 0f;
            AppendStraight(points, pos3, t3, n3, straightLength, segmentsPerStraight,
                mainWidth, thirdBoxLaneW, thirdHasBox);

            // Section 4: left-hand semicircle, center (0, -R), counter-clockwise.
            Vector3 center4 = new Vector3(0f, 0f, -curveRadius);
            AppendLeftSemicircle(points, center4, curveRadius, segmentsPerCurve,
                mainWidth, 0f, false);

            // Compute cumulative arc-length distance for each sampled point.
            float dist = 0f;
            if (points.Count > 0)
            {
                var first = points[0];
                first.distance = 0f;
                points[0] = first;
                for (int i = 1; i < points.Count; i++)
                {
                    dist += Vector3.Distance(points[i].position, points[i - 1].position);
                    var pd = points[i];
                    pd.distance = dist;
                    points[i] = pd;
                }
            }

            return points;
        }

        void AppendStraight(List<TrackPointData> points, Vector3 start, Vector3 tangent, Vector3 normal,
            float length, int segments, float width, float boxLaneW, bool hasBoxLane)
        {
            for (int i = 0; i <= segments; i++)
            {
                float t = (float)i / segments;
                var pd = new TrackPointData();
                pd.position = start + tangent * (t * length);
                pd.tangent = tangent;
                pd.normal = normal;
                pd.width = width;
                pd.boxLaneWidth = boxLaneW;
                pd.hasBoxLane = hasBoxLane;
                pd.distance = 0f;
                points.Add(pd);
            }
        }

        void AppendRightSemicircle(List<TrackPointData> points, Vector3 center, float radius, int segments,
            float width, float boxLaneW, bool hasBoxLane)
        {
            // Clockwise (right turn) from top of circle to bottom.
            // Angle sweeps from +90 deg to -90 deg passing through 0 (right side).
            for (int i = 1; i <= segments; i++)
            {
                float t = (float)i / segments;
                float angle = Mathf.PI / 2f - t * Mathf.PI; // +90 -> -90 deg
                float cx = Mathf.Cos(angle);
                float sx = Mathf.Sin(angle);
                var pd = new TrackPointData();
                pd.position = center + new Vector3(cx, 0f, sx) * radius;
                // Direction of motion as angle decreases: (sin, 0, -cos).
                pd.tangent = new Vector3(sx, 0f, -cx).normalized;
                // Left of tangent (outward side for a right turn is -x; here left points inward/up-right).
                pd.normal = new Vector3(cx, 0f, sx);
                pd.width = width;
                pd.boxLaneWidth = boxLaneW;
                pd.hasBoxLane = hasBoxLane;
                pd.distance = 0f;
                points.Add(pd);
            }
        }

        void AppendLeftSemicircle(List<TrackPointData> points, Vector3 center, float radius, int segments,
            float width, float boxLaneW, bool hasBoxLane)
        {
            // Counter-clockwise (left turn) from bottom of circle to top, bulging to -x.
            // Angle sweeps from -90 deg to -270 deg (clockwise in math space, but a left
            // turn on the track because the car arrives going west and departs going east).
            for (int i = 1; i <= segments; i++)
            {
                float t = (float)i / segments;
                float angle = -Mathf.PI / 2f - t * Mathf.PI; // -90 -> -270 deg
                float cx = Mathf.Cos(angle);
                float sx = Mathf.Sin(angle);
                var pd = new TrackPointData();
                pd.position = center + new Vector3(cx, 0f, sx) * radius;
                pd.tangent = new Vector3(sx, 0f, -cx).normalized;
                pd.normal = new Vector3(cx, 0f, sx);
                pd.width = width;
                pd.boxLaneWidth = boxLaneW;
                pd.hasBoxLane = hasBoxLane;
                pd.distance = 0f;
                points.Add(pd);
            }
        }

        // Sample a track point (position/tangent/normal/width/meta) at a given arc-length.
        TrackPointData SampleAt(List<TrackPointData> points, float distance)
        {
            if (points == null || points.Count == 0) return new TrackPointData();
            if (points.Count == 1) return points[0];

            float len = points[points.Count - 1].distance;
            distance = distance % len;
            if (distance < 0f) distance += len;

            int lo = 0, hi = points.Count - 1;
            while (lo < hi)
            {
                int mid = lo + (hi - lo) / 2;
                if (points[mid].distance < distance) lo = mid + 1;
                else hi = mid;
            }
            int i1 = Mathf.Clamp(lo, 0, points.Count - 1);
            int i0 = Mathf.Clamp(i1 - 1, 0, points.Count - 1);

            float seg = points[i1].distance - points[i0].distance;
            float frac = seg > 1e-5f ? (distance - points[i0].distance) / seg : 0f;
            frac = Mathf.Clamp01(frac);

            var a = points[i0];
            var b = points[i1];
            var pd = new TrackPointData();
            pd.position = Vector3.Lerp(a.position, b.position, frac);
            pd.tangent = Vector3.Lerp(a.tangent, b.tangent, frac).normalized;
            pd.normal = new Vector3(-pd.tangent.z, 0f, pd.tangent.x);
            pd.width = Mathf.Lerp(a.width, b.width, frac);
            pd.boxLaneWidth = Mathf.Lerp(a.boxLaneWidth, b.boxLaneWidth, frac);
            pd.hasBoxLane = a.hasBoxLane && b.hasBoxLane;
            pd.distance = distance;
            return pd;
        }

        void BuildTrackMesh(GameObject parent, List<TrackPointData> points, List<float> widths)
        {
            var trackGO = GameObject.Find("TrackMesh") ?? new GameObject("TrackMesh");
            trackGO.transform.SetParent(parent.transform);

            var mf = trackGO.GetComponent<MeshFilter>();
            if (mf == null) mf = trackGO.AddComponent<MeshFilter>();
            var mr = trackGO.GetComponent<MeshRenderer>();
            if (mr == null) mr = trackGO.AddComponent<MeshRenderer>();
            mr.sharedMaterial = trackMaterial != null ? trackMaterial : CreateDefaultMaterial(new Color(0.35f, 0.35f, 0.35f));

            if (trackMesh == null) trackMesh = new Mesh();
            trackMesh.Clear();

            var vertices = new List<Vector3>();
            var triangles = new List<int>();
            var uvs = new List<Vector2>();

            for (int i = 0; i < points.Count - 1; i++)
            {
                var p0 = points[i];
                var p1 = points[i + 1];
                float width = widths[i];
                Vector3 right = p0.normal;
                Vector3 v0 = p0.position + right * width * 0.5f;
                Vector3 v1 = p0.position - right * width * 0.5f;
                Vector3 v2 = p1.position + right * width * 0.5f;
                Vector3 v3 = p1.position - right * width * 0.5f;

                int baseIndex = vertices.Count;
                vertices.Add(v0);
                vertices.Add(v1);
                vertices.Add(v2);
                vertices.Add(v3);

                triangles.Add(baseIndex);
                triangles.Add(baseIndex + 2);
                triangles.Add(baseIndex + 1);
                triangles.Add(baseIndex + 1);
                triangles.Add(baseIndex + 2);
                triangles.Add(baseIndex + 3);

                uvs.Add(new Vector2(0f, i));
                uvs.Add(new Vector2(1f, i));
                uvs.Add(new Vector2(0f, i + 1));
                uvs.Add(new Vector2(1f, i + 1));
            }

            trackMesh.vertices = vertices.ToArray();
            trackMesh.triangles = triangles.ToArray();
            trackMesh.uv = uvs.ToArray();
            trackMesh.RecalculateNormals();
            trackMesh.RecalculateBounds();
            mf.mesh = trackMesh;

            var collider = trackGO.GetComponent<MeshCollider>();
            if (collider == null) collider = trackGO.gameObject.AddComponent<MeshCollider>();
            collider.sharedMesh = trackMesh;

            CreateBorders(parent, points, widths);
        }

        void CreateBorders(GameObject parent, List<TrackPointData> points, List<float> widths)
        {
            CreateBorder(parent, "BorderLeft", points, widths, 1f, new Color(0.8f, 0.2f, 0f));
            CreateBorder(parent, "BorderRight", points, widths, -1f, new Color(0.8f, 0.2f, 0f));
        }

        void CreateBorder(GameObject parent, string name, List<TrackPointData> points, List<float> widths, float side, Color color)
        {
            var go = GameObject.Find(name) ?? new GameObject(name);
            go.transform.SetParent(parent.transform);

            var mf = go.GetComponent<MeshFilter>();
            if (mf == null) mf = go.AddComponent<MeshFilter>();
            var mr = go.GetComponent<MeshRenderer>();
            if (mr == null) mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = borderMaterial != null ? borderMaterial : CreateDefaultMaterial(color);

            if (side > 0 && borderLeftMesh == null) borderLeftMesh = new Mesh();
            if (side < 0 && borderRightMesh == null) borderRightMesh = new Mesh();
            var mesh = side > 0 ? borderLeftMesh : borderRightMesh;
            mesh.Clear();

            var vertices = new List<Vector3>();
            var triangles = new List<int>();
            float borderWidth = 0.5f;
            float borderHeight = 0.5f;

            for (int i = 0; i < points.Count - 1; i++)
            {
                var p0 = points[i];
                var p1 = points[i + 1];
                float width = widths[i];
                Vector3 right = p0.normal;
                float outerWidth = width * 0.5f + borderWidth;

                Vector3 outer = p0.position + right * outerWidth * side;
                Vector3 inner = p0.position + right * width * 0.5f * side;

                int baseIndex = vertices.Count;
                vertices.Add(outer);
                vertices.Add(inner);
                vertices.Add(outer + Vector3.up * borderHeight);
                vertices.Add(inner + Vector3.up * borderHeight);

                if (side > 0)
                {
                    triangles.Add(baseIndex);
                    triangles.Add(baseIndex + 1);
                    triangles.Add(baseIndex + 2);
                    triangles.Add(baseIndex + 1);
                    triangles.Add(baseIndex + 3);
                    triangles.Add(baseIndex + 2);
                }
                else
                {
                    triangles.Add(baseIndex);
                    triangles.Add(baseIndex + 2);
                    triangles.Add(baseIndex + 1);
                    triangles.Add(baseIndex + 1);
                    triangles.Add(baseIndex + 2);
                    triangles.Add(baseIndex + 3);
                }
            }

            mesh.vertices = vertices.ToArray();
            mesh.triangles = triangles.ToArray();
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            mf.mesh = mesh;
        }

        // Draw the pit/box lane as a distinct red band laid down outside the track edge
        // on every straight segment where hasBoxLane is true. Mirrors the C++ box lane
        // rendered by the track diagram / GDI renderer.
        void CreateBoxLane(GameObject parent, List<TrackPointData> points)
        {
            var go = GameObject.Find("BoxLane");
            if (go != null) Destroy(go);
            go = new GameObject("BoxLane");
            go.transform.SetParent(parent.transform);

            var mf = go.AddComponent<MeshFilter>();
            var mr = go.AddComponent<MeshRenderer>();
            // Distinct blue so the pit lane is clearly different from the racing surface.
            mr.sharedMaterial = boxLaneMaterial != null
                ? boxLaneMaterial
                : CreateDefaultMaterial(new Color(0.2f, 0.4f, 0.9f));

            if (boxLaneMesh == null) boxLaneMesh = new Mesh();
            boxLaneMesh.Clear();

            var vertices = new List<Vector3>();
            var triangles = new List<int>();
            bool inBand = false;

            int firstBandIndex = -1, lastBandIndex = -1;
            Vector3 entryPos = Vector3.zero, entryTangent = Vector3.right, entryNormal = Vector3.forward, entryOffset = Vector3.zero;
            Vector3 exitPos = Vector3.zero, exitTangent = Vector3.right, exitNormal = Vector3.forward, exitOffset = Vector3.zero;

            for (int i = 0; i < points.Count - 1; i++)
            {
                var p0 = points[i];
                var p1 = points[i + 1];
                if (!p0.hasBoxLane || !p1.hasBoxLane)
                {
                    inBand = false;
                    continue;
                }

                if (firstBandIndex < 0)
                {
                    firstBandIndex = i;
                    entryPos = p0.position;
                    entryTangent = p0.tangent;
                    entryNormal = p0.normal;
                    entryOffset = p0.normal * (p0.width * 0.5f + p0.boxLaneWidth * 0.5f);
                }
                lastBandIndex = i;
                exitPos = p1.position;
                exitTangent = p1.tangent;
                exitNormal = p1.normal;
                exitOffset = p1.normal * (p1.width * 0.5f + p1.boxLaneWidth * 0.5f);

                float halfW = p0.width * 0.5f;
                float boxLaneW = p0.boxLaneWidth;
                Vector3 right = p0.normal;

                Vector3 outer0 = p0.position + right * (halfW + boxLaneW);
                Vector3 inner0 = p0.position + right * halfW;
                Vector3 outer1 = p1.position + right * (halfW + boxLaneW);
                Vector3 inner1 = p1.position + right * halfW;

                if (!inBand)
                {
                    // Start a fresh band; emit a degenerate triangle to stitch the seam.
                    int baseIndex = vertices.Count;
                    vertices.Add(outer0); vertices.Add(inner0);
                    vertices.Add(outer0); vertices.Add(inner0);
                    triangles.Add(baseIndex); triangles.Add(baseIndex + 2); triangles.Add(baseIndex + 1);
                    inBand = true;
                }

                int baseIndex2 = vertices.Count;
                vertices.Add(outer0);
                vertices.Add(inner0);
                vertices.Add(outer1);
                vertices.Add(inner1);

                triangles.Add(baseIndex2);
                triangles.Add(baseIndex2 + 2);
                triangles.Add(baseIndex2 + 1);
                triangles.Add(baseIndex2 + 1);
                triangles.Add(baseIndex2 + 2);
                triangles.Add(baseIndex2 + 3);
            }

            if (vertices.Count > 0)
            {
                boxLaneMesh.vertices = vertices.ToArray();
                boxLaneMesh.triangles = triangles.ToArray();
                boxLaneMesh.RecalculateNormals();
            }
            boxLaneMesh.RecalculateBounds();
            mf.mesh = boxLaneMesh;

            if (firstBandIndex >= 0)
            {
                // Green ENTRY marker: where the pit lane begins.
                MakeFlatMarker(go, "PitEntry", entryPos + entryOffset, entryTangent, entryNormal,
                    5f, 4f, 0.04f, CreateDefaultMaterial(new Color(0.1f, 0.8f, 0.2f)));
            }
            if (lastBandIndex >= 0)
            {
                // Red EXIT marker: where the pit lane ends.
                MakeFlatMarker(go, "PitExit", exitPos + exitOffset, exitTangent, exitNormal,
                    5f, 4f, 0.04f, CreateDefaultMaterial(new Color(0.9f, 0.1f, 0.1f)));
            }
        }

        // Place individual pit boxes alongside the box lane, mirroring the C++ pit box
        // layout (arc-length positions along the track centerline).
        void CreatePitBoxes(GameObject parent, List<TrackPointData> points)
        {
            var boxPositions = GetPitBoxPositions();
            if (boxPositions.Length == 0) return;

            var go = GameObject.Find("PitBoxes");
            if (go != null) Destroy(go);
            go = new GameObject("PitBoxes");
            go.transform.SetParent(parent.transform);

            if (pitBoxMesh == null) pitBoxMesh = CreateBoxMesh();

            for (int i = 0; i < boxPositions.Length; i++)
            {
                var pd = SampleAt(points, boxPositions[i]);
                float halfW = pd.width * 0.5f;
                float boxLaneW = pd.boxLaneWidth;
                float gap = 1.0f;
                Vector3 center = pd.position + pd.normal * (halfW + boxLaneW + pitBoxWidth * 0.5f + gap);
                Quaternion rot = Quaternion.LookRotation(pd.tangent, Vector3.up);

                var box = new GameObject($"PitBox_{i}");
                box.transform.SetParent(go.transform);
                box.transform.position = center;
                box.transform.rotation = rot;
                box.transform.localScale = new Vector3(pitBoxWidth, pitBoxHeight, pitBoxLength);

                var mf = box.AddComponent<MeshFilter>();
                mf.mesh = pitBoxMesh;
                var mr = box.AddComponent<MeshRenderer>();
                mr.sharedMaterial = pitBoxMaterial != null
                    ? pitBoxMaterial
                    : CreateDefaultMaterial(new Color(0.8f, 0.8f, 0.8f));
            }
        }

        float[] GetPitBoxPositions()
        {
            if (trackType == TrackType.PitCircuit)
                return new float[] { 850f, 950f, 1050f, 1150f };
            return new float[] { 50f, 100f, 150f, 200f, 250f };
        }

        Mesh CreateBoxMesh()
        {
            var mesh = new Mesh();
            // Unit cube centered at origin.
            float hx = 0.5f, hy = 0.5f, hz = 0.5f;
            var verts = new Vector3[]
            {
                new Vector3(-hx, -hy, -hz), new Vector3( hx, -hy, -hz),
                new Vector3( hx,  hy, -hz), new Vector3(-hx,  hy, -hz),
                new Vector3(-hx, -hy,  hz), new Vector3( hx, -hy,  hz),
                new Vector3( hx,  hy,  hz), new Vector3(-hx,  hy,  hz),
            };
            var tris = new int[]
            {
                0, 2, 1, 0, 3, 2,
                1, 2, 6, 1, 6, 5,
                4, 5, 6, 4, 6, 7,
                2, 3, 7, 2, 7, 6,
                0, 4, 7, 0, 7, 3,
                0, 1, 5, 0, 5, 4,
            };
            mesh.vertices = verts;
            mesh.triangles = tris;
            mesh.RecalculateNormals();
            return mesh;
        }

        void CreateGrass()
        {
            var grassObj = GameObject.Find("Grass");
            if (grassObj == null)
            {
                grassObj = new GameObject("Grass");
            }

            var mf = grassObj.GetComponent<MeshFilter>();
            if (mf == null) mf = grassObj.AddComponent<MeshFilter>();
            var mr = grassObj.GetComponent<MeshRenderer>();
            if (mr == null) mr = grassObj.AddComponent<MeshRenderer>();
            mr.sharedMaterial = grassMaterial != null ? grassMaterial : CreateDefaultMaterial(new Color(0.2f, 0.6f, 0.2f));

            if (grassMesh == null) grassMesh = new Mesh();
            grassMesh.Clear();

            var bounds = GetCurrentBounds();
            float pad = 100f;
            grassMesh.vertices = new Vector3[]
            {
                new Vector3(bounds.minX - pad, -0.1f, bounds.minZ - pad),
                new Vector3(bounds.maxX + pad, -0.1f, bounds.minZ - pad),
                new Vector3(bounds.minX - pad, -0.1f, bounds.maxZ + pad),
                new Vector3(bounds.maxX + pad, -0.1f, bounds.maxZ + pad)
            };
            grassMesh.triangles = new int[] { 0, 2, 1, 1, 2, 3 };
            grassMesh.RecalculateNormals();
            mf.mesh = grassMesh;
        }

        void CreateStartLine(List<TrackPointData> points)
        {
            var pd = points[0];
            var sfObj = GameObject.Find("StartFinishLine");
            if (sfObj == null)
            {
                sfObj = new GameObject("StartFinishLine");
            }
            else
            {
                // Clear previous markers (gantry posts) before rebuilding.
                foreach (Transform child in sfObj.transform) Destroy(child.gameObject);
            }
            sfObj.transform.SetParent(GameObject.Find("Track")?.transform);
            sfObj.transform.localPosition = Vector3.zero;
            sfObj.transform.localRotation = Quaternion.identity;

            Vector3 tangent = pd.tangent;
            Vector3 normal = pd.normal;
            float halfW = pd.width * 0.5f;
            float bandDepth = 4f;

            // Black backing so the checkered band reads clearly from a distance.
            MakeFlatMarker(sfObj, "StartBacking",
                pd.position, tangent, normal, bandDepth + 1f, pd.width + 1.5f, 0.02f,
                CreateDefaultMaterial(Color.black));
            // White foreground of the checkered band.
            MakeFlatMarker(sfObj, "StartWhite",
                pd.position, tangent, normal, bandDepth, pd.width, 0.03f,
                CreateDefaultMaterial(Color.white));

            // Two bright gantry posts at the track edges mark start/finish from afar.
            CreateGantryPost(sfObj, pd.position + normal * (halfW + 0.5f) + tangent * (bandDepth * 0.5f));
            CreateGantryPost(sfObj, pd.position - normal * (halfW + 0.5f) + tangent * (bandDepth * 0.5f));
        }

        void CreateGantryPost(GameObject parent, Vector3 pos)
        {
            var post = GameObject.CreatePrimitive(PrimitiveType.Cube);
            post.name = "StartPost";
            post.transform.SetParent(parent.transform);
            post.transform.position = pos + Vector3.up * 3f;
            post.transform.localScale = new Vector3(0.6f, 6f, 0.6f);
            var r = post.GetComponent<Renderer>();
            if (r != null) r.sharedMaterial = CreateDefaultMaterial(new Color(1f, 0.85f, 0f));
        }

        GameObject MakeFlatMarker(GameObject parent, string name, Vector3 center, Vector3 tangent, Vector3 normal,
            float lengthAlong, float widthAcross, float y, Material mat)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent.transform);
            go.transform.localPosition = Vector3.zero;
            go.transform.localRotation = Quaternion.identity;

            var mf = go.AddComponent<MeshFilter>();
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = mat;

            var mesh = new Mesh();
            Vector3 halfT = tangent * (lengthAlong * 0.5f);
            Vector3 halfW = normal * (widthAcross * 0.5f);
            Vector3 c = center + Vector3.up * y;
            mesh.vertices = new Vector3[]
            {
                c - halfT - halfW,
                c - halfT + halfW,
                c + halfT - halfW,
                c + halfT + halfW
            };
            mesh.triangles = new int[] { 0, 2, 1, 1, 2, 3 };
            mesh.RecalculateNormals();
            mf.mesh = mesh;
            return go;
        }

        void CreateCheckpoints(List<TrackPointData> points, List<float> widths)
        {
            var cpObj = GameObject.Find("Checkpoints");
            if (cpObj != null) DestroyImmediate(cpObj);
            cpObj = new GameObject("Checkpoints");
            cpObj.transform.SetParent(GameObject.Find("Track")?.transform);

            int numCheckpoints = 8;
            float spacing = (float)points.Count / numCheckpoints;

            for (int i = 0; i < numCheckpoints; i++)
            {
                int index = (int)(i * spacing);
                if (index >= points.Count) index = points.Count - 1;

                var cpTrigger = new GameObject($"Checkpoint_{i}");
                cpTrigger.transform.SetParent(cpObj.transform);
                cpTrigger.transform.position = points[index].position;

                var trigger = cpTrigger.AddComponent<CheckpointTrigger>();
                trigger.checkpointIndex = i;
                trigger.totalCheckpoints = numCheckpoints;
            }
        }

        float CalculateLength(List<TrackPointData> points)
        {
            float length = 0f;
            for (int i = 1; i < points.Count; i++)
            {
                length += Vector3.Distance(points[i].position, points[i - 1].position);
            }
            return length;
        }

        struct Bounds2
        {
            public float minX, maxX, minZ, maxZ;
        }

        Bounds2 GetCurrentBounds()
        {
            var points = GenerateTrackPoints();
            float minX = 0, maxX = 0, minZ = 0, maxZ = 0;
            bool first = true;
            foreach (var p in points)
            {
                if (first) { minX = maxX = p.position.x; minZ = maxZ = p.position.z; first = false; }
                minX = Mathf.Min(minX, p.position.x); maxX = Mathf.Max(maxX, p.position.x);
                minZ = Mathf.Min(minZ, p.position.z); maxZ = Mathf.Max(maxZ, p.position.z);
            }
            return new Bounds2 { minX = minX, maxX = maxX, minZ = minZ, maxZ = maxZ };
        }

        Material CreateDefaultMaterial(Color color)
        {
            Shader shader = Shader.Find("Standard");
            if (shader == null) shader = Shader.Find("Universal Render Pipeline/Lit");
            if (shader == null) shader = Shader.Find("Unlit/Color");
            if (shader == null) return null;

            Material mat = new Material(shader);
            mat.color = color;
            return mat;
        }
    }
}
