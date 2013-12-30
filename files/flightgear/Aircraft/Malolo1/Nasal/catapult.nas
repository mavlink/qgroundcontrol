var launchCatapult = func {
	# time on catapult = 1/10 sec
	# speed when leaving catapult = 50 km/h ?
	var countdownRunning = 1;
	var count = 3;
	var countdown = func {
		if (countdownRunning) {
			if (count != 0) {
				setprop("/sim/screen/white",count);
				count = count - 1;
				settimer(countdown, 1);
			}
			else {
				countdownRunning = 0;
				setprop("/sim/screen/yellow","Go!");
				launch();
			}
		}
	}
	countdown();
	
	var launchRunning = 1;
	var magnitude = 230; # lbs, unrealisticly high, because the FDM is wrong
	var launch = func {
		if (launchRunning) {
			if (magnitude == 0){
				launchRunning = 0;
				
				# remove launcher contact points 
				setprop("/fdm/jsbsim/contact/unit[6]/pos-norm",0);
				setprop("/fdm/jsbsim/contact/unit[7]/pos-norm",0);
				setprop("/fdm/jsbsim/contact/unit[8]/pos-norm",0);
			}
			setprop("/fdm/jsbsim/external_reactions/catapult/magnitude",magnitude);
			print (magnitude);
			magnitude = 0;
			settimer(launch, 0.1);
		}
	}
}
