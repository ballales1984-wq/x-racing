using UnityEditor;
using UnityEngine;
using UnityEditor.SceneManagement;

namespace Project0.Unity.Setup
{
    public static class TrackGenerator
    {
        [MenuItem("Project0/Generate Track")]
        public static void GenerateTrack()
        {
            var trackObj = GameObject.Find("Track");
            if (trackObj == null)
            {
                trackObj = new GameObject("Track");
            }

            var meshFilter = trackObj.GetComponent<MeshFilter>();
            if (meshFilter == null)
            {
                meshFilter = trackObj.AddComponent<MeshFilter>();
            }

            var meshRenderer = trackObj.GetComponent<MeshRenderer>();
            if (meshRenderer == null)
            {
                meshRenderer = trackObj.AddComponent<MeshRenderer>();
            }

            if (meshRenderer.sharedMaterial == null)
            {
                Shader shader = Shader.Find("Universal Render Pipeline/Lit");
                if (shader == null)
                {
                    shader = Shader.Find("Standard");
                }
                if (shader == null)
                {
                    shader = Shader.Find("Sprites/Default");
                }
                meshRenderer.sharedMaterial = new Material(shader);
            }
            meshRenderer.sharedMaterial.color = new Color(0.35f, 0.35f, 0.35f);

            float trackWidth = 14f;
            var points = GenerateTrackPoints();

            CreateTrackBorders(points, trackWidth);

            var lightObj = GameObject.Find("Directional Light");
            if (lightObj == null)
            {
                lightObj = new GameObject("Directional Light");
                var light = lightObj.AddComponent<Light>();
                light.type = LightType.Directional;
                light.intensity = 1f;
                lightObj.transform.rotation = Quaternion.Euler(50f, -30f, 0f);
            }

            var trackMesh = BuildTrackMesh(points, trackWidth);
            meshFilter.mesh = trackMesh;

            var collider = trackObj.GetComponent<MeshCollider>();
            if (collider == null)
            {
                collider = trackObj.AddComponent<MeshCollider>();
            }
            collider.sharedMesh = trackMesh;

            Debug.Log($"Track generated: {points.Count} points, length ~{CalculateTrackLength(points):F0}m");
        }

        private static float CalculateTrackLength(System.Collections.Generic.List<Vector3> points)
        {
            float length = 0f;
            for (int i = 1; i < points.Count; i++)
            {
                length += Vector3.Distance(points[i - 1], points[i]);
            }
            return length;
        }

