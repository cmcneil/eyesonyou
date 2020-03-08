// const angular_acceleration = ; // 
// const math = window.math;

function updatePosition(eye_model, obj_position) {
    // We know the eye is at (0, 0, 0)
    // with pointing vector (0, 0, 1)
    var norm = Math.sqrt(Math.pow(obj_position[0], 2) +
                         Math.pow(obj_position[1], 2) +
                         Math.pow(obj_position[2], 2));
    var x_ang = Math.asin(obj_position[0] / norm);
    var y_ang = Math.asin(obj_position[1] / norm);
    eye_model.rotation.x = x_ang;
    eye_model.rotation.y = y_ang;
}