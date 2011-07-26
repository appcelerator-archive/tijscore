xcodebuild -sdk iphonesimulator -configuration Release -target iOSTiCore clean
xcodebuild -sdk iphoneos -configuration Release -target iOSTiCore clean

xcodebuild -sdk iphonesimulator -configuration Release -target iOSTiCore
xcodebuild -sdk iphoneos -configuration Release -target iOSTiCore

python lipoit.py
