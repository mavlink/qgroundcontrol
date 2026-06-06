import QtQuick

/*
 * ROVArmView — MIST MAVIROV robotic arm 3-D visualiser.
 *
 * Faithful JavaScript port of the painter's-algorithm software renderer in
 * ROV GCS v3.4 (Python/Tkinter). Same scene geometry, camera, lights and
 * shading. The original renders at 900x620 (FOV 540) and crops
 * (120,90,860,540) before stretching onto the widget; that transform is
 * folded into the projection constants below so any canvas size shows the
 * exact same framing.
 *
 *   rollDeg   — arm roll angle in degrees (servo 2, 1525 us == 0 deg)
 *   clawRatio — claw opening 0..1 (servo 1, 0 == closed, 1 == fully open)
 */
Item {
    id: _root

    property real rollDeg:   0
    property real clawRatio: 0

    implicitWidth:  300
    implicitHeight: Math.round(width * 450 / 740)

    onRollDegChanged:   canvas.requestPaint()
    onClawRatioChanged: canvas.requestPaint()

    Canvas {
        id:              canvas
        anchors.fill:    parent
        renderStrategy:  Canvas.Cooperative
        onWidthChanged:  requestPaint()
        onHeightChanged: requestPaint()

        // ── Vector / rotation helpers ─────────────────────────────────────
        property var _js: ({})

        function vsub(a,b){ return [a[0]-b[0],a[1]-b[1],a[2]-b[2]]; }
        function vadd(a,b){ return [a[0]+b[0],a[1]+b[1],a[2]+b[2]]; }
        function vdot(a,b){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
        function vcross(a,b){ return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]; }
        function vnorm(v){ var L=Math.sqrt(vdot(v,v)); return L>1e-9 ? [v[0]/L,v[1]/L,v[2]/L] : v; }
        function rotateY(p,ang){ var c=Math.cos(ang),s=Math.sin(ang); return [p[0]*c+p[2]*s, p[1], -p[0]*s+p[2]*c]; }
        function rotatePivotY(p,pivot,ang){ return vadd(rotateY(vsub(p,pivot),ang),pivot); }
        function rotateAxisX(p,y0,z0,ang){
            var y=p[1]-y0, z=p[2]-z0, c=Math.cos(ang), s=Math.sin(ang);
            return [p[0], y*c-z*s+y0, y*s+z*c+z0];
        }

        // ── Geometry builders ─────────────────────────────────────────────
        function buildBox(center,size){
            var cx=center[0],cy=center[1],cz=center[2];
            var hx=size[0]/2,hy=size[1]/2,hz=size[2]/2;
            var p=[[cx-hx,cy-hy,cz-hz],[cx+hx,cy-hy,cz-hz],[cx+hx,cy+hy,cz-hz],[cx-hx,cy+hy,cz-hz],
                   [cx-hx,cy-hy,cz+hz],[cx+hx,cy-hy,cz+hz],[cx+hx,cy+hy,cz+hz],[cx-hx,cy+hy,cz+hz]];
            return [[p[4],p[5],p[6],p[7]],[p[1],p[0],p[3],p[2]],[p[5],p[1],p[2],p[6]],
                    [p[0],p[4],p[7],p[3]],[p[7],p[6],p[2],p[3]],[p[0],p[1],p[5],p[4]]];
        }
        function buildPrismX(center,radius,length,sides){
            var cx=center[0],cy=center[1],cz=center[2],half=length/2,top=[],bot=[],i,j,a;
            for(i=0;i<sides;i++){
                a=2*Math.PI*i/sides;
                top.push([cx+half,cy+radius*Math.cos(a),cz+radius*Math.sin(a)]);
                bot.push([cx-half,cy+radius*Math.cos(a),cz+radius*Math.sin(a)]);
            }
            var faces=[];
            for(i=0;i<sides;i++){ j=(i+1)%sides; faces.push([bot[i],bot[j],top[j],top[i]]); }
            faces.push(top.slice());
            faces.push(bot.slice().reverse());
            return faces;
        }
        function buildPrismY(center,radius,height,sides){
            var cx=center[0],cy=center[1],cz=center[2],half=height/2,top=[],bot=[],i,j,a;
            for(i=0;i<sides;i++){
                a=2*Math.PI*i/sides;
                top.push([cx+radius*Math.cos(a),cy+half,cz+radius*Math.sin(a)]);
                bot.push([cx+radius*Math.cos(a),cy-half,cz+radius*Math.sin(a)]);
            }
            var faces=[];
            for(i=0;i<sides;i++){ j=(i+1)%sides; faces.push([bot[i],bot[j],top[j],top[i]]); }
            faces.push(top.slice());
            faces.push(bot.slice().reverse());
            return faces;
        }
        function buildBevelBox(center,size,bevel){
            var faces=buildBox(center,size);
            var topC=[center[0],center[1]+size[1]/2+bevel/2,center[2]];
            var topS=[size[0]-2*bevel,bevel,size[2]-2*bevel];
            return faces.concat(buildBox(topC,topS));
        }

        // ── Camera / projection (matches Python render+crop exactly) ──────
        readonly property var camPos:  [6.4, 4.2, 7.8]
        readonly property var camLook: [1.2, 1.05, 0.0]
        property var camF: vnorm(vsub(camLook,camPos))
        property var camR: vnorm(vcross(camF,[0,1,0]))
        property var camU: vcross(camR,camF)
        property var viewDir: vnorm(vsub(camPos,camLook))

        function worldToCam(p){
            var d=vsub(p,camPos);
            return [vdot(d,camR), vdot(d,camU), vdot(d,camF)];
        }
        // 900x620 @ FOV 540, cropped to (120,90,860,540) => 740x450 region:
        readonly property real prjCx: 330/740
        readonly property real prjCy: 220/450
        readonly property real prjFx: 540/740
        readonly property real prjFy: 540/450
        function project(pcam,w,h){
            var z=pcam[2]<0.05?0.05:pcam[2];
            return [prjCx*w + pcam[0]*prjFx*w/z,
                    prjCy*h - pcam[1]*prjFy*h/z];
        }

        // ── Materials, colors, lights ─────────────────────────────────────
        readonly property var matPaint:   [0.55,48]
        readonly property var matPlastic: [0.35,22]
        readonly property var matMetal:   [0.75,90]
        readonly property var matBrushed: [0.40,18]
        readonly property var matRubber:  [0.08,6]
        readonly property var matMatte:   [0.15,10]
        readonly property var matEmissive: null

        readonly property var colArm:     [255,140,30]
        readonly property var colArmDark: [170,85,18]
        readonly property var colBase:    [58,64,80]
        readonly property var colBaseRim: [90,98,120]
        readonly property var colPost:    [85,92,110]
        readonly property var colServo:   [40,44,60]
        readonly property var colHorn:    [215,222,238]
        readonly property var colJaw:     [200,208,225]
        readonly property var colJawDark: [125,132,150]
        readonly property var colRubber:  [22,24,28]
        readonly property var colBolt:    [105,110,128]
        readonly property var colLed:     [140,255,170]
        readonly property var colLabel:   [210,215,225]

        property var keyLight:  vnorm([0.55,1.10,0.50])
        property var fillLight: vnorm([-0.60,0.25,0.75])
        property var rimLight:  vnorm([-0.20,0.40,-0.85])
        readonly property real ambient: 0.28

        function shade(color,material,normal){
            if(material===matEmissive) return color;
            var key=Math.max(0,vdot(normal,keyLight));
            var fill=Math.max(0,vdot(normal,fillLight))*0.45;
            var rim=Math.max(0,vdot(normal,rimLight))*0.25;
            var diffuse=ambient+(1.0-ambient)*(key*0.85+fill*0.55+rim*0.30);
            var half=vnorm(vadd(keyLight,viewDir));
            var ndh=Math.max(0,vdot(normal,half));
            var spec=Math.pow(ndh,material[1])*material[0];
            return [Math.min(255,Math.floor(color[0]*diffuse+255*spec)),
                    Math.min(255,Math.floor(color[1]*diffuse+255*spec)),
                    Math.min(255,Math.floor(color[2]*diffuse+255*spec))];
        }

        // ── Scene (identical to rov_gcs _build_scene) ─────────────────────
        readonly property real armAxisY: 1.15
        readonly property real armAxisZ: 0.0

        function buildScene(rollRad,jawHalfRad){
            var scene=[], AA=rollRad, i;
            function add(faces,col,mat){ for(var k=0;k<faces.length;k++) scene.push([faces[k],col,mat]); }
            function addRot(faces,col,mat){
                for(var k=0;k<faces.length;k++){
                    var face=faces[k], out=[];
                    for(var m=0;m<face.length;m++) out.push(canvas.rotateAxisX(face[m],canvas.armAxisY,canvas.armAxisZ,AA));
                    scene.push([out,col,mat]);
                }
            }
            add(buildPrismY([0,0.012,0],1.55,0.024,32),[28,30,40],matMatte);
            add(buildPrismY([0,0.12,0],0.85,0.24,24),colBase,matMatte);
            add(buildPrismY([0,0.245,0],0.88,0.02,24),colBaseRim,matMetal);
            for(i=0;i<6;i++){
                var a=2*Math.PI*i/6+Math.PI/12, bx=0.72*Math.cos(a), bz=0.72*Math.sin(a);
                add(buildPrismY([bx,0.27,bz],0.055,0.03,12),colBolt,matMetal);
            }
            add(buildBox([0,0.48,0],[0.5,0.46,0.5]),colPost,matMatte);
            add(buildBox([0,0.72,0],[0.44,0.04,0.44]),colBaseRim,matMetal);
            add(buildBevelBox([0,0.92,0],[0.58,0.36,0.58],0.02),colServo,matPlastic);
            add(buildBox([0,0.94,0.295],[0.34,0.08,0.01]),colLabel,matBrushed);
            add(buildPrismX([0.295,1.03,-0.18],0.022,0.01,12),colLed,matEmissive);
            addRot(buildPrismX([0.24,armAxisY,0],0.17,0.04,20),colHorn,matMetal);
            addRot(buildPrismX([0.42,armAxisY,0],0.20,0.28,20),colArm,matPaint);
            addRot(buildPrismX([1.45,armAxisY,0],0.135,2.00,16),colArm,matPaint);
            var xs=[0.85,2.00];
            for(i=0;i<2;i++) addRot(buildPrismX([xs[i],armAxisY,0],0.155,0.06,20),colArmDark,matPaint);
            addRot(buildBox([1.45,armAxisY-0.155,0],[1.8,0.04,0.10]),[32,34,42],matRubber);
            addRot(buildPrismX([2.55,armAxisY,0],0.22,0.30,22),colArm,matPaint);
            addRot(buildBevelBox([2.90,armAxisY,0],[0.42,0.42,0.42],0.02),colServo,matPlastic);
            addRot(buildPrismX([3.15,armAxisY,0],0.14,0.03,20),colHorn,matMetal);
            addRot(buildBox([3.22,armAxisY,0],[0.08,0.34,0.38]),colJawDark,matBrushed);

            function addJaw(pivot,sign){
                function pushJawBox(center,size,col,mat){
                    var faces=canvas.buildBox(center,size);
                    for(var k=0;k<faces.length;k++){
                        var face=faces[k], out=[];
                        for(var m=0;m<face.length;m++){
                            var v=canvas.rotatePivotY(face[m],pivot,sign*jawHalfRad);
                            out.push(canvas.rotateAxisX(v,canvas.armAxisY,canvas.armAxisZ,AA));
                        }
                        scene.push([out,col,mat]);
                    }
                }
                pushJawBox([pivot[0]+0.25,canvas.armAxisY,pivot[2]+sign*0.04],[0.45,0.10,0.09],canvas.colJaw,canvas.matBrushed);
                pushJawBox([pivot[0]+0.62,canvas.armAxisY,pivot[2]+sign*0.02],[0.32,0.07,0.07],canvas.colJaw,canvas.matBrushed);
                pushJawBox([pivot[0]+0.25,canvas.armAxisY,pivot[2]+sign*0.005],[0.38,0.07,0.015],canvas.colRubber,canvas.matRubber);
            }
            addJaw([3.27,armAxisY,-0.12],+1);
            addJaw([3.27,armAxisY,+0.12],-1);
            return scene;
        }

        function rgb(c){ return "rgb("+c[0]+","+c[1]+","+c[2]+")"; }

        onPaint: {
            var ctx=getContext("2d");
            var w=width, h=height;
            if(w<2||h<2) return;
            ctx.reset();

            // Background vignette (same palette as the Python radial rings)
            var g=ctx.createRadialGradient(w/2,h/2,0, w/2,h/2,Math.max(w,h)*0.75);
            g.addColorStop(0,"rgb(20,29,48)");
            g.addColorStop(1,"rgb(8,11,22)");
            ctx.fillStyle=g;
            ctx.fillRect(0,0,w,h);

            // Neon grid floor
            var size=5.0, step=0.4, n=Math.floor(size/step), i, v;
            for(i=-n;i<=n;i++){
                v=i*step;
                var major=(((i%5)+5)%5===0);
                var segs=[[[v,0,-size],[v,0,size]],[[-size,0,v],[size,0,v]]];
                for(var s=0;s<2;s++){
                    var c1=worldToCam(segs[s][0]), c2=worldToCam(segs[s][1]);
                    if(c1[2]<0.1||c2[2]<0.1) continue;
                    var p1=project(c1,w,h), p2=project(c2,w,h);
                    ctx.strokeStyle = major ? "rgb(80,95,130)" : "rgb(45,55,80)";
                    ctx.lineWidth   = major ? 2 : 1;
                    ctx.beginPath();
                    ctx.moveTo(p1[0],p1[1]);
                    ctx.lineTo(p2[0],p2[1]);
                    ctx.stroke();
                }
            }

            // Scene: shade, depth-sort, paint far-to-near
            var rollRad=_root.rollDeg*Math.PI/180.0;
            var ratio=Math.max(0,Math.min(1,_root.clawRatio));
            var jawHalfRad=ratio*45.0*Math.PI/180.0;
            var scene=buildScene(rollRad,jawHalfRad);
            var polys=[], k;
            for(i=0;i<scene.length;i++){
                var face=scene[i][0], base=scene[i][1], material=scene[i][2];
                if(face.length<3) continue;
                var camPts=[], bad=false;
                for(k=0;k<face.length;k++){
                    var cp=worldToCam(face[k]);
                    if(cp[2]<0.1){ bad=true; break; }
                    camPts.push(cp);
                }
                if(bad) continue;
                var n2=vnorm(vcross(vsub(face[1],face[0]),vsub(face[2],face[0])));
                var col=shade(base,material,n2);
                var avgZ=0;
                for(k=0;k<camPts.length;k++) avgZ+=camPts[k][2];
                avgZ/=camPts.length;
                var pts=[];
                for(k=0;k<camPts.length;k++) pts.push(project(camPts[k],w,h));
                polys.push({z:avgZ, pts:pts, col:col, emissive:(material===matEmissive)});
            }
            polys.sort(function(a,b){ return b.z-a.z; });

            ctx.lineWidth=1;
            for(i=0;i<polys.length;i++){
                var p=polys[i];
                ctx.beginPath();
                ctx.moveTo(p.pts[0][0],p.pts[0][1]);
                for(k=1;k<p.pts.length;k++) ctx.lineTo(p.pts[k][0],p.pts[k][1]);
                ctx.closePath();
                ctx.fillStyle=rgb(p.col);
                ctx.fill();
                if(!p.emissive){
                    ctx.strokeStyle=rgb([Math.max(0,p.col[0]-35),Math.max(0,p.col[1]-35),Math.max(0,p.col[2]-35)]);
                    ctx.stroke();
                }
            }
        }
    }
}
