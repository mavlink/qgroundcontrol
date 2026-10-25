.pragma library

// The pose of the vehicle on each side, for the 3D picture. Quaternions are [w, x, y, z] arrays. The world has y up, and
// the vehicle's nose points along -z with its right wing along +x. A pose is only known up to the heading, so a tip
// picks the heading which turns the vehicle the least.

var identity = [1, 0, 0, 0]

function axisAngle(x, y, z, degrees) {
    var half = degrees * Math.PI / 360
    var s = Math.sin(half)
    return [Math.cos(half), x * s, y * s, z * s]
}

function multiply(a, b) {
    return [
        a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
        a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
        a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
        a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0]
    ]
}

function conjugate(q) {
    return [q[0], -q[1], -q[2], -q[3]]
}

function rotate(q, v) {
    var p = multiply(multiply(q, [0, v[0], v[1], v[2]]), conjugate(q))
    return [p[1], p[2], p[3]]
}

function _dot(a, b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]
}

/// The angle in degrees of the shortest turn from a to b
function angleBetween(a, b) {
    return 2 * Math.acos(Math.min(1, Math.abs(_dot(a, b)))) * 180 / Math.PI
}

/// The pose on the way from a to b along the shortest turn
function slerp(a, b, t) {
    var dot = _dot(a, b)
    var sign = dot < 0 ? -1 : 1
    dot = Math.min(1, Math.abs(dot))
    var theta = Math.acos(dot)
    var s = Math.sin(theta)
    var ka = 1 - t
    var kb = t
    if (s > 1e-6) {
        ka = Math.sin((1 - t) * theta) / s
        kb = Math.sin(t * theta) / s
    }
    kb *= sign
    var r = [ka * a[0] + kb * b[0], ka * a[1] + kb * b[1], ka * a[2] + kb * b[2], ka * a[3] + kb * b[3]]
    var length = Math.sqrt(_dot(r, r))
    return [r[0] / length, r[1] / length, r[2] / length, r[3] / length]
}

var _sidePose = {
    level:   identity,
    nose:    axisAngle(1, 0, 0, -90),
    tail:    axisAngle(1, 0, 0, 90),
    left:    axisAngle(0, 0, 1, 90),
    right:   axisAngle(0, 0, 1, -90),
    flipped: axisAngle(0, 0, 1, 180)
}

/// The direction in the vehicle's own frame which points at the ground on the side
var _bodyDown = {
    level:   [0, -1, 0],
    nose:    [0, 0, -1],
    tail:    [0, 0, 1],
    left:    [-1, 0, 0],
    right:   [1, 0, 0],
    flipped: [0, 1, 0]
}

function bodyDown(side) {
    return _bodyDown[side]
}

/// The pose on the side with the vehicle's nose pointing the default way
function sidePose(side) {
    return _sidePose[side]
}

/// The pose for the vehicle's attitude in degrees: roll right wing down, pitch nose up and heading clockwise from above.
/// Values which are not known yet count as zero.
function attitudePose(roll, pitch, heading) {
    function known(degrees) {
        return isFinite(degrees) ? degrees : 0
    }
    return multiply(multiply(axisAngle(0, 1, 0, -known(heading)), axisAngle(1, 0, 0, known(pitch))),
                    axisAngle(0, 0, 1, -known(roll)))
}

var _headings = [0, 90, -90, 180]

/// The pose to end a tip from the current pose onto the side. It turns the vehicle the least, so a tip from one side
/// onto the next is a single quarter turn.
function tipTarget(current, side) {
    var best = null
    var bestAngle = Infinity
    for (var i = 0; i < _headings.length; i++) {
        var candidate = multiply(axisAngle(0, 1, 0, _headings[i]), _sidePose[side])
        var angle = angleBetween(current, candidate)
        if (angle < bestAngle - 1e-6) {
            best = candidate
            bestAngle = angle
        }
    }
    return best
}

/// How far the vehicle's centre is above the ground it rests on in the pose. half is the half size of the box which
/// bounds the vehicle along its x, y and z axes.
function restHeight(q, half) {
    var w = q[0]
    var x = q[1]
    var y = q[2]
    var z = q[3]
    return Math.abs(2 * (x * y + w * z)) * half[0] + Math.abs(1 - 2 * (x * x + z * z)) * half[1] + Math.abs(2 * (y * z - w * x)) * half[2]
}
