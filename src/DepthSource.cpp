#include "DepthSource.h"

DepthSource::DepthSource()
{
	width = 0;
	height = 0;
	backPixels = NULL;
	currentPixels = NULL;
    oniDepthPixels = NULL;
	deviceMaxDepth = 0;
	doDoubleBuffering = true;
	doRawDepth = false;
	isOn = false;
    nearClipping=400;
    farClipping=4000;
}

bool DepthSource::setup(DeviceController& deviceController)
{
	doRawDepth = deviceController.settings.doRawDepth;

	Status status = STATUS_OK;
	status = videoStream.create(deviceController.device, SENSOR_DEPTH);
	if (status == STATUS_OK)
	{
		ofLogVerbose() << "Find DepthSource stream PASS";
		status = videoStream.start();
		if (status == STATUS_OK)
		{
			ofLogVerbose() << "Start DepthSource stream PASS";
		}else
		{

			ofLogError() << "Start DepthSource stream FAIL: " << OpenNI::getExtendedError();
			videoStream.destroy();
		}
	}else
	{
		ofLogError() << "Find DepthSource stream FAIL: " <<  OpenNI::getExtendedError();
	}
	if (videoStream.isValid())
	{
		if(!deviceController.settings.useOniFile && !deviceController.isKinect)
		{
			const VideoMode* requestedMode = deviceController.findMode(SENSOR_DEPTH);
			if (requestedMode != NULL)
			{
				videoStream.setVideoMode(*requestedMode);
			}
		}
		allocateBuffers();


		if (!deviceController.isKinect)
		{
			deviceMaxDepth	= videoStream.getMaxPixelValue();
		}else
		{
			deviceMaxDepth = 10000;
		}
		ofLogVerbose() << "deviceMaxDepth: " << deviceMaxDepth;
		status = videoStream.addNewFrameListener(this);
		if (status == STATUS_OK)
		{
			ofLogVerbose() << "DepthSource videoStream addNewFrameListener PASS";
		}else
		{
			ofLogError() << "DepthSource videoStream addNewFrameListener FAIL: " <<  OpenNI::getExtendedError();
		}



		isOn = true;
	}else
	{
       
		ofLogError() << "DepthSource is INVALID";
        isOn = false;
	}


	return isOn;
}
void DepthSource::update()
{
	texture.loadData(*currentPixels);
}

void DepthSource::draw()
{
	texture.draw(width, 0);
}

void DepthSource::allocateBuffers()
{
	videoMode = videoStream.getVideoMode();
	width = videoMode.getResolutionX();
	height = videoMode.getResolutionY();

    

    
    currentPixels = new ofPixels();
    currentPixels->allocate(width, height, OF_IMAGE_COLOR_ALPHA);
    
    noAlphaPixels = new ofPixels();
    noAlphaPixels->allocate(width, height, OF_IMAGE_GRAYSCALE);
    
    if (doDoubleBuffering) 
    {
        ofLogVerbose(__func__) << "doDoubleBuffering: " << doDoubleBuffering;
        backPixels = new ofPixels();
        backPixels->allocate(width, height, OF_IMAGE_COLOR_ALPHA);
        
    }
 
    int dataSize = width*height*4; 
    oniDepthPixels = new DepthPixel[dataSize];
    
	if (doRawDepth)
	{
        currentRawPixels = new ofShortPixels();
        currentRawPixels->setFromExternalPixels(oniDepthPixels, width, height, 1);
        
        if (doDoubleBuffering) 
        {
            ofLogVerbose(__func__) << "doDoubleBuffering: " << doDoubleBuffering;
            backRawPixels = new ofShortPixels();
            backRawPixels->setFromExternalPixels(oniDepthPixels, width, height, 1);
        }
	}

    texture.allocate(width, height, GL_RGBA);
}

void DepthSource::onNewFrame(VideoStream& stream)
{
    Status status = stream.readFrame(&videoFrameRef);
    if (status != STATUS_OK)
    {
        ofLogError() << "DepthSource readFrame FAIL: " <<  OpenNI::getExtendedError();
    }
    
    oniDepthPixels = (DepthPixel*)videoFrameRef.getData();

    if (doRawDepth)
    {
        if (doDoubleBuffering)
        {
            swap(backRawPixels,currentRawPixels);
        }
    }

    ofPixels* pixelBuffer = currentPixels;

    if (doDoubleBuffering)
    {
        pixelBuffer = backPixels;
    }

    float max = 255;
    unsigned char nearColor=255;
    unsigned char farColor=20;
    for (unsigned short y = 0; y < height; y++)
    {
        unsigned char * pixel = pixelBuffer->getPixels() + y * width * 4;
        unsigned char * pixelna = noAlphaPixels->getPixels() + y * width;
        for (unsigned short  x = 0;
             x < width;
             x++,
             oniDepthPixels++,
             pixel += 4,
             pixelna++)
        {
            unsigned char red = 0;
            unsigned char green = 0;
            unsigned char blue = 0;
            unsigned char alpha = 255;

            if( (*oniDepthPixels > nearClipping) && (*oniDepthPixels< farClipping) )
            {
                unsigned char a = ofMap(*oniDepthPixels, nearClipping, farClipping, nearColor, farColor, true);
                red		= a;
                green	= a;
                blue	= a;
                pixel[0] = red;
                pixel[1] = green;
                pixel[2] = blue;

                pixelna[0] = a;

                if (*oniDepthPixels == 0)
                {
                    pixel[3] = 0;
                } else
                {
                    pixel[3] = alpha;
                }
            }else
            {
                pixel[0] = 0;
                pixel[1] = 0;
                pixel[2] = 0;
                pixel[3] = 255;

                pixelna[0] = 0;
            }


        }
    }
    if (doDoubleBuffering)
    {
        swap(backPixels,currentPixels);
    }

}

void DepthSource::setDepthClipping(float nearClip, float farClip){
    nearClipping=nearClip;
    farClipping=farClip;
}

void DepthSource::close()
{
	ofLogVerbose() << "DepthSource close";
	isOn = false;
	videoStream.stop();
	videoStream.removeNewFrameListener(this);
	videoStream.destroy();
}
