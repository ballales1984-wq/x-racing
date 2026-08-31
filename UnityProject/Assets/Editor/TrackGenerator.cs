using UnityEditor;
using UnityEngine;
using UnityEditor.SceneManagement;

namespace Project0.Unity.Setup
{
    public static class TrackGenerator
    {
        [MenuItem("X-Racing/Generate Track")]
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
                meshRenderer.sharedMaterial = CreateDefaultMaterial(new Color(0.35f, 0.35f, 0.35f));
            }

            float normalWidth = 12f;
            float mainStraightWidth = 16f;
            var points = GenerateTrackPoints(out var widths);

            var oldBorders = Object.FindObjectsByType<GameObject>();
            foreach (var border in oldBorders)
            {
                if (border.name.Contains("TrackBorder"))
                {
                    UnityEngine.Object.DestroyImmediate(border);
                }
            }

            var oldGrass = Object.FindObjectsByType<GameObject>();
            foreach (var g in oldGrass)
            {
                if (g.name == "Grass")
                {
                    UnityEngine.Object.DestroyImmediate(g);
                }
            }

            CreateTrackBorders(points, widths, normalWidth, mainStraightWidth);
            CreateGrassArea();
            CreateStartFinishLine();
            CreateBoxLane(points, widths);
            CreatePitLane(points, widths);
            CreateCheckpoints(points, widths);

            var lightObj = GameObject.Find("Directional Light");
            if (lightObj == null)
            {
                lightObj = new GameObject("Directional Light");
                var light = lightObj.AddComponent<Light>();
                light.type = LightType.Directional;
                light.intensity = 1f;
                lightObj.transform.rotation = Quaternion.Euler(50f, -30f, 0f);
            }

            var trackMesh = BuildTrackMesh(points, widths);
            meshFilter.mesh = trackMesh;

            var collider = trackObj.GetComponent<MeshCollider>();
            if (collider == null)
            {
                collider = trackObj.AddComponent<MeshCollider>();
            }
            collider.sharedMesh = trackMesh;

            Debug.Log($"Track generated: {points.Count} points, length ~{CalculateTrackLength(points):F0}m");
        }

        private static Material CreateDefaultMaterial(Color color)
        {
            Shader shader = Shader.Find("Standard");
            if (shader == null)
            {
                shader = Shader.Find("Hidden/InternalErrorShader");
            }

            Material mat = new Material(shader);
            mat.color = color;
            return mat;
        }

         private static void CreateGrassArea()
         {
             var grassObj = new GameObject("Grass");
             var meshFilter = grassObj.AddComponent<MeshFilter>();
             var meshRenderer = grassObj.AddComponent<MeshRenderer>();
             
             var mesh = new Mesh();
             var vertices = new Vector3[]
             {
                 new Vector3(-1000, -0.1f, -1000),
                 new Vector3(1000, -0.1f, -1000),
                 new Vector3(-1000, -0.1f, 1000),
                 new Vector3(1000, -0.1f, 1000)
             };
             var triangles = new int[] { 0, 2, 1, 1, 2, 3 };
             
             mesh.vertices = vertices;
             mesh.triangles = triangles;
             mesh.RecalculateNormals();
             mesh.RecalculateBounds();
             
             meshFilter.mesh = mesh;
             
             var mat = CreateDefaultMaterial(new Color(0.2f, 0.6f, 0.2f));
             if (mat != null)
             {
                 meshRenderer.sharedMaterial = mat;
             }
         }

