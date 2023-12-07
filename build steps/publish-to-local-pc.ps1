#- Remove files from target directory
#- copy over new files
Remove-Item -Path "C:\Users\George\startup\HeadTracker Latest\*.*"
Copy-Item -Path "C:\Users\George\projects\TrackIRMouse\bin\x64\Release\*" -Destination "C:\Users\George\startup\HeadTracker Latest" -Recurse
