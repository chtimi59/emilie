package com.jdodev.emilie;

import android.hardware.Camera;
import android.hardware.Camera.Parameters;

public class LedFlash {
	
	Camera mCamera;
	Parameters mCameraSetting;
	
	public LedFlash() {
		 mCamera = Camera.open();
	     mCameraSetting = mCamera.getParameters();
	}
	
	public void release() {
		mCameraSetting.setFlashMode(Parameters.FLASH_MODE_OFF);
   	 	mCamera.setParameters(mCameraSetting);
   	 	mCamera.stopPreview();
	    mCamera = null; //Camera..open();
	}
	
	
	private Boolean mLedStatus = false;
	public Boolean getLedStatus() {
	     return this.mLedStatus;
	}
	
	public void setLedStatus(Boolean status) {
		if (mLedStatus == status) return;
		mLedStatus = status;
		if (mLedStatus) {
    		mCameraSetting.setFlashMode(Parameters.FLASH_MODE_TORCH);
    		mCamera.setParameters(mCameraSetting);
    		mCamera.startPreview();
    	} else {
    		mCameraSetting.setFlashMode(Parameters.FLASH_MODE_OFF);
	    	mCamera.setParameters(mCameraSetting);
	    	mCamera.stopPreview();
    	}
    }
	
	
}