        private static System.Collections.Generic.List<Vector3> GenerateTrackPoints()
        {
            var points = new System.Collections.Generic.List<Vector3>();
            float straightLength = 200f;
            float curveRadius = 75f;
            int segmentsPerStraight = 100;
            int segmentsPerCurve = 50;

            // Bottom straight: from (0, 0, 0) to (200, 0, 0)
            for (int i = 0; i <= segmentsPerStraight; i++)
            {
                float t = (float)i / segmentsPerStraight;
                points.Add(new Vector3(t * straightLength, 0f, 0f));
            }

            // Right curve: semicircle from (200, 0, 0) to (200, 0, 150)
            // Center: (200, 0, 75), radius 75
            for (int i = 1; i <= segmentsPerCurve; i++)
            {
                float t = (float)i / segmentsPerCurve;
                float angle = -Mathf.PI / 2 + t * Mathf.PI;
                float x = 200f + curveRadius * Mathf.Cos(angle);
                float z = 75f + curveRadius * Mathf.Sin(angle);
                points.Add(new Vector3(x, 0f, z));
            }

            // Top straight: from (200, 0, 150) to (0, 0, 150)
            for (int i = 1; i <= segmentsPerStraight; i++)
            {
                float t = (float)i / segmentsPerStraight;
                points.Add(new Vector3(200f - t * straightLength, 0f, 150f));
            }

            // Left curve: semicircle from (0, 0, 150) to (0, 0, 0)
            // Center: (0, 0, 75), radius 75
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

        private static Mesh BuildTrackMesh(System.Collections.Generic.List<Vector3> points, float trackWidth)
        {
            var vertices = new System.Collections.Generic.List<Vector3>();
            var triangles = new System.Collections.Generic.List<int>();
            var uvs = new System.Collections.Generic.List<Vector2>();

            float segmentLength = 1f;

            for (int i = 0; i < points.Count - 1; i++)
            {
                Vector3 p0 = points[i];
                Vector3 p1 = points[i + 1];
                Vector3 dir = (p1 - p0).normalized;
                Vector3 right = new Vector3(-dir.z, 0f, dir.x);

                Vector3 v0 = p0 + right * trackWidth * 0.5f;
                Vector3 v1 = p0 - right * trackWidth * 0.5f;
                Vector3 v2 = p1 + right * trackWidth * 0.5f;
                Vector3 v3 = p1 - right * trackWidth * 0.5f;

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

                uvs.Add(new Vector2(0f, i * segmentLength));
                uvs.Add(new Vector2(1f, i * segmentLength));
                uvs.Add(new Vector2(0f, (i + 1) * segmentLength));
                uvs.Add(new Vector2(1f, (i + 1) * segmentLength));
            }

            var mesh = new Mesh
            {
                vertices = vertices.ToArray(),
                triangles = triangles.ToArray(),
                uv = uvs.ToArray()
            };
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();

            return mesh;
        }

        private static void CreateTrackBorders(System.Collections.Generic.List<Vector3> points, float trackWidth)
        {
            var borderMaterial = new Material(Shader.Find("Universal Render Pipeline/Lit"));
            if (borderMaterial.shader == null)
            {
                borderMaterial = new Material(Shader.Find("Standard"));
            }
            borderMaterial.color = new Color(0.8f, 0.2f, 0f);

            var leftVertices = new System.Collections.Generic.List<Vector3>();
            var rightVertices = new System.Collections.Generic.List<Vector3>();
            var leftTriangles = new System.Collections.Generic.List<int>();
            var rightTriangles = new System.Collections.Generic.List<int>();

            float borderWidth = 0.5f;
            float borderHeight = 0.5f;

            for (int i = 0; i < points.Count - 1; i++)
            {
                Vector3 p0 = points[i];
                Vector3 p1 = points[i + 1];
                Vector3 dir = (p1 - p0).normalized;
                Vector3 right = new Vector3(-dir.z, 0f, dir.x);

                Vector3 leftOuter = p0 + right * (trackWidth * 0.5f + borderWidth);
                Vector3 leftInner = p0 + right * (trackWidth * 0.5f);
                Vector3 rightOuter = p0 - right * (trackWidth * 0.5f + borderWidth);
                Vector3 rightInner = p0 - right * (trackWidth * 0.5f);

                int leftBase = leftVertices.Count;
                leftVertices.Add(leftOuter);
                leftVertices.Add(leftInner);
                leftVertices.Add(leftOuter + Vector3.up * borderHeight);
                leftVertices.Add(leftInner + Vector3.up * borderHeight);

                leftTriangles.Add(leftBase);
                leftTriangles.Add(leftBase + 1);
                leftTriangles.Add(leftBase + 2);

                leftTriangles.Add(leftBase + 1);
                leftTriangles.Add(leftBase + 3);
                leftTriangles.Add(leftBase + 2);

                int rightBase = rightVertices.Count;
                rightVertices.Add(rightOuter);
                rightVertices.Add(rightInner);
                rightVertices.Add(rightOuter + Vector3.up * borderHeight);
                rightVertices.Add(rightInner + Vector3.up * borderHeight);

                rightTriangles.Add(rightBase);
                rightTriangles.Add(rightBase + 2);
                rightTriangles.Add(rightBase + 1);

                rightTriangles.Add(rightBase + 1);
                rightTriangles.Add(rightBase + 2);
                rightTriangles.Add(rightBase + 3);
            }

            CreateBorderMesh("TrackBorderLeft", leftVertices, leftTriangles, borderMaterial);
            CreateBorderMesh("TrackBorderRight", rightVertices, rightTriangles, borderMaterial);
        }

        private static void CreateBorderMesh(string name, System.Collections.Generic.List<Vector3> vertices, System.Collections.Generic.List<int> triangles, Material material)
        {
            var borderObj = new GameObject(name);
            var meshFilter = borderObj.AddComponent<MeshFilter>();
            var meshRenderer = borderObj.AddComponent<MeshRenderer>();

            var mesh = new Mesh
            {
                vertices = vertices.ToArray(),
                triangles = triangles.ToArray()
            };
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();

            meshFilter.mesh = mesh;
            meshRenderer.material = material;
        }
    }
}
