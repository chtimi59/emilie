package com.jdodev.emilie;

import com.jdodev.emilie.APSynch.STATE;
import com.jdodev.emilie.R;

import android.os.Bundle;
import android.os.Handler;
import android.widget.TextView;
import android.widget.Toast;
import android.app.Activity;
import android.app.AlertDialog;


public class MainActivity extends Activity implements APSynch.EventsListener {
	
	TextView mTv;
	LedFlash mLed;
	APSynch  mAp;
	Handler  mHandler = new Handler();

	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mTv = (TextView)findViewById(R.id.label) ;
        mLed = new LedFlash();
        mAp  = new APSynch(this, this);
        mAp.start(null);
    }
    
    
    protected void onPause() {
        super.onPause();
     }

     protected void onResume() {
        super.onResume();
     }

	 @Override
	 public void onDestroy() {
	     mLed.release();	
	     mAp.release();
	     super.onDestroy();
	 }
 

	@Override
	public void onAPSync() {
		String msg = "Synch to " + mAp.getBSSID() + " " + mAp.getFreq() + "MHz " + " scan " + mAp.getScanRate() + "ms";
		Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();
		mHandler.postDelayed(updateTimerThread, 0);
	}


	@Override
	public void onAPOutOfSync() {
		String msg = "Out of synch!";
		Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();		
	}


	@Override
	public void onAPEvent(String data) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onAPError(int errorCode) {
		String msg = "Error " + errorCode;
		Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();		
	}
    
   
	private Runnable updateTimerThread = new Runnable() {
		public void run() {
			if (mAp.getStatus()==STATE.SYNC) {
				
				long ms = mAp.getTimeInMiliseconds();
				
				if (ms % 500 == 0) {
					mLed.setLedStatus(!mLed.getLedStatus());
				}
				
				mHandler.postDelayed(updateTimerThread, 0);				
			}
	    };
	};
	
}
