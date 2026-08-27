using UnityEngine;
using UnityEditor;
using System.IO;
using System.Text;

public class FBXToOBJ : EditorWindow
{
    [MenuItem("Tools/FBX to OBJ")]
    public static void Convert()
    {
        string fbxPath = @"D:\x-racing\UnityProject\Assets\Models\car.fbx";
        string objPath = @"D:\x-racing\assets\models\car.obj";
        
        // Load FBX from Assets/Models folder
        string assetPath = "Assets/Models/car.fbx";
        GameObject obj = AssetDatabase.LoadAssetAtPath<GameObject>(assetPath);
        
        if (obj == null)
        {
            Debug.LogError("Could not load FBX at " + assetPath);
            return;
        }
        
        StringBuilder sb = new StringBuilder();
        sb.AppendLine("# Converted from FBX using Unity");
        
        MeshFilter[] meshFilters = obj.GetComponentsInChildren<MeshFilter>();
        int vertexOffset = 0;
        
        foreach (MeshFilter mf in meshFilters)
        {
            Mesh mesh = mf.sharedMesh;
            if (mesh == null) continue;
            
            // Write vertices
            foreach (Vector3 v in mesh.vertices)
            {
                sb.AppendLine($"v {v.x:F6} {v.y:F6} {v.z:F6}");
            }
            
            // Write normals
            foreach (Vector3 n in mesh.normals)
            {
                sb.AppendLine($"vn {n.x:F6} {n.y:F6} {n.z:F6}");
            }
            
            // Write UVs
            foreach (Vector2 uv in mesh.uv)
            {
                sb.AppendLine($"vt {uv.x:F6} {uv.y:F6}");
            }
            
            // Write faces
            for (int i = 0; i < mesh.triangles.Length; i += 3)
            {
                int i0 = mesh.triangles[i] + 1 + vertexOffset;
                int i1 = mesh.triangles[i + 1] + 1 + vertexOffset;
                int i2 = mesh.triangles[i + 2] + 1 + vertexOffset;
                sb.AppendLine($"f {i0}/{i0}/{i0} {i1}/{i1}/{i1} {i2}/{i2}/{i2}");
            }
            
            vertexOffset += mesh.vertices.Length;
        }
        
        File.WriteAllText(objPath, sb.ToString());
        Debug.Log($"Exported {meshFilters.Length} meshes to {objPath}");
    }
}
