var nofuel = props.globals.getNode("engines/engine[0]/out-of-fuel",1 );


var kill_engine = func {
	nofuel.setValue(1);
	nofuel.setAttribute("writable", 0);
#	interpolate ("/engines/engine[0]/fuel-press", 0, 1);
#	interpolate ("/engines/engine[0]/mp-osi", 0, 1.5);
	
}
