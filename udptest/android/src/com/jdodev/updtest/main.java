package com.jdodev.updtest;
import com.jdodev.updtest.UDPFrame.TESTFrame;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ToggleButton;

public class main extends Activity implements UDPClient.Listener {
	
	public final static String TAG = "UDPTST";
	private main mCtx = null;
	
	private TextView mTxtInformation = null;
	private ToggleButton mStartStopBtn = null;
	private Spinner mAddressView = null;
	private CheckBox mLightCheckBoxView = null;
	
	private UDPClient mRxThread = null;
	private PowerManager mPowerManager = null;
	private PowerManager.WakeLock mWakeLockScreen = null;

	private LedFlash mLed;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mCtx = this;
        
        mLed = new LedFlash(mCtx);
        
        mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mTxtInformation = (TextView) findViewById(R.id.textView1);
        mStartStopBtn = (ToggleButton)findViewById(R.id.toggleButton1);
        mAddressView = (Spinner)findViewById(R.id.spinner1);
        mLightCheckBoxView= (CheckBox)findViewById(R.id.CheckBox1);

        mLightCheckBoxView.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				if (!mLightCheckBoxView.isChecked()) mLed.setLedStatus(false);				
			}
        });
        
        mTxtInformation.setText("Off line");
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, new String[] {
        		"Group 1", "Group 2", "Group 3", "Group 4", "Group 5", "Group 6", "Group 7", "Group 8" });
        mAddressView.setAdapter(adapter);
        
        mStartStopBtn.setOnClickListener(new OnClickListener() {
        	@Override
			public void onClick(View v) {
        		if (mStartStopBtn.isChecked()) {
        			if (mRxThread == null) {
        				mRxThread = new UDPClient(mCtx, mCtx);
        				mRxThread.start();
        			}
        		} else {
        			if (mRxThread!=null) {
        				mTxtInformation.setText("Disconnecting...");
        				mRxThread.shutdown();
        				mStartStopBtn.setChecked(true);
        			}
        		}
			}
		});
      
        
        
    }
	
	private void udpstop() {
		mTxtInformation.setText("Off line");
		if (mRxThread!=null) mRxThread.shutdown();
		if (mWakeLockScreen!=null) mWakeLockScreen.release();
		mWakeLockScreen = null;
	}

	
	@Override
	public void onBackPressed() {
		this.finish();
	    super.onBackPressed();
	}
	
	@Override
	public void onDestroy() {
		udpstop();
		mLed.release();
	    super.onDestroy();
	}

	
	@Override
	public void OnThreadStart() {
		mTxtInformation.setText("Wait for "+UDPClient.MULTICAST_ADDR+":"+UDPClient.UDP_PORT);
		if (mWakeLockScreen==null) {
			mWakeLockScreen = mPowerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, "My Tag");
			if (mWakeLockScreen!=null) mWakeLockScreen.acquire();
		}
	}

	@Override
	public void OnThreadEnd() {
		udpstop();
		mRxThread = null;
		mStartStopBtn.setChecked(false);
	}
	
	@Override
	public void OnDataReceived(UDPFrame frame) {
		mTxtInformation.setText(frame.toString());
		long posIdx = mAddressView.getSelectedItemId();
		long myAddress = 1 << posIdx;
		
		switch(frame.frametype) {
			case TESTFrame.ID:
				UDPFrame.TESTFrame tstfrm = (UDPFrame.TESTFrame)frame.payload;
				switch (tstfrm.keycode) {
					case 0x0d:
						//udpstop();
						break;
					case 0x32:
						if ( mLightCheckBoxView.isChecked() && 
							 (frame.address == 0xFFFFFFFFl || frame.address == myAddress)
							) {
							mLed.setLedStatus(true);
						}
						break;
					case 0x31:
						if ( mLightCheckBoxView.isChecked() && 
							 (frame.address == 0xFFFFFFFFl || frame.address == myAddress)
							) {
							mLed.setLedStatus(false);
						}
						break;
				}
				
		}
		
	}

	
}
