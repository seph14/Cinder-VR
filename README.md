# Cinder-VR

Cinder-VR fork with PlaystationVR support.  

##### Usage
Compile the libraries first. And then add this block to your projects.

##### Dependency
This fork uses my [Cinder-PSVR](https://github.com/seph14/Cinder-PSVR.git) for PSVR control and sensor fusion handling. 

##### Behaviour
When connected to PSVR, the rendering will be default to stereo-undistorted, when the headset is worn, it will automatically switch to distorted rendering, as the PSVR break box will un-distort the image and map it to your screen.

##### Known Issues
VR mode do not work for Macbook Air (or mac mini): the outputed monitor will show things correctly, but the display on the headset is black.   
It might be related to graphic card pixel formats. But need more investigation at the moment.