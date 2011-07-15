xcodebuild -sdk iphonesimulator4.3 -configuration Release -target JavascriptCore-Static clean
xcodebuild -sdk iphoneos4.3 -configuration Release -target JavascriptCore-Static clean

xcodebuild -sdk iphonesimulator4.3 -configuration Release -target JavascriptCore-Static
xcodebuild -sdk iphoneos4.3 -configuration Release -target JavascriptCore-Static

python lipoit.py
