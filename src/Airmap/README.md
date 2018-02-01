## To be deleted when development is complete

*   Traffic monitor timeout is now set to 2 minutes following instructions from Thomas Voß.

*   Group rules jurisdictions

*   ~Check rules sorting order. Repopulating QmlObjectListModel causes the repeater to show the contents in random order.~

*   ~AirMapRestrictionManager seems to be incomplete (if not redundant). Shouldn't we use the array of AirSpace items returned by AirMapAdvisoryManager instead? In addition to the AirSpace object, it gives you a color to use in order of importance.~ See question below.

*   ~"Required" rules must be pre-selectd and cannot be unselected by the user~

*   ~Add a "Wrong Way" icon to the airspace widget when not connected~



Questions:

Given a same set of coordinates, what is the relationship between the `Airspace` elements returned by `Airspaces::Search` and those returned by `Status::GetStatus` (Advisories)? The former don’t have a “color”. The latter do but those don’t have any geometry. In addition, given two identical set of arguments (coordinates), the resulting sets are not the same. A given airspace may also be repeated several times with different “levels”, such as an airport having several “green” and several “yellow” advisories (but no red or orange). How do you filter what to show on the screen? 

Also, the main glaring airspace nearby, KRDU has one entry in Airspaces whose ID is not in the array returned by the advisories (and therefore no color to assign to it). In fact, KRDU shows up 20 times in Advisories and 11 times in Airspaces.

In summary, why do I have to make two requests to get two arrays of Airspaces, which should be mostly the same?
