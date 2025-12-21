#!/bin/bash
# ArduCopter SITL Test Environment
# Runs ArduCopter in Docker for testing QGC without hardware
#
# Usage: ./run-arducopter-sitl.sh [--with-latency]
#
# Options:
#   --with-latency  Simulate 100ms round-trip latency (Herelink-like conditions)

set -e

COPTER_VERSION="Copter-4.5.6"
IMAGE_NAME="ardupilot-sitl-4.5.6"
CONTAINER_NAME="arducopter-sitl"

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    exit 1
fi

# Build the image if it doesn't exist
if ! docker image inspect "$IMAGE_NAME" &> /dev/null; then
    echo "Building ArduCopter $COPTER_VERSION SITL image..."
    echo "This may take 10-15 minutes on first run..."
    docker build --tag "$IMAGE_NAME" \
        --build-arg COPTER_TAG="$COPTER_VERSION" \
        https://github.com/radarku/ardupilot-sitl-docker.git
fi

# Stop any existing container
docker rm -f "$CONTAINER_NAME" 2>/dev/null || true

# Base command - run ArduCopter directly with TCP on port 5760
DOCKER_CMD="docker run -d --name $CONTAINER_NAME -p 5760:5760"
ARDUPILOT_CMD="/ardupilot/build/sitl/bin/arducopter -S --model + --speedup 1 --defaults /ardupilot/Tools/autotest/default_params/copter.parm --home 42.3898,-71.1476,14.0,270.0 --serial0 tcp:0:5760:wait"

# Check for latency simulation flag
if [[ "$1" == "--with-latency" ]]; then
    echo ""
    echo "Starting SITL with simulated latency (100ms round-trip)..."
    echo "This simulates Herelink-like network conditions."
    echo ""

    # Run container with tc (traffic control) for latency
    $DOCKER_CMD --cap-add=NET_ADMIN --entrypoint /bin/bash "$IMAGE_NAME" -c "tc qdisc add dev eth0 root netem delay 50ms 10ms 2>/dev/null || true; exec $ARDUPILOT_CMD"
else
    echo ""
    echo "Starting SITL (no latency simulation)..."
    echo "Use --with-latency to simulate Herelink network conditions."
    echo ""

    $DOCKER_CMD --entrypoint /ardupilot/build/sitl/bin/arducopter "$IMAGE_NAME" \
        -S --model + --speedup 1 \
        --defaults /ardupilot/Tools/autotest/default_params/copter.parm \
        --home 42.3898,-71.1476,14.0,270.0 \
        --serial0 tcp:0:5760:wait
fi

# Wait for startup
sleep 3

# Check if running
if docker ps --filter name="$CONTAINER_NAME" --format "{{.Status}}" | grep -q "Up"; then
    echo ""
    echo "============================================"
    echo "ArduCopter $COPTER_VERSION SITL is running!"
    echo "============================================"
    echo ""
    echo "Connect QGC to: tcp://localhost:5760"
    echo ""
    echo "Test procedure:"
    echo "  1. Open QGC and connect to localhost:5760"
    echo "  2. Go to Plan view"
    echo "  3. Create a mission with waypoints (no geofence)"
    echo "  4. Click Upload"
    echo "  5. Verify no 'Geofence Transfer failed' error appears"
    echo ""
    echo "Commands:"
    echo "  View logs:  docker logs -f $CONTAINER_NAME"
    echo "  Stop SITL:  docker stop $CONTAINER_NAME"
    echo "  Remove:     docker rm $CONTAINER_NAME"
    echo ""
else
    echo "Error: SITL container failed to start"
    echo "Logs:"
    docker logs "$CONTAINER_NAME" 2>&1 | tail -20
    exit 1
fi
