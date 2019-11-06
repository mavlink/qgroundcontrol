#!/bin/bash

# Start virtual X server in the background if there is no display
if [[ -x "$(command -v Xvfb)" && ! -z "$DISPLAY" ]]; then
	echo "Starting Xvfb"
	Xvfb :99 -screen 0 1600x1200x24+32 &
	export DISPLAY=:99
fi

# Add local user
# Either use the LOCAL_USER_ID if passed in at runtime or
# fallback

if [ -n "${LOCAL_USER_ID}" ]; then

	echo "Starting with UID : $LOCAL_USER_ID"

	# modify existing user's id
	usermod -u $LOCAL_USER_ID user

	exec gosu user "$@"

else

	exec "$@"
fi
