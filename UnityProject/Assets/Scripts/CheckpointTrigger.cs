using UnityEngine;

namespace Project0.Unity
{
    [RequireComponent(typeof(BoxCollider))]
    public class CheckpointTrigger : MonoBehaviour
    {
        public int checkpointIndex = 0;
        public int totalCheckpoints = 8;

        private BoxCollider boxCol;
        private bool wasPassed = false;

        void Awake()
        {
            boxCol = GetComponent<BoxCollider>();
            boxCol.isTrigger = true;

            Transform trackTransform = GameObject.Find("Track")?.transform;
            if (trackTransform != null)
            {
                Transform parent = trackTransform.parent;
                if (parent != null)
                    transform.SetParent(parent);
            }
        }

        void OnTriggerEnter(Collider other)
        {
            if (other.name != "Car") return;

            if (!wasPassed)
            {
                wasPassed = true;
                var carController = other.GetComponent<CarController>();
                if (carController != null)
                {
                    carController.OnCheckpointPassed(checkpointIndex, totalCheckpoints);
                }
            }
        }

        void OnDisable()
        {
            wasPassed = false;
        }

        void OnEnable()
        {
            wasPassed = false;
        }

        public void ResetCheckpoint()
        {
            wasPassed = false;
        }
    }
}
