
function loadEyeModelGltf() {
  var promise = new Promise((resolve, reject) => {
    var loader = new THREE.GLTFLoader().setPath( '/gltfEyeball/' );
    loader.load( 'scene.gltf', function ( gltf ) {

    // gltf.scene.traverse( function ( child ) {
    //   if ( child.isMesh ) {
    //     roughnessMipmapper.generateMipmaps( child.material );
    //   }
    // } );

      resolve(gltf.scene);
    // scene.add( gltf.scene );

    // roughnessMipmapper.dispose();
    });
  });
  return promise;
}