private static void CreateStartFinishLine()
        {
            var sfObj = GameObject.Find("StartFinishLine");
            if (sfObj == null)
            {
                sfObj = new GameObject("StartFinishLine");
            }
            sfObj.transform.SetParent(GameObject.Find("Track")?.transform);
            sfObj.transform.localPosition = new Vector3(0f, 0.01f, 0f);
            sfObj.transform.localRotation = Quaternion.identity;

            var mf = sfObj.GetComponent<MeshFilter>();
            if (mf == null) mf = sfObj.AddComponent<MeshFilter>();

            var mr = sfObj.GetComponent<MeshRenderer>();
            if (mr == null) mr = sfObj.AddComponent<MeshRenderer>();

            float lineLength = 16f;
            float lineThickness = 0.2f;
            var mesh = new Mesh();
            var verts = new Vector3[]
            {
                new Vector3(-lineLength * 0.5f, 0f, -lineThickness * 0.5f),
                new Vector3(-lineLength * 0.5f, 0f,  lineThickness * 0.5f),
                new Vector3( lineLength * 0.5f, 0f, -lineThickness * 0.5f),
                new Vector3( lineLength * 0.5f, 0f,  lineThickness * 0.5f),
            };
            var tris = new int[] { 0, 2, 1, 1, 2, 3 };
            var uvs = new Vector2[] { new Vector2(0, 0), new Vector2(0, 1), new Vector2(1, 0), new Vector2(1, 1) };
            mesh.vertices = verts;
            mesh.triangles = tris;
            mesh.uv = uvs;
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            mf.mesh = mesh;

            var mat = CreateDefaultMaterial(Color.yellow);
            if (mat != null)
            {
                mr.sharedMaterial = mat;
            }
        }

        private static void CreateBoxLane(System.Collections.Generic.List<Vector3> points, System.Collections.Generic.List<float> widths)
        {
            // Remove existing box lane
            var oldBox = GameObject.Find("BoxLane");
            if (oldBox != null) UnityEngine.Object.DestroyImmediate(oldBox);

            var boxObj = new GameObject("BoxLane");
            boxObj.transform.SetParent(GameObject.Find("Track")?.transform);

            var mf = boxObj.AddComponent<MeshFilter>();
            var mr = boxObj.AddComponent<MeshRenderer>();

            var vertices = new System.Collections.Generic.List<Vector3>();
            var triangles = new System.Collections.Generic.List<int>();
            var uvs = new System.Collections.Generic.List<Vector2>();

            float boxLaneWidth = 3.5f;
            float boxLaneOffset = 1.5f;
            float segmentLength = 1f;

            // Only create box lane on the first straight section (where width = 16f)
            for (int i = 0; i < points.Count - 1; i++)
            {
                // Box lane only on main straight (width > 14f indicates main straight)
                if (widths[i] < 14f) continue;

                Vector3 p0 = points[i];
                Vector3 p1 = points[i + 1];
                Vector3 dir = (p1 - p0).normalized;
                Vector3 right = new Vector3(-dir.z, 0f, dir.x);

                float trackHalfWidth = widths[i] * 0.5f;
                float boxInner = trackHalfWidth + boxLaneOffset;
                float boxOuter = boxInner + boxLaneWidth;

                Vector3 v0 = p0 + right * boxInner;
                Vector3 v1 = p0 + right * boxOuter;
                Vector3 v2 = p1 + right * boxInner;
                Vector3 v3 = p1 + right * boxOuter;

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
            mf.mesh = mesh;

            var mat = CreateDefaultMaterial(new Color(0.3f, 0.3f, 0.4f));
            if (mat != null)
            {
                mr.sharedMaterial = mat;
            }
        }

        private static void CreatePitLane(System.Collections.Generic.List<Vector3> points, System.Collections.Generic.List<float> widths)
        {
            var oldPitLane = GameObject.Find("PitLane");
            if (oldPitLane != null) UnityEngine.Object.DestroyImmediate(oldPitLane);

            var pitLaneObj = new GameObject("PitLane");
            pitLaneObj.transform.SetParent(GameObject.Find("Track")?.transform);

            var mf = pitLaneObj.AddComponent<MeshFilter>();
            var mr = pitLaneObj.AddComponent<MeshRenderer>();

            var vertices = new System.Collections.Generic.List<Vector3>();
            var triangles = new System.Collections.Generic.List<int>();
            var uvs = new System.Collections.Generic.List<Vector2>();

            float pitLaneWidth = 4f;
            float pitLaneOffset = 4f;
            float pitLaneStart = 100f;
            float pitLaneEnd = 600f;

            // Find pit lane points on the main straight
            for (int i = 0; i < points.Count - 1; i++)
            {
                if (widths[i] < 14f) continue;

                float xPos = points[i].x;
                if (xPos < pitLaneStart || xPos > pitLaneEnd) continue;

                Vector3 p0 = points[i];
                Vector3 p1 = points[i + 1];
                Vector3 dir = (p1 - p0).normalized;
                Vector3 right = new Vector3(-dir.z, 0f, dir.x);

                float trackHalfWidth = widths[i] * 0.5f;
                float pitInner = trackHalfWidth + pitLaneOffset;
                float pitOuter = pitInner + pitLaneWidth;

                Vector3 v0 = p0 + right * pitInner;
                Vector3 v1 = p0 + right * pitOuter;
                Vector3 v2 = p1 + right * pitInner;
                Vector3 v3 = p1 + right * pitOuter;

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

                uvs.Add(new Vector2(0f, xPos));
                uvs.Add(new Vector2(1f, xPos));
                uvs.Add(new Vector2(0f, xPos + (p1 - p0).magnitude));
                uvs.Add(new Vector2(1f, xPos + (p1 - p0).magnitude));
            }

            var mesh = new Mesh
            {
                vertices = vertices.ToArray(),
                triangles = triangles.ToArray(),
                uv = uvs.ToArray()
            };
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            mf.mesh = mesh;

            var mat = CreateDefaultMaterial(new Color(0.4f, 0.4f, 0.45f));
            if (mat != null)
            {
                mr.sharedMaterial = mat;
            }

            // Create pit boxes
            CreatePitBoxes(pitLaneStart, pitLaneEnd, pitLaneOffset, pitLaneWidth, widths);

            // Create entry/exit lanes connecting pit to track
            CreatePitEntryExit(pitLaneStart, pitLaneEnd, pitLaneOffset, pitLaneWidth, widths);
        }

        private static void CreatePitEntryExit(float startX, float endX, float offset, float laneWidth, System.Collections.Generic.List<float> widths)
        {
            var oldEntry = GameObject.Find("PitEntryExit");
            if (oldEntry != null) UnityEngine.Object.DestroyImmediate(oldEntry);

            var entryObj = new GameObject("PitEntryExit");
            entryObj.transform.SetParent(GameObject.Find("Track")?.transform);

            var mf = entryObj.AddComponent<MeshFilter>();
            var mr = entryObj.AddComponent<MeshRenderer>();

            var vertices = new System.Collections.Generic.List<Vector3>();
            var triangles = new System.Collections.Generic.List<int>();

            float trackHalfWidth = 8f; // Main straight half width
            float pitInner = trackHalfWidth + offset;
            float pitOuter = pitInner + laneWidth;

            // Entry lane at start of pit
            float entryStartX = startX - 20f;
            float entryEndX = startX;
            float zTrack = trackHalfWidth;
            float zPit = pitInner;

            // Create entry ramp
            for (int i = 0; i < 4; i++)
            {
                float t = i / 4f;
                float x = entryStartX + (entryEndX - entryStartX) * t;
                float z = zTrack + (zPit - zTrack) * t;

                Vector3 p0 = new Vector3(x, 0f, z);
                Vector3 p1 = new Vector3(x + 5f, 0f, z + (zPit - zTrack) / 4f);

                int baseIndex = vertices.Count;
                vertices.Add(p0);
                vertices.Add(new Vector3(p0.x, 0f, z + laneWidth));
                vertices.Add(p1);
                vertices.Add(new Vector3(p1.x, 0f, z + (zPit - zTrack) / 4f + laneWidth));

                triangles.Add(baseIndex);
                triangles.Add(baseIndex + 2);
                triangles.Add(baseIndex + 1);
                triangles.Add(baseIndex + 1);
                triangles.Add(baseIndex + 2);
                triangles.Add(baseIndex + 3);
            }

            // Exit lane at end of pit
            float exitStartX = endX;
            float exitEndX = endX + 20f;
            zTrack = trackHalfWidth;
            zPit = pitInner;

            for (int i = 0; i < 4; i++)
            {
                float t = i / 4f;
                float x = exitStartX + (exitEndX - exitStartX) * t;
                float z = zPit + (zTrack - zPit) * t;

                Vector3 p0 = new Vector3(x, 0f, z);
                Vector3 p1 = new Vector3(x + 5f, 0f, z + (zTrack - zPit) / 4f);

                int baseIndex = vertices.Count;
                vertices.Add(p0);
                vertices.Add(new Vector3(p0.x, 0f, z + laneWidth));
                vertices.Add(p1);
                vertices.Add(new Vector3(p1.x, 0f, z + (zTrack - zPit) / 4f + laneWidth));

                triangles.Add(baseIndex);
                triangles.Add(baseIndex + 2);
                triangles.Add(baseIndex + 1);
                triangles.Add(baseIndex + 1);
                triangles.Add(baseIndex + 2);
                triangles.Add(baseIndex + 3);
            }

            var mesh = new Mesh
            {
                vertices = vertices.ToArray(),
                triangles = triangles.ToArray()
            };
            mesh.RecalculateNormals();
            mesh.RecalculateBounds();
            mf.mesh = mesh;

            var mat = CreateDefaultMaterial(new Color(0.5f, 0.5f, 0.55f));
            if (mat != null)
            {
                mr.sharedMaterial = mat;
            }
        }

        private static void CreatePitBoxes(float startX, float endX, float offset, float laneWidth, System.Collections.Generic.List<float> widths)
        {
            var oldBoxes = GameObject.Find("PitBoxes");
            if (oldBoxes != null) UnityEngine.Object.DestroyImmediate(oldBoxes);

            var pitBoxesObj = new GameObject("PitBoxes");
            pitBoxesObj.transform.SetParent(GameObject.Find("Track")?.transform);

            float boxLength = 6f;
            float boxWidth = 3f;
            float boxHeight = 0.5f;
            float boxSpacing = 10f;

            int boxIndex = 0;
            for (float x = startX + 10f; x < endX - 10f; x += boxSpacing)
            {
                var boxObj = GameObject.CreatePrimitive(PrimitiveType.Cube);
                boxObj.name = $"PitBox_{boxIndex}";
                boxObj.transform.SetParent(pitBoxesObj.transform);

                float zPos = 12f + offset + laneWidth * 0.5f;
                boxObj.transform.position = new Vector3(x + boxLength * 0.5f, boxHeight * 0.5f, zPos);
                boxObj.transform.localScale = new Vector3(boxLength, boxHeight, boxWidth);

                var renderer = boxObj.GetComponent<Renderer>();
                if (renderer != null)
                {
                    var mat = CreateDefaultMaterial(boxIndex % 2 == 0 ? new Color(0.9f, 0.9f, 0.9f) : new Color(0.2f, 0.2f, 0.2f));
                    if (mat != null)
                    {
                        renderer.sharedMaterial = mat;
                    }
                }

                boxIndex++;
            }

            Debug.Log($"Created {boxIndex} pit boxes");
        }

        private static void CreateCheckpoints(System.Collections.Generic.List<Vector3> points, System.Collections.Generic.List<float> widths)
        {
            var oldCheckpoints = GameObject.Find("Checkpoints");
            if (oldCheckpoints != null) UnityEngine.Object.DestroyImmediate(oldCheckpoints);

            var checkpointsObj = new GameObject("Checkpoints");
            checkpointsObj.transform.SetParent(GameObject.Find("Track")?.transform);

            int numCheckpoints = 8;
            var cumulative = new System.Collections.Generic.List<float>(points.Count);
            cumulative.Add(0f);
            for (int i = 1; i < points.Count; i++)
            {
                cumulative.Add(cumulative[i - 1] + Vector3.Distance(points[i], points[i - 1]));
            }
            float totalLength = cumulative[cumulative.Count - 1];

            for (int i = 0; i < numCheckpoints; i++)
            {
                float targetDist = totalLength * i / numCheckpoints;
                int index = 0;
                for (int j = 1; j < cumulative.Count; j++)
                {
                    if (cumulative[j] > targetDist)
                    {
                        index = j - 1;
                        break;
                    }
                }
                if (index >= points.Count) index = points.Count - 1;

                var cpObj = new GameObject($"Checkpoint_{i}");
                cpObj.transform.SetParent(checkpointsObj.transform);
                cpObj.transform.position = points[index];

                Vector3 dir = (index < points.Count - 1 ? points[index + 1] - points[index] : points[index] - points[index - 1]).normalized;
                cpObj.transform.rotation = Quaternion.LookRotation(dir, Vector3.up);

                var trigger = cpObj.AddComponent<CheckpointTrigger>();
                trigger.checkpointIndex = i;
                trigger.totalCheckpoints = numCheckpoints;
            }

            Debug.Log($"Created {numCheckpoints} checkpoints");
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

        private static System.Collections.Generic.List<Vector3> GenerateTrackPoints(out System.Collections.Generic.List<float> widths)
        {
            var points = new System.Collections.Generic.List<Vector3>();
            widths = new System.Collections.Generic.List<float>();
            float straightLength = 765f;
            float curveRadius = 75f;
            int segmentsPerStraight = 150;
            int segmentsPerCurve = 75;

            // Main straight: from (0, 0, 0) to (765, 0, 0) - width 16m
            for (int i = 0; i <= segmentsPerStraight; i++)
            {
                float t = (float)i / segmentsPerStraight;
                points.Add(new Vector3(t * straightLength, 0f, 0f));
                widths.Add(16f);
            }

            // Right curve: semicircle from (765, 0, 0) to (765, 0, 150) - width 12m
            // Center: (765, 0, 75), radius 75
            for (int i = 1; i <= segmentsPerCurve; i++)
            {
                float t = (float)i / segmentsPerCurve;
                float angle = -Mathf.PI / 2 + t * Mathf.PI;
                float x = 765f + curveRadius * Mathf.Cos(angle);
                float z = 75f + curveRadius * Mathf.Sin(angle);
                points.Add(new Vector3(x, 0f, z));
                widths.Add(12f);
            }

            // Back straight: from (765, 0, 150) to (0, 0, 150) - width 12m
            for (int i = 1; i <= segmentsPerStraight; i++)
            {
                float t = (float)i / segmentsPerStraight;
                points.Add(new Vector3(765f - t * straightLength, 0f, 150f));
                widths.Add(12f);
            }

            // Left curve: semicircle from (0, 0, 150) to (0, 0, 0) - width 12m
            // Center: (0, 0, 75), radius 75
            for (int i = 1; i <= segmentsPerCurve; i++)
            {
                float t = (float)i / segmentsPerCurve;
                float angle = Mathf.PI / 2 + t * Mathf.PI;
                float x = curveRadius * Mathf.Cos(angle);
                float z = 75f + curveRadius * Mathf.Sin(angle);
                points.Add(new Vector3(x, 0f, z));
                widths.Add(12f);
            }

            return points;
        }

        private static Mesh BuildTrackMesh(System.Collections.Generic.List<Vector3> points, System.Collections.Generic.List<float> widths)
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

        private static void CreateTrackBorders(System.Collections.Generic.List<Vector3> points, System.Collections.Generic.List<float> widths, float normalWidth, float mainStraightWidth)
        {
            Material borderMat = CreateDefaultMaterial(new Color(0.8f, 0.2f, 0f));

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
                float width = widths[i];

                float outerWidth = width * 0.5f + borderWidth;

                Vector3 leftOuter = p0 + right * outerWidth;
                Vector3 leftInner = p0 + right * width * 0.5f;
                Vector3 rightOuter = p0 - right * outerWidth;
                Vector3 rightInner = p0 - right * width * 0.5f;

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

            CreateBorderMesh("TrackBorderLeft", leftVertices, leftTriangles, borderMat);
            CreateBorderMesh("TrackBorderRight", rightVertices, rightTriangles, borderMat);
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
