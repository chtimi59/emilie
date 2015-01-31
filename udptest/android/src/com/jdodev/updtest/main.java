package com.jdodev.updtest;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.ToggleButton;

public class main extends Activity implements UDPClient.Listener {
	
	private main mCtx = null;
	
	private TextView mTxtInformation = null;
	private ToggleButton mStartStopBtn = null;
	private Spinner mAddressView = null;
	private CheckBox mLightCheckBoxView = null;
	
	private UDPClient mRxThread = null;
	private PowerManager mPowerManager = null;
	protected PowerManager.WakeLock mWakeLockScreen = null;
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mCtx = this;
        
        mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mTxtInformation = (TextView) findViewById(R.id.textView1);
        mStartStopBtn = (ToggleButton)findViewById(R.id.toggleButton1);
        mAddressView = (Spinner)findViewById(R.id.spinner1);
        mLightCheckBoxView= (CheckBox)findViewById(R.id.CheckBox1);

        mTxtInformation.setText("");
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, new String[] {
        		"Group 1", "Group 2", "Group 3", "Group 4", "Group 5", "Group 6", "Group 7", "Group 8" });
        mAddressView.setAdapter(adapter);
        
        mLightCheckBoxView.setOnClickListener(new OnClickListener() {
        	@Override
			public void onClick(View v) {
        		
        		udpstart();
			}
		});
        
        mStartStopBtn.setOnClickListener(new OnClickListener() {
        	@Override
			public void onClick(View v) {
        		if (mRxThread == null && mStartStopBtn.isChecked()) {
        			mRxThread = new UDPClient(mCtx, mCtx);
        			mRxThread.start();
        			mStartStopBtn.setTo
        		} else {
        			
        		}
			}
		});
        
        mStopBtn.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				udpstop();
			}
		});
        
        
    }
	
	private void udpstart() {
		 WifiManager wifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
	        if (wifi != null){
	        	mLock1 = wifi.createWifiLock("mylock");
	        	if (mLock1!=null) mLock1.acquire();
	        	mLock2=wifi.createMulticastLock("mylock2");
	        	if (mLock2!=null) mLock2.acquire();
	        	
	        	if ((mLock1!=null) && (mLock1!=null)) {
	        		mRxThread = new Client();
	        		mRxThread.start();
	        	} 	
	        }
	        
	        if (mRxThread==null) {
	        	mTxtInformation.setText("No Wifi!");
	        } else {
	        	mTxtInformation.setText("Wait for "+MULTICAST_ADDR+":"+UDP_PORT);
	            mWakeLockScreen = mPowerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, "My Tag");
	            if (mWakeLockScreen!=null) mWakeLockScreen.acquire();
	        }
	}
	
	private void udpstop() {
		if (mRxThread!=null) mRxThread.shutdown();
	    mRxThread = null;
	   
	    if (mWakeLockScreen!=null) mWakeLockScreen.release();;
	    mWakeLockScreen = null;
	    mTxtInformation.setText("Bye!");		
	}

	@Override
	public void onBackPressed() {
		this.finish();
	    super.onBackPressed();
	}
	
	@Override
	public void onDestroy() {
		udpstop();
	    super.onDestroy();
	}

	@Override
	public void OnThreadStart() {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void OnDataReceived(UDPFrame frame) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void OnThreadEnd() {
		// TODO Auto-generated method stub
		
	}
	
	
	
	
	

	
	
	
	
}
