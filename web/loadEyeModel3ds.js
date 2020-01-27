function loadEyeModel3ds() {
  var promise = new Promise((resolve, reject) => {
    var loader = new THREE.TextureLoader();
var normal = loader.load( '/Eyeball/textures/Eye_N.jpg' );

var loader = new THREE.TDSLoader( );
loader.setResourcePath( '/Eyeball/textures/' );
loader.load( '/Eyeball/eyeball.3ds', function ( object ) {

  object.traverse( function ( child ) {

    if ( child.isMesh ) {

      child.material.normalMap = normal;

    }

  } );
  resolve(object);

} );

});
return promise;
}
