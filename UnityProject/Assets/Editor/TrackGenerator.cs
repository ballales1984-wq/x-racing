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
                meshRenderer.sharedMaterial = new Material(Shader.Find("Standard"));
                meshRenderer.sharedMaterial.SetFloat("_Glossiness", 0f);
            }
            meshRenderer.sharedMaterial.color = new Color(0.25f, 0.25f, 0.25f);

            var trackMesh = BuildTrackMesh();
            meshFilter.mesh = trackMesh;

            var collider = trackObj.GetComponent<MeshCollider>();
            if (collider == null)
            {
                collider = trackObj.AddComponent<MeshCollider>();
            }
            collider.sharedMesh = trackMesh;

            Debug.Log("Track generated successfully!");
        }

        private static Mesh BuildTrackMesh()
        {
            var points = GenerateTrackPoints();
            var vertices = new System.Collections.Generic.List<Vector3>();
            var triangles = new System.Collections.Generic.List<int>();
            var uvs = new System.Collections.Generic.List<Vector2>();

            float trackWidth = 6f;
            float segmentLength = 2f;

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

        private static System.Collections.Generic.List<Vector3> GenerateTrackPoints()
        {
            var points = new System.Collections.Generic.List<Vector3>();
            int numPoints = 400;

            for (int i = 0; i <= numPoints; i++)
            {
                float t = (float)i / numPoints;
                Vector3 pos;

                if (t < 0.25f)
                {
                    float lt = t / 0.25f;
                    pos = new Vector3(lt * 200f, 0f, 0f);
                }
                else if (t < 0.5f)
                {
                    float lt = (t - 0.25f) / 0.25f;
                    float cx = 200f;
                    float cz = -150f;
                    float r = 150f;
                    float angle = -90f + lt * 180f;
                    float rad = angle * Mathf.Deg2Rad;
                    pos = new Vector3(cx + r * Mathf.Cos(rad), 0f, cz + r * Mathf.Sin(rad));
                }
                else if (t < 0.75f)
                {
                    float lt = (t - 0.5f) / 0.25f;
                    pos = new Vector3(200f - lt * 400f, 0f, -300f);
                }
                else
                {
                    float lt = (t - 0.75f) / 0.25f;
                    float cx = -200f;
                    float cz = -150f;
                    float r = 150f;
                    float angle = 180f + lt * 180f;
                    float rad = angle * Mathf.Deg2Rad;
                    pos = new Vector3(cx + r * Mathf.Cos(rad), 0f, cz + r * Mathf.Sin(rad));
                }

                points.Add(pos);
            }

            return points;
        }
    }
}
