using UnityEngine;
using System.Collections.Generic;

namespace Project0.Unity
{
    public class XRTrackGenerator : MonoBehaviour
    {
        [Header("Track Settings")]
        public float trackLength = 5000f;
        public float defaultWidth = 12f;
        public int resolution = 500;

        [Header("Materials")]
        public Material trackMaterial;
        public Material borderMaterial;
        public Material grassMaterial;
        public Material startLineMaterial;

        private Mesh trackMesh;
        private Mesh borderLeftMesh;
        private Mesh borderRightMesh;

        void Start()
        {
            GenerateTrack();
        }

        public void GenerateTrack()
        {
            var trackObj = GameObject.Find("Track");
            if (trackObj == null)
            {
                trackObj = new GameObject("Track");
            }

            var points = GenerateTrackPoints();
            var widths = GenerateWidths(points.Count);

            BuildTrackMesh(trackObj, points, widths);
            CreateGrass();
            CreateStartLine(points, widths);
            CreateCheckpoints(points, widths);

            Debug.Log($"Track generated: {points.Count} points, length ~{CalculateLength(points):F0}m");
        }

        List<Vector3> GenerateTrackPoints()
        {
            var points = new List<Vector3>();
            float straightLength = 765f;
            float curveRadius = 75f;
            int segmentsPerStraight = 150;
            int segmentsPerCurve = 75;

            for (int i = 0; i <= segmentsPerStraight; i++)
            {
                float t = (float)i / segmentsPerStraight;
                points.Add(new Vector3(t * straightLength, 0f, 0f));
            }

            for (int i = 1; i <= segmentsPerCurve; i++)
            {
                float t = (float)i / segmentsPerCurve;
                float angle = -Mathf.PI / 2 + t * Mathf.PI;
                float x = 765f + curveRadius * Mathf.Cos(angle);
                float z = 75f + curveRadius * Mathf.Sin(angle);
                points.Add(new Vector3(x, 0f, z));
            }

            for (int i = 1; i <= segmentsPerStraight; i++)
            {
                float t = (float)i / segmentsPerStraight;
                points.Add(new Vector3(765f - t * straightLength, 0f, 150f));
            }

            for (int i = 1; i <= segmentsPerCurve; i++)
            {
                float t = (float)i / segmentsPerCurve;
                float angle = Mathf.PI / 2 + t * Mathf.PI;
                float x = curveRadius * Mathf.Cos(angle);
                float z = 75f + curveRadius * Mathf.Sin(angle);
                points.Add(new Vector3(x, 0f, z));
            }

            return points;
        }

        List<float> GenerateWidths(int count)
        {
            var widths = new List<float>(count);
            int segmentsPerStraight = 150;
            int segmentsPerCurve = 75;

            for (int i = 0; i < count; i++)
            {
                if (i <= segmentsPerStraight)
                    widths.Add(16f);
                else if (i <= segmentsPerStraight + segmentsPerCurve)
                    widths.Add(12f);
                else if (i <= segmentsPerStraight * 2 + segmentsPerCurve)
                    widths.Add(12f);
                else
                    widths.Add(12f);
            }

            return widths;
        }

        void BuildTrackMesh(GameObject parent, List<Vector3> points, List<float> widths)
        {
            var trackGO = new GameObject("TrackMesh");
            trackGO.transform.SetParent(parent.transform);

            var mf = trackGO.AddComponent<MeshFilter>();
            var mr = trackGO.AddComponent<MeshRenderer>();
            mr.sharedMaterial = trackMaterial != null ? trackMaterial : CreateDefaultMaterial(new Color(0.35f, 0.35f, 0.35f));

            var mesh = new Mesh();
            var vertices = new List<Vector3>();
            var triangles = new List<int>();
            var uvs = new List<Vector2>();

            for (int i = 0; i < points.Count - 1; i++)
            {
                Vector3 p0 = points[i];
                Vector3 p1 = points[i + 1];
                Vector3 dir = (p1 - p0).normalized;
                Vector3 right = new Vector3(-dir.z, 0f, dir.x);
                float width = widths[i];

                Vector3 v0 = p0 + right * width * 0.5f;
                Vector3 v1 = p0 - right * width * 0.5f;
                Vector3 v2 = p1 + right * width * 0.5f;
                Vector3 v3 = p1 - right * width * 0.5f;

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

                uvs.Add(new Vector2(0f, i * 1f));
                uvs.Add(new Vector2(1f, i * 1f));
                uvs.Add(new Vector2(0f, (i + 1) * 1f));
                uvs.Add(new Vector2(1f, (i + 1) * 1f));
            }

            mesh.vertices = vertices.ToArray();
            mesh.triangles = triangles.ToArray();
            mesh.uv = uvs.ToArray();
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            mf.mesh = mesh;

            var collider = trackGO.AddComponent<MeshCollider>();
            collider.sharedMesh = mesh;

            CreateBorders(parent, points, widths);
        }

