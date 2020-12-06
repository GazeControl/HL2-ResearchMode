using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Microsoft.MixedReality.Toolkit.SpatialAwareness;
using TMPro;

namespace Microsoft.MixedReality.Toolkit
{
    public class SpatialPointCloud : MonoBehaviour
    {
        //IMixedRealitySpatialAwarenessMeshObserver meshObserver = CoreServices.GetSpatialAwarenessSystemDataProvider<IMixedRealitySpatialAwarenessMeshObserver>();
        public MeshFilter PointCloudRender;
        public Transform World;
        public TextMeshPro Text;
        private bool Status = false;
        // Start is called before the first frame update
        void Start()
        {

            var spatialAwarenessService = CoreServices.SpatialAwarenessSystem;

            // Cast to the IMixedRealityDataProviderAccess to get access to the data providers
            var dataProviderAccess = spatialAwarenessService as IMixedRealityDataProviderAccess;

            var meshObserver = dataProviderAccess.GetDataProvider<IMixedRealitySpatialAwarenessMeshObserver>();
        }

        // Update is called once per frame
        void Update()
        {
            //var observer = CoreServices.GetSpatialAwarenessSystemDataProvider<IMixedRealitySpatialAwarenessMeshObserver>();

            // Loop through all known Meshes

            var observer = CoreServices.GetSpatialAwarenessSystemDataProvider<IMixedRealitySpatialAwarenessMeshObserver>();


            CombineInstance[] combine = new CombineInstance[observer.Meshes.Count];
            Text.text = "mesh nums:"+observer.Meshes.Count;
            int i = 0;
            foreach (SpatialAwarenessMeshObject meshObject in observer.Meshes.Values)
            {
                Mesh mesh = meshObject.Filter.mesh;
                GameObject Mesh = meshObject.GameObject;
                combine[i].mesh = meshObject.Filter.mesh;
                combine[i].transform = meshObject.GameObject.transform.localToWorldMatrix;
                i++;
            }
            if(!PointCloudRender.mesh)
            { PointCloudRender.mesh = new Mesh(); }
            
            
            PointCloudRender.mesh.CombineMeshes(combine);
        }

        public void toggleSpatial()
        {
            if(!Status)
            { CoreServices.SpatialAwarenessSystem.ResumeObservers(); }
            else
            { CoreServices.SpatialAwarenessSystem.SuspendObservers(); }
            //CoreServices.SpatialAwarenessSystem.ResumeObservers();

            //// Suspend Mesh Observation from all Observers
            //CoreServices.SpatialAwarenessSystem.SuspendObservers();
            Status = !Status;
        }
    }
}

