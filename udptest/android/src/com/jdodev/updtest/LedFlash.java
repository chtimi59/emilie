package com.jdodev.updtest;

import java.io.IOException;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.util.Log;

public class LedFlash {
	
	Boolean mDeviceHasFlash;
	Camera mCamera=null;
	
	Context mCtx;
	
	public Boolean IsAvailable() { return mDeviceHasFlash; }
	
	public LedFlash(Context ctx) {
		mCtx = ctx;
		mDeviceHasFlash = mCtx.getApplicationContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA_FLASH); 
		if (!mDeviceHasFlash) return;
		if (!getCamera()) {
			mDeviceHasFlash = false;
			return;
		}		 
	}
	
	SurfaceTexture surface = null;
	
	@TargetApi(11)
	private void api11setPreviewTexture(Camera camera) throws IOException {
		surface = new SurfaceTexture(0);
		camera.setPreviewTexture(surface);
	}
	
	private boolean getCamera() {
		
		if (mCamera != null) return true; 
		try {
			mCamera = Camera.open();
		} catch (RuntimeException e) {
            Log.e("LEDFLASH", "Flash light error 1", e);
            return false;
        } 
		
    	try {
    		api11setPreviewTexture(mCamera);
    	} catch (IOException e) {
   	 		Log.e("LEDFLASH", "Flash light error 2", e);
   	 		return false;
		}
            	
    	return true;
	}
	
	public void release() {
		if (mCamera != null) {
			Parameters p;
			p = mCamera.getParameters();
			p.setFlashMode(Parameters.FLASH_MODE_OFF);
	   	 	mCamera.setParameters(p);
	   	 	mCamera.stopPreview();
	   	 	mCamera.release();
		    mCamera = null; //Camera..open();
		}
	}
	
	
	private Boolean mLedStatus = false;
	public Boolean getLedStatus() {
	     return this.mLedStatus;
	}
	
	public void setLedStatus(Boolean status) {
		if (mCamera==null) return;
		if (mLedStatus == status) return;
		mLedStatus = status;
		Parameters p;
		try {
			if (mLedStatus) {
				p = mCamera.getParameters();
	    		p.setFlashMode(Parameters.FLASH_MODE_TORCH);
	    		mCamera.setParameters(p);
	    		mCamera.startPreview();
	    	} else {
	    		p = mCamera.getParameters();
	    		p.setFlashMode(Parameters.FLASH_MODE_OFF);
		    	mCamera.setParameters(p);
		    	mCamera.stopPreview();		    	
	    	}
		} catch (Exception e) { }
    }
	
}
