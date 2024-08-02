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

mkdir -p "/home/$USER/.local/share/icons"
mkdir -p "/home/$USER/.local/share/applications"
cd ..
cp "OpenVR-SpaceCalibrator/icon.png" "/home/$USER/.local/share/icons/OpenVR-SpaceCalibrator.png"
cp "openvrspacecalibrator.desktop" "openvrspacecalibrator.desktop.copy"
sed -i -e "s|change-path-1|$PWD/build/OpenVR-SpaceCalibrator/openvr-spacecalibrator|g" "openvrspacecalibrator.desktop.copy"
sed -i -e "s|change-path-2|$PWD/build/OpenVR-SpaceCalibrator|g" "openvrspacecalibrator.desktop.copy"
mv "openvrspacecalibrator.desktop.copy" "/home/$USER/.local/share/applications/openvrspacecalibrator.desktop"

echo "Driver is now installed."
echo "Start SteamVR, then launch the companion software from desktop programs or in terminal:"
echo "./OpenVR-SpaceCalibrator/openvr-spacecalibrator"



