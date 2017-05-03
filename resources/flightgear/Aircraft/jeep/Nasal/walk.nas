var sin = func(a) { math.sin(a * math.pi / 180.0) }
var cos = func(a) { math.cos(a * math.pi / 180.0) }
var posx= 0;
var posy= 0;
var posz= 0;

 setlistener("sim/walker/walking", func {
	walk();
 });

var main_loop = func {
	if (getprop ("sim/walker/outside") == 0) {
		setprop ("sim/walker/latitude-deg" , (getprop ("position/latitude-deg")));
		setprop ("sim/walker/longitude-deg" , (getprop ("position/longitude-deg")));
		setprop ("sim/walker/altitude-ft" , (getprop ("position/altitude-ft")));
		setprop ("sim/walker/roll-deg" , (getprop ("orientation/roll-deg")));
		setprop ("sim/walker/pitch-deg" , (getprop ("orientation/pitch-deg")));
		setprop ("sim/walker/heading-deg" , (getprop ("orientation/heading-deg")));
	}
  settimer(main_loop, 0.008)
}
var walk = func {


				if (getprop ("sim/current-view/view-number") == view.indexof("Walk View")) {
				if (getprop ("devices/status/mice/mouse/mode") == 2){


					if (getprop ("sim/walker/outside") == 1) {
						ext_mov();
					} else {
						int_mov();
					}
		}
	}
}

var ext_mov = func {
	speed = getprop ("sim/walker/speed");
	head = getprop ("sim/current-view/heading-offset-deg");
	posy = getprop ("sim/walker/latitude-deg");
	posx = getprop ("sim/walker/longitude-deg");
	posz1 = getprop ("sim/walker/altitude-ft");
	posx1 = posx - speed*sin(head);
	if (posy < 0 ) {
		# southern hemisphere
		posy1 = posy + (speed+0.000001*sin(posy))*cos(head);
#		print ("south");
		} else {
		# northern hemisphere
#		print ("north");
		posy1 = posy + (speed-0.000001*sin(posy))*cos(head);
		}
#	print (head,"  ",speed,"  ",speed+0.0000001*sin(posy));
#	print (cos(head),"  ",sin(head));
#	print (posy,"  ",speed+0.000001*sin(posy));
	posz2 = geo.elevation (posy1,posx1);

	if ((posz2 * 3.28084) < (posz1+10)) {
		interpolate ("sim/walker/latitude-deg", posy1,0.25,0.3);
#		print (posy);
		interpolate ("sim/walker/longitude-deg", posx1,0.25,0.3);
#		print (posx);

#		print (posz1," ", posz2 * 3.28084);
		if ((posz1+0.4) > (posz2 * 3.28084) or (posz1-0.4) < posz2 * 3.28084) {
			interpolate ("sim/walker/altitude-ft", posz2 * 3.28084 ,0.25,0.3);
		}
	}
if (getprop ("sim/walker/walking") == 1) {
	settimer(ext_mov, 0.25);
	}
}
var get_out = func {
	if (getprop ("sim/current-view/view-number") == view.indexof("Walk View")) {
		posx = getprop ("position/longitude-deg");
		posy = getprop ("sim/walker/latitude-deg");
		head = getprop ("orientation/heading-deg");
		posy1  = posy + (0.000001*sin(posy))*sin(head);

		setprop ("sim/walker/heading-deg", 0);
		setprop ("sim/walker/outside", 1);
		setprop ("sim/current-view/x-offset-m", 0);
		setprop ("sim/current-view/y-offset-m", 1.87);
		setprop ("sim/current-view/z-offset-m", 0);
		setprop ("sim/current-view/roll-offset-deg", 0);
	}
}	

var get_in = func {
	if (getprop ("sim/current-view/view-number") == view.indexof("Walk View")) {
		setprop ("sim/walker/outside", 0);
		setprop ("sim/current-view/x-offset-m", 0.35);
		setprop ("sim/current-view/y-offset-m", 1.5);
		setprop ("sim/current-view/z-offset-m", 2.295);
		setprop ("sim/current-view/roll-offset-deg", 0);
	}
}
var int_mov = func {
print ("Not yet implemented");
}
