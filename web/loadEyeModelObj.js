
function loadEyeModelObj() {
  var manager = new THREE.LoadingManager();

  var promise = new Promise((resolve, reject) => {
    new THREE.MTLLoader( manager )
    .setPath( '/obj/' )
    .load( 'eyeball.mtl', function ( materials ) {
      materials.preload();

      new THREE.OBJLoader( manager )
        .setMaterials( materials )
        .setPath( '/obj/' )
        .load( 'eyeball.obj', function ( object ) {
          object.geometry.computeFaceNormals();
          resolve(object);
      }, null, (err) => reject(err) );
    });
  });
  return promise;
}
