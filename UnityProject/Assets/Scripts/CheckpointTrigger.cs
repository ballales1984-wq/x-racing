using UnityEngine;
using System.Collections.Generic;

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
                boxCol.size = new Vector3(15f, 4f, 5f);
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
