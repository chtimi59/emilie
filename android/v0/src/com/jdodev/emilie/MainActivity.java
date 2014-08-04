package com.jdodev.emilie;

import java.text.DecimalFormat;

import com.jdodev.emilie.APSynch.STATE;
import com.jdodev.emilie.R;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import android.app.Activity;
import android.graphics.Canvas;



public class MainActivity extends Activity implements APSynch.EventsListener {
	
	Button mBtn;
	TextView mTv;
	MyGraph mGraph;
	ProgressBar mProgressBar;
	
	Canvas   mCanvas;
	LedFlash mLed;
	APSynch  mAp;
	Handler  mHandler = new Handler();
	
	int mMarker = 1;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        mBtn = (Button)findViewById(R.id.button) ;
        mProgressBar = (ProgressBar)findViewById(R.id.progressBar) ;
        mTv = (TextView)findViewById(R.id.label) ;
        mGraph = (MyGraph) findViewById(R.id.rect);
        mLed = new LedFlash();
        mAp  = new APSynch(this, this);

        mBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v)
            {
            	switch (mAp.getStatus())
            	{
            	   	case IDLE:
            	   		mAp.start(null);
            	   		break;
            	   	default:
            	   		mAp.stop();
            	   		break;
            	   		
            	}                
            }
        });
        
       ((Button)findViewById(R.id.un)).setOnClickListener(new View.OnClickListener() {public void onClick(View v) { mMarker = 1; }});
       ((Button)findViewById(R.id.deux)).setOnClickListener(new View.OnClickListener() {public void onClick(View v) { mMarker = 2; }});
       ((Button)findViewById(R.id.trois)).setOnClickListener(new View.OnClickListener() {public void onClick(View v) { mMarker = 3; }});
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
	public void onAPStateChange(STATE state) {
		mTv.setText(state.toString());
		switch (state)
		{
			
			case UNUSABLE:
				mGraph.Clear();
				mBtn.setEnabled(false);
				mBtn.setText("--");
				mProgressBar.setVisibility(View.INVISIBLE);
				break;

			case IDLE:
				mGraph.Clear();
				mBtn.setEnabled(true);
				mBtn.setText("Start");
				mProgressBar.setVisibility(View.INVISIBLE);
				break;
				
			case CAPTURING:
				mGraph.Clear();
				mBtn.setEnabled(true);
				mBtn.setText("Stop");
				mProgressBar.setVisibility(View.VISIBLE);
				break;
				
			case SYNC:
				mGraph.Clear();
				mBtn.setEnabled(true);
				mBtn.setText("Stop");
				mProgressBar.setVisibility(View.INVISIBLE);
				
				float err = 100 - mAp.getScanDurationError() / mAp.getScanDuration() * 100;
				String serr = new DecimalFormat("#.##").format(err);
				//String msg = mAp.getFreq() + "MHz ";
				String msg = mAp.getSSID();
				msg += " " + mAp.getScanDuration() + "ms trust: " + serr + "%";
				Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();				
				mHandler.postDelayed(updateTimerThread, 0);
				break;
				
		}
	}


	@Override
	public void onAPDataReceived(String data) {
		Toast.makeText(getApplicationContext(), data, Toast.LENGTH_SHORT).show();		
	}

	
	@Override
	public void onAPError(int errorCode) {
		String msg = "Error " + errorCode;
		Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();		
	}
    
	@Override
	public void onAPCaptureProgress(int pos, int count) {
		mTv.setText(mAp.getStatus().toString() + " " + pos + "/" + count );		
	}
	
	
	
    
	@Override
	public void onAPSynchUpdated() {
		mTv.setText("" + mAp.GRRRR);
		long cur = System.currentTimeMillis();
		long ms = mAp.getTimeInMiliseconds();
		mGraph.addMeasurement(cur, ms);
	}

	
	
	
	
    static int ANIM_POS = 0;
    final static long ANIM_COUNT = 3;    
    final static long ANIM_FRAME = 500; // 500ms    
    final static Boolean ANIM1[] = {  true, false, false  };
    final static Boolean ANIM2[] = {  false, true, false  };
    final static Boolean ANIM3[] = {  false, false, true  };
    
    
	private Runnable updateTimerThread = new Runnable() {
		public void run() {
			if (mAp.getStatus()==STATE.SYNC)
			{
				long ms = mAp.getTimeInMiliseconds();
				long frame = ms/ANIM_FRAME;
				int pos = (int)(frame % ANIM_COUNT);
				if (pos!=ANIM_POS)
				{
					ANIM_POS = pos;
					Log.d("ANIM!", "pos:" + ANIM_POS);
				
					switch(mMarker) {
						case 1: mLed.setLedStatus(ANIM1[ANIM_POS]); break;
						case 2: mLed.setLedStatus(ANIM2[ANIM_POS]); break;
						case 3: mLed.setLedStatus(ANIM3[ANIM_POS]); break;
					}
				
				}
				mHandler.postDelayed(updateTimerThread, 0);
			}
	    };
	};




	
	
}
