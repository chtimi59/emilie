package com.example.test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.annotation.Annotation;
import java.util.List;

import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.WifiLock;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.PowerManager;
import android.util.Log;
import android.widget.TextView;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class MainActivity extends Activity {

	PowerManager.WakeLock mWakeLock;
	WifiLock mWifiLock;
	WifiManager mMainWifiObj;
	WifiScanReceiver mWifiReciever;
	TextView mTv;
	Camera mCamera;
	Parameters mCameraSetting;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mTv = (TextView)findViewById(R.id.label) ;
        mCamera = Camera.open();
        mCameraSetting = mCamera.getParameters();
        
        /* This code together with the one in onDestroy() 
         * will make the screen be always on until this Activity gets destroyed. */
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        this.mWakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "My Tag");
        this.mWakeLock.acquire();
                
        mMainWifiObj = (WifiManager) getSystemService(Context.WIFI_SERVICE); 
        mMainWifiObj.disconnect();
        
        mWifiLock = mMainWifiObj.createWifiLock( WifiManager.WIFI_MODE_SCAN_ONLY, "TOTO");
        mWifiLock.acquire();
        
        mWifiReciever = new WifiScanReceiver();
        
        mMainWifiObj.setWifiEnabled(true);
        if (!mMainWifiObj.isScanAlwaysAvailable()) {
        	showMessage("not isScanAlwaysAvailable");
        }
        
        printClass(android.net.wifi.WifiManager.class);
        
        
        Field field = null;
        Object mServices = null;
		try {
			field = WifiManager.class.getDeclaredField("mService");
			field.setAccessible(true);
			mServices = field.get(mMainWifiObj);
			printClass(mServices.getClass());
		} catch (NoSuchFieldException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (IllegalAccessException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

        
     
        
		registerReceiver(mWifiReciever, new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
        mMainWifiObj.startScan();
    }
    
    void printClass(Class CLS) {
    	
    	String name = CLS.toString();
    	name = name.replace(".", "_");
    	int sz = name.length();
    	if (sz>20) name = name.substring(sz-20, sz);
    	String data = "class: " + CLS.toString();
    	
    	data += "\n";
	    data += "*** ANNOTATION ****\n";
	    Annotation[] ann = null;
	    ann = CLS.getAnnotations();
		for(Annotation a : ann) {
			 data += a.toString();
			 data += "\n";
		}
		
		
        data += "\n";
        data += "*** FIELDS ****\n";
		Field[] fields = null;
		fields = CLS.getDeclaredFields();
		for(Field f : fields) {
			 data += f.toString();
			 data += "\n";
		}
		
		data += "\n";
	    data += "*** CLASSES ****\n";
		Class[] cla = null;
		cla = CLS.getClasses();
		for(Class c : cla) {
			 data += c.toString();
			 data += "\n";
		}
		
		data += "\n";
		data += "*** METHODS ****\n";
		Method[] met = null;
		met = CLS.getMethods();
		for(Method m : met) {
			 data += m.toString();
			 data += "\n";
		}
		
		File file = new File("/sdcard/" + name + ".txt");
		try{
		    FileOutputStream f1 = new FileOutputStream(file,false); //True = Append to file, false = Overwrite
		    PrintStream p = new PrintStream(f1);
		    p.print(data);
		    p.close();
		    f1.close();
		} catch (FileNotFoundException e) {
			Log.e("EOROR", e.toString());
		} catch (IOException e) {
			Log.e("EOROR", e.toString());
		} 
    
    }
    
    protected void onPause() {
        
        super.onPause();
     }

     protected void onResume() {
        
        super.onResume();
     }

	 @Override
	 public void onDestroy() {
		 mWifiLock.release();
		 unregisterReceiver(mWifiReciever);
	     this.mWakeLock.release();
		 mCameraSetting.setFlashMode(Parameters.FLASH_MODE_OFF);
    	 mCamera.setParameters(mCameraSetting);
    	 mCamera.stopPreview();
	     mCamera = null; //Camera..open();
	     super.onDestroy();
	 }
 
    
	static Boolean LEDON = false;
	
	void switchLed() {
		LEDON = !LEDON;
    	if (LEDON) {
    		mCameraSetting.setFlashMode(Parameters.FLASH_MODE_TORCH);
    		mCamera.setParameters(mCameraSetting);
    		mCamera.startPreview();
    	} else {
    		mCameraSetting.setFlashMode(Parameters.FLASH_MODE_OFF);
	    	mCamera.setParameters(mCameraSetting);
	    	mCamera.stopPreview();
    	}
	}
	
	
    void showMessage(String msg) {
    	AlertDialog.Builder builder = new AlertDialog.Builder(this);
    	builder.setMessage(msg);
    	builder.setPositiveButton("OK", null);
    	builder.show();
    	
    }
    
    long mFirstUnix = 0;
    long mFirstAP = 0;
    long mLastUnix = 0;
    String data = "";
    long i = 0;
    class WifiScanReceiver extends BroadcastReceiver {

    	
    	
		@Override
		public void onReceive(Context context, Intent intent) {
			
			
			Boolean found = false;
			List<ScanResult> wifiScanList = mMainWifiObj.getScanResults();
			int apCnt = wifiScanList.size();
			
			if (i>20) {
				mFirstUnix = 0;
				mFirstAP = 0;
				mLastUnix = 0;
				data = "";
				i=0;
			}
			i++;
			
			// Unix local date in Milliseconds
			long UnixTimeMS = System.currentTimeMillis();
			if (mFirstUnix == 0) mFirstUnix = UnixTimeMS;
			UnixTimeMS -= mFirstUnix;
			
			for(int apIdx=0; apIdx<apCnt; apIdx++)
			{
				ScanResult sr = wifiScanList.get(apIdx);
				
				if (sr.BSSID.compareTo("d8:6c:e9:1f:1c:7d")==0) {
					found = true;
					
					// WIFI AP Number of microseconds timekeeper has been active (64bits)
					// reminder: The long data type is a 64-bit two's complement integer.
					long APTimeMS = sr.timestamp/1000L;
					if (mFirstAP == 0)   mFirstAP = APTimeMS;
					APTimeMS -= mFirstAP;
					
					data += "" + ((UnixTimeMS-mLastUnix)) + "ms,  AP" + apIdx+"/"+apCnt+",  drift: " + (UnixTimeMS-APTimeMS) + "ms\n";
					break;
				}
			}
			
			if (!found) {
				data += "" + ((UnixTimeMS-mLastUnix)) + "ms,  AP??/"+apCnt+",  drift: ??ms\n";
				mFirstAP = 0;
			}
			
			//display result
			switchLed();
			mTv.setText(data);
			
			mLastUnix = UnixTimeMS;
			
			// AND RESTART !
			mMainWifiObj.startScan();
		}
    }
    
}