        void CreateBorders(GameObject parent, List<Vector3> points, List<float> widths)
        {
            CreateBorder(parent, "BorderLeft", points, widths, 1f, new Color(0.8f, 0.2f, 0f));
            CreateBorder(parent, "BorderRight", points, widths, -1f, new Color(0.8f, 0.2f, 0f));
        }

        void CreateBorder(GameObject parent, string name, List<Vector3> points, List<float> widths, float side, Color color)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent.transform);

            var mf = go.AddComponent<MeshFilter>();
            var mr = go.AddComponent<MeshRenderer>();
            mr.sharedMaterial = borderMaterial != null ? borderMaterial : CreateDefaultMaterial(color);

            var vertices = new List<Vector3>();
            var triangles = new List<int>();
            float borderWidth = 0.5f;
            float borderHeight = 0.5f;

            for (int i = 0; i < points.Count - 1; i++)
            {
                Vector3 p0 = points[i];
                Vector3 p1 = points[i + 1];
                Vector3 dir = (p1 - p0).normalized;
                Vector3 right = new Vector3(-dir.z, 0f, dir.x);
                float width = widths[i];
                float outerWidth = width * 0.5f + borderWidth;

                Vector3 outer = p0 + right * outerWidth * side;
                Vector3 inner = p0 + right * width * 0.5f * side;

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

            var mesh = new Mesh();
            mesh.vertices = vertices.ToArray();
            mesh.triangles = triangles.ToArray();
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            mf.mesh = mesh;
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

            var mesh = new Mesh();
            mesh.vertices = new Vector3[]
            {
                new Vector3(-200f, -0.1f, -200f),
                new Vector3(1000f, -0.1f, -200f),
                new Vector3(-200f, -0.1f, 400f),
                new Vector3(1000f, -0.1f, 400f)
            };
            mesh.triangles = new int[] { 0, 2, 1, 1, 2, 3 };
            mesh.RecalculateNormals();
            mf.mesh = mesh;
        }

        void CreateStartLine(List<Vector3> points, List<float> widths)
        {
            var sfObj = GameObject.Find("StartFinishLine");
            if (sfObj == null)
            {
                sfObj = new GameObject("StartFinishLine");
            }
            sfObj.transform.SetParent(GameObject.Find("Track")?.transform);
            sfObj.transform.localPosition = Vector3.zero;
            sfObj.transform.localRotation = Quaternion.identity;

            var mf = sfObj.GetComponent<MeshFilter>();
            if (mf == null) mf = sfObj.AddComponent<MeshFilter>();
            var mr = sfObj.GetComponent<MeshRenderer>();
            if (mr == null) mr = sfObj.AddComponent<MeshRenderer>();
            mr.sharedMaterial = startLineMaterial != null ? startLineMaterial : CreateDefaultMaterial(Color.yellow);

            float lineLength = 16f;
            float lineThickness = 0.5f;
            var mesh = new Mesh();
            mesh.vertices = new Vector3[]
            {
                new Vector3(-lineLength * 0.5f, 0.01f, -lineThickness * 0.5f),
                new Vector3(-lineLength * 0.5f, 0.01f, lineThickness * 0.5f),
                new Vector3(lineLength * 0.5f, 0.01f, -lineThickness * 0.5f),
                new Vector3(lineLength * 0.5f, 0.01f, lineThickness * 0.5f)
            };
            mesh.triangles = new int[] { 0, 2, 1, 1, 2, 3 };
            mesh.uv = new Vector2[] { new Vector2(0, 0), new Vector2(0, 1), new Vector2(1, 0), new Vector2(1, 1) };
            mesh.RecalculateNormals();
            mf.mesh = mesh;
        }

        void CreateCheckpoints(List<Vector3> points, List<float> widths)
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
                cpTrigger.transform.position = points[index];

                var trigger = cpTrigger.AddComponent<CheckpointTrigger>();
                trigger.checkpointIndex = i;
                trigger.totalCheckpoints = numCheckpoints;
            }
        }

        float CalculateLength(List<Vector3> points)
        {
            float length = 0f;
            for (int i = 1; i < points.Count; i++)
            {
                length += Vector3.Distance(points[i - 1], points[i]);
            }
            return length;
        }

        Material CreateDefaultMaterial(Color color)
        {
            Shader shader = Shader.Find("Standard");
            if (shader == null) shader = Shader.Find("Universal Render Pipeline/Lit");
            if (shader == null) shader = Shader.Find("Hidden/InternalErrorShader");

            Material mat = new Material(shader);
            mat.color = color;
            return mat;
        }
    }
}
