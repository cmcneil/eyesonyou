function loadEyeModelFbx() {
  var promise = new Promise((resolve, reject) => {
    var loader = new THREE.FBXLoader();
    console.log(loader);
    loader.load( '/Eyeball/eyeball.fbx', function ( object ) {

      // mixer = new THREE.AnimationMixer( object );
      //
      // var action = mixer.clipAction( object.animations[ 0 ] );
      // action.play();

      object.traverse( function ( child ) {

        if ( child.isMesh ) {

          child.castShadow = true;
          child.receiveShadow = true;

        }
      });
      resolve(object);
    });
  });
  return promise;
}
