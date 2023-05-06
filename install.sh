#!/usr/bin/env bash
set -e

SRC_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ "$1" != "" ]; then
  STEAMVR_DIR="$1"
else 
  STEAMVR_DIR="/home/$USER/.local/share/Steam/steamapps/common/SteamVR"
fi

VRPATHREG="$STEAMVR_DIR/bin/vrpathreg.sh"

if ! [ -f "$VRPATHREG" ]; then
  echo "SteamVR not found at $STEAMVR_DIR!"
  echo "Please pass your SteamVR folder as the 1st argument, like such:"
  echo "$0 /path/to/steamapps/common/SteamVR"
  exit 1
fi

echo "SteamVR found at $STEAMVR_DIR"

cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 $SRC_DIR
cp $SRC_DIR/lib/openvr/lib/linux64/libopenvr_api.so .

make -j $(nproc)
cp -r $SRC_DIR/OpenVR-SpaceCalibratorDriver/01spacecalibrator .
mkdir -p 01spacecalibrator/bin/linux64
cp OpenVR-SpaceCalibratorDriver/libopenvr-spacecalibratordriver.so 01spacecalibrator/bin/linux64/driver_01spacecalibrator.so

mkdir -p ~/.local/share/OpenVR-SpaceCalibrator

python3 $SRC_DIR/driverInstall.py --vrpathreg "$VRPATHREG" --toInstall "01spacecalibrator"

echo "Driver is now installed."
echo "Start SteamVR, then launch the companion software at:"
echo "./OpenVR-SpaceCalibrator/openvr-spacecalibrator"

